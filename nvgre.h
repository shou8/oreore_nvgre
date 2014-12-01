#ifndef NVGRE_H_INCLUDED
#define NVGRE_H_INCLUDED



#include <pthread.h>
#include <sys/socket.h>

#include "base.h"
#include "table.h"



#define VSID_BIT		24
#define VSID_BYTE		3
#define NUMOF_UINT8		(UINT8_MAX + 1)



typedef struct _nvgre_instance_ {

	pthread_t th;
	uint8_t vsid[VSID_BYTE];
	device *tap;
	list **table;
	struct sockaddr_storage maddr;
	int timeout;				// Specific
} nvgre_i;



typedef struct _nvgred {

	int sock;
	sa_family_t family;
	char cmaddr[DEFAULT_BUFLEN];
	struct sockaddr_storage maddr;

	char *if_name;
	nvgre_i ****nvi;
	char udom[DEFAULT_BUFLEN];
	int lock;
	int timeout;				// Default
	char conf_path[DEFAULT_BUFLEN];
} nvgred;



extern nvgred nvgre;



int init_nvgre(void);
void destroy_nvgre(void);
nvgre_i *add_nvi(char *buf, uint8_t *vsid, struct sockaddr_storage);
void del_nvi(char *buf, uint8_t *vsid);
#ifndef OS_LINUX
void sig_catch(int sig);
#endif /* OS_LINUX */



#endif /* NVGRE_H_INCLUDED */

