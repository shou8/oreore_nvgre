#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#include "base.h"
#include "netutil.h"
#include "nvgre.h"
#include "log.h"



#define BUF_SIZE			65536

#define NVGRE_FLAGS			0x20
#define NVGRE_VERSION		0x00
#define NVGRE_PROTOCOLTYPE	0x6558



#ifdef DEBUG
static void print_nvgre_hdr(struct nvgre_hdr *nvhdr, FILE *fp);
#endif



int outer_loop(int soc)
{

	char buf[BUF_SIZE], *bp = buf;
	int len;
	int rlen = sizeof(buf) - 1;

	struct sockaddr_storage src;
	socklen_t addrlen = sizeof(src);

	while (1)
	{
		if ((len = recvfrom(soc, buf, rlen, 0,
						(struct sockaddr *)&src, &addrlen)) < 0)
			log_perr("recvfrom");

		/* IP */
		struct iphdr *iphdr = (struct iphdr *)buf;
		size_t iph_len = iphdr->ihl * 4;

#ifdef DEBUG
		if (get_status()) {
			uint16_t sum = checksum((uint16_t *)iphdr, iph_len); fprintf(stdout, "=====  ip_header  ======\n");
			fprintf(stdout, "checksum: 0x%04X\n", sum);
			if (sum != 0 && sum != 0xFFFF) {
				log_err("Invalid checksum: %"PRIu16"\n", sum);
				continue;
			}
		}
#endif

		/* NVGRE */
		struct nvgre_hdr *nvhdr = (struct nvgre_hdr *)(buf + iph_len);
		bp = buf + sizeof(struct nvgre_hdr);
		len -= sizeof(struct nvgre_hdr);

#ifdef DEBUG
		if (get_status())
			print_nvgre_hdr(nvhdr, stdout);
#endif

		nvgre_i *nins = nvgre.nvi[nvhdr->vsid[0]][nvhdr->vsid[1]][nvhdr->vsid[2]];
		if (nins == NULL) continue;

		struct ether_header *eh = (struct ether_header *)bp;
		if (add_data(nins->table, eh->ether_shost, &src) < 0) {
			log_crit("Failed to add data (add_data)\n");
			continue;
		}

		write(nins->tap.sock, bp, len);

#ifdef DEBUG
		if (get_status())
			print_eth_h(eh, stdout);
#endif
	}
}



int inner_loop(nvgre_i *nvi)
{
	char buf[BUF_SIZE];
	char *rp = buf + sizeof(struct nvgre_hdr);
	int rlen = sizeof(buf) - sizeof(struct nvgre_hdr) - 1;
	int len;

	mac_tbl *data;
	struct nvgre_hdr *nvh = (struct nvgre_hdr *)buf;

	while (1)
	{
		if ((len = read(nvi->tap.sock, rp, rlen)) < 0)
			log_perr("inner_loop.read");

		memset(nvh, 0, sizeof(struct nvgre_hdr));
		nvh->flags.byte = NVGRE_FLAGS;
		nvh->ver.byte = NVGRE_VERSION;
		nvh->protocol_type = htons(NVGRE_PROTOCOLTYPE);
		memcpy(nvh->vsid, nvi->vsid, VSID_BYTE);
		len += sizeof(struct nvgre_hdr);

#ifdef DEBUG
		if (get_status())
			print_nvgre_hdr(nvh, stdout);
#endif

		struct ether_header *eh = (struct ether_header *)rp;

#ifdef DEBUG
		if (get_status())
			print_eth_h(eh, stdout);
#endif
		data = find_data(nvi->table, eh->ether_dhost);
		if (data == NULL) {
			if (sendto(nvgre.sock, buf, len, MSG_DONTWAIT, (struct sockaddr *)&nvi->maddr, sizeof(nvi->maddr)) < 0)
				log_perr("inner_loop.sendto");
			continue;
		}

		if (sendto(nvgre.sock, buf, len, MSG_DONTWAIT, (struct sockaddr *)&data->addr, sizeof(data->addr)) < 0)
			log_perr("inner_loop.sendto");
	}

	return 0;
}




#ifdef DEBUG
static void print_nvgre_hdr(struct nvgre_hdr *nvhdr, FILE *fp)
{
	fprintf(fp, "===== nvgre_header =====\n");
	fprintf(fp, "            c-ks----\n");
	fprintf(fp, "flag    : 0b%s - %s\n", get_byte2bits(nvhdr->flags.byte), (nvhdr->flags.byte == NVGRE_FLAGS) ? "OK" : "NG");
	fprintf(fp, "ver     : 0b%s - 0x%02X\n", get_byte2bits(nvhdr->ver.byte), nvhdr->ver.bits.v);
	fprintf(fp, "vsid    : 0x%02X%02X%02X\n", nvhdr->vsid[0], nvhdr->vsid[1], nvhdr->vsid[2]);
	fprintf(fp, "protocol: 0x%04X\n", ntohs(nvhdr->protocol_type));
	fprintf(fp, "flowid  : 0x%02X\n", nvhdr->flowid);
} 
#endif



