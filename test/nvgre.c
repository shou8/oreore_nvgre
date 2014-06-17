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
#include <netdb.h>
#include <errno.h>



#define NVGRE_FLAGS			0x20
#define NVGRE_VERSION		0x00
#define NVGRE_PROTOCOLTYPE	0x6558


int make_packet(char *buf);
void get_sockaddr(struct sockaddr_storage *saddr);
int set_nvgre_hdr(char *buf);



struct nvgre_hdr {
	union {
		uint8_t byte;
		struct _fbits {
			uint8_t : 4;
			uint8_t s: 1;
			uint8_t k: 1;
			uint8_t : 1;
			uint8_t c: 1;
		} bits;
	} flags;
	union {
		uint8_t byte;
		struct _vbits {
			uint8_t v: 3;
			uint8_t : 5;
		} bits;
	} ver;
	uint16_t protocol_type;
	uint8_t vsid[3];
	uint8_t flowid;
};



int main(int argc, char *argv[]) {

	int sock;

	sock = socket(AF_INET, SOCK_RAW, IPPROTO_GRE);
	if (sock < 0) {
		perror("socket");
		return -1;
	}

	int len, buf_len;
	char buf[128];

	struct sockaddr_storage saddr;
	get_sockaddr(&saddr);

	while(1) {

		memset(buf, 0, 128);
		buf_len = set_nvgre_hdr(buf);
		printf("buf_len: %d\n", buf_len);
		len = sendto(sock, buf, buf_len, MSG_DONTWAIT, (struct sockaddr *)&saddr, sizeof(saddr));
		if (len < 0 ) {
			perror("sendto");
		}
		printf("len: %d\n", len);
		usleep(1000000);
	}

    return 0;
}



int make_packet(char *buf)
{
	sprintf(buf, "Hello\n");
	return strlen(buf);
}



void get_sockaddr(struct sockaddr_storage *saddr)
{
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_protocol = IPPROTO_GRE;

	if (getaddrinfo("192.168.2.11", NULL, &hints, &res) < 0) {
		perror("getaddrinfo");
		return;
	}

	printf("ai_family   : %d\n", res->ai_family);
	printf("ai_socktype : %d\n", res->ai_socktype);
	printf("ai_protocol : %d\n", res->ai_protocol);
	printf("ai_addrlen  : %d\n", res->ai_addrlen);

	memcpy(saddr, res->ai_addr, res->ai_addrlen);
}



int set_nvgre_hdr(char *buf)
{
	struct nvgre_hdr *hdr = (struct nvgre_hdr *)buf;
	memset(hdr, 0, sizeof(struct nvgre_hdr));
	hdr->flags.byte = NVGRE_FLAGS;
	hdr->ver.byte = NVGRE_VERSION;
	hdr->protocol_type = htons(NVGRE_PROTOCOLTYPE);
	hdr->vsid[0] = 0;
	hdr->vsid[1] = 0;
	hdr->vsid[2] = 100;
	hdr->flowid = 0;

	return sizeof(struct nvgre_hdr);
}
