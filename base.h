#ifndef BASE_H_INCLUDED
#define BASE_H_INCLUDED

#include <stdint.h>
#include <net/if.h>



#define DAEMON_NAME			"nvgred"
#define CONTROLLER_NAME		"nvconfig"
#define TAP_BASE_NAME		"nvgre"
#define UNIX_TAP_BASE_NAME	"tap"

/* MAC Address Length */
#define MAC_LEN_BITS	48
#define MAC_LEN			6		// 48bits / uint8_t = 6

#define DEFAULT_BUFLEN	256

#define UNIX_DOMAIN_LEN			1024
#define DEFAULT_UNIX_DOMAIN		"/var/run/nvgre.sock"
#define DEFAULT_MCAST_ADDR4		"224.0.0.100"
#define DEFAULT_MCAST_ADDR6		"FF15::1"

#define DEFAULT_MAC_TIMEOUT		14400
#define DEFAULT_CONFIG_PATH		"/etc/nvgre.conf"
#define DEFAULT_PID_FILE		"/var/run/nvgred.pid"

#define EPOLL_MAX_EVENTS 4096



// For NULL-Terminated
#ifdef OS_LINUX
#define StrCpy(dst, src, len)				\
	do {									\
		strncpy((dst), (src), (len - 1));	\
		*((dst) + sizeof(dst) - 1) = '\0';	\
	} while(0)
#else
#define StrCpy(dst, src, len) (strlcpy((dst), (src), (len)))
#endif /* OS_LINUX */



typedef struct _device_ {
	int sock;
	char name[IFNAMSIZ];
	uint8_t hwaddr[MAC_LEN];
} device;



enum status {
	SUCCESS,
	NOSUCHCMD,
	CMD_FAILED,
	SRV_FAILED,
	LOGIC_FAILED,
	CMD_HELP
};



#define NVGRE_FLAGS			0x20
#define NVGRE_VERSION		0x00
#define NVGRE_PROTOCOLTYPE	0x6558



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



extern int debug_mode;



#endif /* BASE_H_INCLUDED */



