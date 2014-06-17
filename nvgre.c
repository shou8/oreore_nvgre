#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "base.h"
#include "util.h"
#include "log.h"
#include "nvgre.h"
#include "tap.h"
#include "sock.h"
#include "net.h"
#include "netutil.h"



nvgred nvgre = {
	-1,
	AF_INET6,
	DEFAULT_MCAST_ADDR6,
//	DEFAULT_MCAST_ADDR4,
	{},

	NULL,
	NULL,
	DEFAULT_UNIX_DOMAIN,
	0,
	DEFAULT_MAC_TIMEOUT,
	DEFAULT_CONFIG_PATH
};



static void init_nvi(void);
static device create_nvgre_if(uint8_t *vsid);



int init_nvgre(void)
{
	init_nvi();

	if (get_sockaddr(&nvgre.maddr, nvgre.cmaddr) < 0) {
		log_pcrit("getaddrinfo");
		log_cexit("Invalid multicast address: %s\n", nvgre.cmaddr);
	}

	nvgre.family = nvgre.maddr.ss_family;

	if ((nvgre.sock = init_gre_sock(nvgre.family)) < 0)
		return -1;

	switch (nvgre.family) {
		case AF_INET:
			if (join_mcast4_group(nvgre.sock, ((struct sockaddr_in *)(&nvgre.maddr))->sin_addr, nvgre.if_name) < 0)
			return -1;
			break;
		case AF_INET6:
			if (join_mcast6_group(nvgre.sock, ((struct sockaddr_in6 *)(&nvgre.maddr))->sin6_addr, nvgre.if_name) < 0)
			return -1;
			break;
		default:
			log_cexit("Unknown address family\n");
	}

	return 0;
}



/*
 * Create 3 Demention Matrix
 */
static void init_nvi(void)
{
	nvgre.nvi = (nvgre_i ****)malloc(sizeof(nvgre_i ***) * NUMOF_UINT8);
	if (nvgre.nvi == NULL) log_pcexit("malloc");
	nvgre.nvi[0] = (nvgre_i ***)malloc(sizeof(nvgre_i **) * NUMOF_UINT8 * NUMOF_UINT8);
	if (nvgre.nvi[0] == NULL) log_pcexit("malloc");
	nvgre.nvi[0][0] = (nvgre_i **)malloc(sizeof(nvgre_i *) * NUMOF_UINT8 * NUMOF_UINT8 * NUMOF_UINT8);
	if ( nvgre.nvi[0][0] == NULL ) log_pcexit("malloc");

	int i,j;
	for (i=0; i<NUMOF_UINT8; i++) {
		nvgre.nvi[i] = nvgre.nvi[0] + i * NUMOF_UINT8;
		for (j=0; j<NUMOF_UINT8; j++)
			nvgre.nvi[i][j] = nvgre.nvi[0][0] + i * NUMOF_UINT8 * NUMOF_UINT8 + j * NUMOF_UINT8;
	}

	memset(nvgre.nvi[0][0], 0, sizeof(nvgre_i *) * NUMOF_UINT8 * NUMOF_UINT8 * NUMOF_UINT8);
}



void destroy_nvgre(void)
{
	free(nvgre.nvi[0][0]);
	free(nvgre.nvi[0]);
	free(nvgre.nvi);
	nvgre.nvi = NULL;
}



static device create_nvgre_if(uint8_t *vsid)
{
	device tap;
	uint32_t vsid32 = To32ex(vsid);

	snprintf(tap.name, IF_NAME_LEN, "nvgre%"PRIu32, vsid32);
	log_info("Tap interface \"%s\" is created (vsid: %"PRIu32").\n", tap.name, vsid32);

	tap.sock = tap_alloc(tap.name);
	if (tap.sock < 0) log_cexit("Cannot create tap interface\n");
	tap_up(tap.name);
	get_mac(tap.sock, tap.name, tap.hwaddr);

	return tap;
}



nvgre_i *add_nvi(char *buf, uint8_t *vsid, struct sockaddr_storage maddr)
{
	nvgre_i *v = (nvgre_i *)malloc(sizeof(nvgre_i));
	if (v == NULL) {
		log_pcrit("malloc");
		return NULL;
	}

	memcpy(v->vsid, vsid, VSID_BYTE);
	v->table = init_table(DEFAULT_TABLE_SIZE);
	if (v->table == NULL) {
		log_pcrit("malloc");
		free(v);
		return NULL;
	}

	v->tap = create_nvgre_if(vsid);
	v->timeout = nvgre.timeout;
	v->maddr = maddr;

	nvgre.nvi[vsid[0]][vsid[1]][vsid[2]] = v;

	return v;
}



void del_nvi(char *buf, uint8_t *vsid)
{
	sa_family_t family = nvgre.nvi[vsid[0]][vsid[1]][vsid[2]]->maddr.ss_family;

	if (memcmp(&nvgre.nvi[vsid[0]][vsid[1]][vsid[2]]->maddr, &nvgre.maddr, sizeof(struct sockaddr_storage)) != 0) {
		int i, j, k;
		for (i=0; i<NUMOF_UINT8; i++) {
			for (j=0; j<NUMOF_UINT8; j++) {
				for (k=0; k<NUMOF_UINT8; k++) {
					if (nvgre.nvi[i][j][k] == NULL) continue;
					if (memcmp(&nvgre.nvi[i][j][k]->maddr, &nvgre.nvi[vsid[0]][vsid[1]][vsid[2]]->maddr, sizeof(struct sockaddr_storage)) != 0)
						break;
				}
			}
		}

		if ( i != NUMOF_UINT8 || j != NUMOF_UINT8 || k != NUMOF_UINT8) {
			if (family == AF_INET)
				leave_mcast4_group(nvgre.sock, ((struct sockaddr_in *)(&nvgre.nvi[vsid[0]][vsid[1]][vsid[2]]->maddr))->sin_addr, nvgre.if_name);
			else
				leave_mcast6_group(nvgre.sock, ((struct sockaddr_in6 *)(&nvgre.nvi[vsid[0]][vsid[1]][vsid[2]]->maddr))->sin6_addr, nvgre.if_name);
		}
	}

	close(nvgre.nvi[vsid[0]][vsid[1]][vsid[2]]->tap.sock);
	free(nvgre.nvi[vsid[0]][vsid[1]][vsid[2]]);
	nvgre.nvi[vsid[0]][vsid[1]][vsid[2]] = NULL;
}



