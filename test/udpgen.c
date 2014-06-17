#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <netinet/if_ether.h>
#include <errno.h>

//#include "net.h"



#define VXLAN_PORT	4789

#define MCAST_DEFAULT_ADDR 0xef12b500

typedef struct _vxlan_h_ {
	char flag;
	char reserve1[3];
	char vni[3];
	char reserve2;
} vxlan_h;


void sendRaw(void);
void sendUdp(void);
int make_arp_packet(char *buf);
int make_l2_dran_packet(char *buf);
int make_l2_packet(char *buf);
int make_vxlan_header(char *buf);



int main(int argc, char *argv[])
{
	sendUdp();
	//sendRaw();
    return 0;
}



void sendRaw(void) {

	struct ifreq ifreq;
	struct sockaddr_ll sa;
	int sock;
	int ret;

	sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (sock < 0) { 
		perror("socket");
	} else {
		printf("sock: %d\n", sock);
	}
	memset(&ifreq, 0, sizeof(struct ifreq));
	strncpy(ifreq.ifr_name, "vxlan1", IFNAMSIZ-1);
	ret = ioctl(sock, SIOCGIFINDEX, &ifreq);
	if (ret < 0) { 
		perror("ioctl");
	} else {
		printf("ioctl ret: %d\n", ret);
	}

	sa.sll_family = AF_PACKET;
	sa.sll_protocol = htons(ETH_P_ALL);
	sa.sll_ifindex = ifreq.ifr_ifindex;

	ret = bind(sock, (struct sockaddr *)&sa, sizeof(sa));
	if (ret < 0) {
		perror("bind");
	} else {
		printf("bind ret: %d\n", ret);
	}

	int len, buf_len;
	char buf[128];

	while(1) {

		memset(buf, 0, 128);
		buf_len = make_l2_packet(buf);
		printf("buf_len: %d\n", buf_len);
//		len = sendto(sock, buf, 128, 0, (struct sockaddr *)&sa, sizeof(sa));
		len = write(sock, buf, buf_len);
		if (len < 0 ) {
			perror("sendto");
		}
		printf("len: %d\n", len);
		usleep(100000);
	}
}



void sendUdp(void) {

	int len;
	int sock;
	struct sockaddr_in addr, maddr;
	struct ip_mreq mreq;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(VXLAN_PORT);
	inet_pton(AF_INET, "192.168.2.11", &addr.sin_addr.s_addr);
//	inet_pton(AF_INET, "192.168.2.12", &addr.sin_addr.s_addr);

	maddr.sin_family = AF_INET;
	maddr.sin_port = htons(VXLAN_PORT);
	inet_pton(AF_INET, "224.1.1.1", &maddr.sin_addr.s_addr);
//	inet_ntoa(MCAST_DEFAULT_ADDR, &maddr.sin_addr.s_addr);
//	inet_ntoa(MCAST_DEFAULT_ADDR, &mreq.imr_multiaddr);
//	setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq));

	char buf1[128];
	char buf2[128];

	while (1) {

		memset(buf1, 0, 128);
		memset(buf2, 0, 128);
		int len1 = make_vxlan_header(buf1);
		int len2 = make_vxlan_header(buf2);
		make_l2_dran_packet(buf1 + len1);
		make_arp_packet(buf2 + len2);
		len = sendto(sock, buf1, 128, 0, (struct sockaddr *)&addr, sizeof(addr));
		len = sendto(sock, buf2, 128, 0, (struct sockaddr *)&maddr, sizeof(maddr));
		if (len < 0 ) {
			perror("sendto");
		}
	//	usleep(1000);
		sleep(1);
	}
}



int make_l2_packet(char *buf) {

	struct ether_header *eh = (struct ether_header *)buf;
	uint8_t *addr = eh->ether_shost;
	eh->ether_type = ETH_P_IP;

	addr[0] = 0x0;
	addr[1] = 0x1;
	addr[2] = 0x2;
	addr[3] = 0x3;
	addr[4] = 0x4;
	addr[5] = 0x5;

	addr = eh->ether_dhost;

	addr[0] = 0x11;
	addr[1] = 0x11;
	addr[2] = 0x11;
	addr[3] = 0x11;
	addr[4] = 0x11;
	addr[5] = 0x11;

	buf[sizeof(struct ether_header) + 1] = '\0';

	return sizeof(struct ether_header);
}



int make_arp_packet(char *buf) {

	int len = make_l2_dran_packet(buf);
	struct ether_arp *arp = (struct ether_arp *)(buf + len);
	arp->arp_op = ARPOP_REQUEST;

	return len + sizeof(struct ether_arp);
}



// VNI: 001
int make_vxlan_header(char *buf) {

	static char c = 0;
	vxlan_h *v = (vxlan_h *)buf;
	v->flag = 0x08;
	v->vni[0] = 0;
	v->vni[1] = 0;
	v->vni[2] = 1;

	return sizeof(vxlan_h);
}

int make_l2_dran_packet(char *buf) {

	struct ether_header *eh = (struct ether_header *)buf;
	uint8_t *addr = eh->ether_dhost;
	eh->ether_type = ETH_P_ARP;

	addr[0] = 0x0;
	addr[1] = 0x1;
	addr[2] = 0x2;
	addr[3] = 0x3;
	addr[4] = 0x4;
	addr[5] = 0x5;

	addr = eh->ether_shost;

	addr[0] = rand() % 256;
	addr[1] = rand() % 256;
	addr[2] = rand() % 256;
	addr[3] = rand() % 256;
	addr[4] = rand() % 256;
	addr[5] = rand() % 256;

	buf[sizeof(struct ether_header) + 1] = '\0';

	return sizeof(struct ether_header);
}
