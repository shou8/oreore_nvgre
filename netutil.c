#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <net/ethernet.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#ifndef OS_LINUX
#include <sys/types.h>
#include <ifaddrs.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#endif

#include "netutil.h"
#include "log.h"



/* ARP Caching time */
#define ARP_CACHE_TIME_HOUR		0
#define ARP_CACHE_TIME_MIN		10	// Linux(Debian 7.0 wheezy 32bit)'s default is 7 min (See Note1)
#define ARP_CACHE_TIME_SEC		0
#define ARP_CACHE_TIME			(((ARP_CACHE_TIME_HOUR * 60) + \
									ARP_CACHE_TIME_MIN) * 60 + \
									ARP_CACHE_TIME_SEC)

/* MAC Address Table */
//#define MAC_ADDR_TABLE_SIZE		(8 * 1024)

/* To represent MAC Address by uint64_t */
#define MAC_SPACE 2		// uint64_t - uint8_t * 6 = 16bit = 2Byte 

// inet_atom
#define MAC_OUI_RANGE			6
#define MAC_OUI_RANGE_STR		12 
#define ASCII_RANGE_NUM_LOWER	48
#define ASCII_RANGE_NUM_HIGHER	57
#define ASCII_RANGE_ALP_LOWER	65
#define ASCII_RANGE_ALP_HIGHER	70
#define ASCII_ALP_TO_HEX		55
#define HEX_SHIFT				4


static uint8_t cton(char c);



/* For Debug: MAC address to String */
void mtos(char *str, uint8_t hwaddr[MAC_LEN])
{
	sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X", hwaddr[0], hwaddr[1],
								hwaddr[2], hwaddr[3],
								hwaddr[4], hwaddr[5]);
}



int cmp_mac(uint8_t hwaddr1[MAC_LEN], uint8_t hwaddr2[MAC_LEN])
{
	return memcmp(hwaddr1, hwaddr2, MAC_LEN);
}



/*
 * @ARG:
 *		char *if_name:
 *			Interface's Name (ex. eth0)
 *
 * @RET:
 *		struct in_addr: INADDR_ANY(e.g. 0): No interface or Some error.
 *		struct in_addr: Not 0				: Interface's Address
 */
struct in_addr get_addr(char *if_name)
{
	int fd;
	struct ifreq ifr;
	struct sockaddr_in *addr;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		log_pcexit("socket");

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, if_name, IF_NAMESIZE-1);
	if (ioctl(fd, SIOCGIFADDR, &ifr) == -1) {
		close(fd);
		log_pcexit("ioctl");
	}

	close(fd);

	addr = (struct sockaddr_in *)&ifr.ifr_addr;
	return addr->sin_addr;
}



uint8_t *get_mac(int sock, char *name, uint8_t *hwaddr)
{
#ifdef OS_LINUX
//	uint8_t *addr;
	struct ifreq ifreq;

	strncpy(ifreq.ifr_name, name, sizeof(ifreq.ifr_name) - 1);
	if ( ioctl(sock, SIOCGIFHWADDR, &ifreq) == -1 ) {
		log_pcrit("ioctl");
		close(sock);
		return NULL;
	} else {
//		addr = (uint8_t *)&ifreq.ifr_hwaddr.sa_data;
//		memcpy(hwaddr, addr, MAC_LEN);
		memcpy(hwaddr, &ifreq.ifr_hwaddr.sa_data, MAC_LEN);
	}
#else
	struct ifaddrs *ifa_list, *ifa;
	if ( getifaddrs(&ifa_list) < 0 ) {
		log_pcrit("getifaddrs");
		close(sock);
		return NULL;
	}
	for (ifa = ifa_list; ifa; ifa = ifa->ifa_next) {
		struct sockaddr_dl *dl = (struct sockaddr_dl *)ifa->ifa_addr;
		if ( dl->sdl_family == AF_LINK && dl->sdl_type == IFT_ETHER &&
				strncmp(name, ifa->ifa_name, dl->sdl_nlen) == 0 ) {
			memcpy(hwaddr, LLADDR(dl), MAC_LEN);
			break;
		}
	}
#endif

	return hwaddr;
}



#include <stdlib.h>

/*
 * Get random mac address for DEBUG
 */
void get_ran_mac( uint8_t hwaddr[MAC_LEN] )
{
	int i = 0;
	for (i=0; i<MAC_LEN; i++)
		hwaddr[i] = rand() % 0xFF;
}



char *eth_ntoa(uint8_t *hwaddr, char *buf, size_t size)
{
	snprintf(buf, size, "%02X:%02X:%02X:%02X:%02X:%02X", hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);
	return buf;
}



void print_eth_h(struct ether_header *eh, FILE *fp)
{
	uint16_t eth_type = ntohs(eh->ether_type);
	char buf[128];

	fprintf(fp, "ether_header -----\n");
	fprintf(fp, "ether_dhost: %s\n", eth_ntoa(eh->ether_dhost, buf, sizeof(buf)));
	fprintf(fp, "ether_shost: %s\n", eth_ntoa(eh->ether_shost, buf, sizeof(buf)));
	fprintf(fp, "ether_type : ");

	switch(eth_type) {
		case ETHERTYPE_PUP:
			fprintf(fp, "(PUP)\n");
			break;
		case ETHERTYPE_SPRITE:
			fprintf(fp, "(SPRITE)\n");
			break;
		case ETHERTYPE_IP:
			fprintf(fp, "(IP)\n");
			break;
		case ETHERTYPE_ARP:
			fprintf(fp, "(ARP)\n");
			break;
		case ETHERTYPE_REVARP:
			fprintf(fp, "(REVARP)\n");
			break;
		case ETHERTYPE_AT:
			fprintf(fp, "(AT)\n");
			break;
		case ETHERTYPE_AARP:
			fprintf(fp, "(AARP)\n");
			break;
		case ETHERTYPE_VLAN:
			fprintf(fp, "(VLAN)\n");
			break;
		case ETHERTYPE_IPX:
			fprintf(fp, "(IPX)\n");
			break;
		case ETHERTYPE_IPV6:
			fprintf(fp, "(IPv6)\n");
			break;
		case ETHERTYPE_LOOPBACK:
			fprintf(fp, "(LOOPBACK)\n");
			break;
		default:
			fprintf(fp, "(unknown)\n");
			break;
	}
}



int inet_atom(uint8_t mac[MAC_LEN], char *mac_s)
{
	char *p = mac_s;
	uint8_t c = 0;
	int i = 0;

	for (p = mac_s, i = 0; *p != '\0' && p != NULL && i < MAC_OUI_RANGE_STR; p++) {

		if (*p == '.' || *p == ':')
			continue;

		c = cton(*p);
		if ( c > 15 ) return -1;

		int j = i/2;
		if ( i % 2 == 1 ) mac[j] = mac[j] << HEX_SHIFT;
		mac[j] += c;
		i++;
	}

	return 0;
}



static uint8_t cton(char c)
{
    if(c >= ASCII_RANGE_NUM_LOWER && c <= ASCII_RANGE_NUM_HIGHER)
        return (c - ASCII_RANGE_NUM_LOWER);

    c = toupper(c);
    if(c < ASCII_RANGE_ALP_LOWER || c > ASCII_RANGE_ALP_HIGHER)
		return -1;

    return (uint8_t)(c - ASCII_ALP_TO_HEX);
}



int get_sockaddr(struct sockaddr_storage *saddr, const char *caddr)
{
	struct addrinfo *res;
	struct addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_protocol = IPPROTO_GRE;
	hints.ai_flags = AI_NUMERICHOST;

	if (getaddrinfo(caddr, NULL, &hints, &res) < 0)
		return -1;

	memcpy(saddr, res->ai_addr, res->ai_addrlen);
	saddr->ss_family = res->ai_family;

	return 0;
}



char *get_straddr(struct sockaddr_storage *saddr)
{
	static char str[DEFAULT_BUFLEN];

	inet_ntop(saddr->ss_family, (saddr->ss_family == AF_INET) ? (void *)&((struct sockaddr_in *)saddr)->sin_addr : (void *)&((struct sockaddr_in6 *)saddr)->sin6_addr, str, DEFAULT_BUFLEN);

	return str;
}



#define BIT_SIZE	8



char *get_byte2bits(uint8_t bits)
{
	int bit = 1;
	int i = 0;
	static char str[BIT_SIZE + 1];

	str[BIT_SIZE] = '\0';

	for (i=BIT_SIZE-1; i>=0; i--, bit <<= 1)
		str[i] = (bits & bit) ? '1' : '0';

	return str;
}



uint16_t checksum(uint16_t *buf, int bufsz)
{ 
	uint32_t sum = 0;
	while (bufsz > 1) {
		sum += *buf;
		buf++;
		bufsz -= 2;
	}

	if (bufsz == 1)
		sum += *(unsigned char *)buf;

	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);

	return ~sum;
}

