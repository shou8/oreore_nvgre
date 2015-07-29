#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <inttypes.h>
#include <getopt.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdarg.h>

#include "base.h"
#include "util.h"
#include "log.h"
#include "nvgre.h"
#include "net.h"
#include "sock.h"
#include "cmd.h"
#include "netutil.h"



#define CTL_BUFLEN DEFAULT_BUFLEN * 16



struct cmd_entry {
	const char *name;
	int (*exec)(int soc, int cmd_i, int argc, char *argv[]);
	const char *arg;
	const char *comment;
};



void *inner_loop_thread(void *args);
int order_parse(char *rbuf, int soc);
char *get_argv0(char *buf, char *argv0);
int cmd_usage(int soc, int cmd_i);
int cmd_usage_all(int soc);

static void _soc_printf(int soc, const int len, const char *fmt, ...);

static void _show_nvi(int soc);
static void _show_table(int soc, list **table);

/* Comands: cmd_XXX(char *, int, int, char **); */
int cmd_create_nvi(int soc, int cmd_i, int argc, char *argv[]);
static int _cmd_drop_nvi(int soc, int cmd_i, int argc, char *argv[]);
static int _cmd_exit(int soc, int cmd_i, int argc, char *argv[]);
static int _cmd_list(int soc, int cmd_i, int argc, char *argv[]);
static int _cmd_mac(int soc, int cmd_i, int argc, char *argv[]);
static int _cmd_show(int soc, int cmd_i, int argc, char *argv[]);
static int _cmd_help(int soc, int cmd_i, int argc, char *argv[]);
static int _cmd_clear(int soc, int cmd_i, int argc, char *argv[]);
static int _cmd_flush(int soc, int cmd_i, int argc, char *argv[]);
static int _cmd_time(int soc, int cmd_i, int argc, char *argv[]);
static int _cmd_add(int soc, int cmd_i, int argc, char *argv[]);
static int _cmd_del(int soc, int cmd_i, int argc, char *argv[]);
static int _cmd_info(int soc, int cmd_i, int argc, char *argv[]);
#ifdef DEBUG
static int _cmd_debug(int soc, int cmd_i, int argc, char *argv[]);
#endif /* DEUBG */



struct cmd_entry _cmd_t[] = {
	{ "create", cmd_create_nvi, "<vsid> [<Multicast address>]", "Create instance and interface"},
	{ "drop", _cmd_drop_nvi, "<vsid>", "Delete instance and interface"},
	{ "destroy", _cmd_drop_nvi, "<vsid>", "Alias to \"drop\""},
	{ "exit", _cmd_exit, NULL, "Exit process"},
	{ "list", _cmd_list, NULL, "Show instances"},
	{ "mac", _cmd_mac, "<vsid>", "Show MAC address table"},
	{ "show", _cmd_show, "{instance|mac <vsid>}", "Show instance table (=list) or MAC address table (=mac)"},
	{ "help", _cmd_help, NULL, "Show this help message" },
	{ "clear", _cmd_clear, "[force] <vsid>", "Clear time outed MAC address from table. If added \"force\", similarly behave \"flush\" commnad" },
	{ "flush", _cmd_flush, "<vsid>", "Drop and Create MAC address table (all cached MAC address is discarded)" },
	{ "time", _cmd_time, "{<vsid>|default} <time>", "Set timeout"},
	{ "add", _cmd_add, "<vsid> <MAC_addr> <NVE IPaddr>", "Manually add cache"},
	{ "del", _cmd_del, "<vsid> <MAC_addr>", "Manually delete cache"},
	{ "info", _cmd_info, "[<vsid>]", "Get general information"},
#ifdef DEBUG
	{ "debug", _cmd_debug, NULL, "Get general information"},
#endif /* DEBUG */
};

static int _cmd_len = sizeof(_cmd_t) / sizeof(struct cmd_entry);



void ctl_loop(char *dom)
{
	int usoc, asoc, len;
	char rbuf[CTL_BUFLEN];
	char wbuf[CTL_BUFLEN];

	if ((usoc = init_unix_sock(dom, UNIX_SOCK_SERVER)) < 0)
		log_pcexit("ctl_loop.init_unix_sock");

	while (1) {

		memset(wbuf, 0, sizeof(wbuf));

		if ((asoc = accept(usoc, NULL, 0)) < 0) {
			log_perr("ctl_loop.accept");
			continue;
		}

		if ((len = read(asoc, rbuf, CTL_BUFLEN)) < 0) {
			log_perr("ctl_loop.read");
			continue;
		}

		rbuf[len] = '\0';
		switch (order_parse(rbuf, asoc)) {
			case SUCCESS:
				break;
			case NOSUCHCMD:
				_soc_printf(asoc, CTL_BUFLEN, "ERROR, No such command: %s\n", rbuf);
				break;
			case CMD_FAILED:
			case SRV_FAILED:
			case LOGIC_FAILED:
			default:
				break;
		}

		close(asoc);
	}

	close(usoc);
}



/*
 * "inner_loop" function infinite loop.
 * So we don'nt have to care memory location.
 *
 * If inner_loop is not infinite loop,
 * you have to use "malloc" to allocate to heap area.
 */

void *inner_loop_thread(void *args)
{
	uint8_t vsid[VSID_BYTE];
	memcpy(vsid, (uint8_t *)args, VSID_BYTE);
	free(args);

	inner_loop(nvgre.nvi[vsid[0]][vsid[1]][vsid[2]]);

	/* Cannot Reach */
	log_crit("The instance (vsid: %"PRIx32") is doen.", To32ex(vsid));
	del_nvi(NULL, vsid);

	return NULL;
}



int order_parse(char *rbuf, int soc)
{
	int i, argc;
	char *p;
	char *argv[CTL_BUFLEN];

	p = strtok(rbuf, " ");
	for (i = 0; i < _cmd_len; i++)
		if (str_cmp(_cmd_t[i].name, p)) break;

	if (i == _cmd_len) return NOSUCHCMD;

	for (argc = 0; p != NULL; argc++) {
		argv[argc] = p;
		p = strtok(NULL, " ");
	}

	return ((_cmd_t[i].exec)(soc, i, argc, argv));
}



int _cmd_usage(int soc, int cmd_i)
{
	_soc_printf(soc, CTL_BUFLEN, "Usage: %s %s %s\n", CONTROLLER_NAME, _cmd_t[cmd_i].name, (_cmd_t[cmd_i].arg == NULL) ? "":_cmd_t[cmd_i].arg);
	return CMD_FAILED;
}



/****************
 ****************
 ***          ***
 *** Commands ***
 ***          ***
 ****************
 ****************/



int cmd_create_nvi(int soc, int cmd_i, int argc, char *argv[])
{
	if (argc < 2) {
		_soc_printf(soc, CTL_BUFLEN, "ERROR: Too few arguments\n");
		return _cmd_usage(soc, cmd_i);
	}

	if (argc > 3) {
		_soc_printf(soc, CTL_BUFLEN, "ERROR: Too many arguments\n");
		return _cmd_usage(soc, cmd_i);
	}

	char buf[CTL_BUFLEN];
	char *vsid_s = argv[1];
//	uint8_t vsid[VSID_BYTE];
	uint8_t *vsid = (uint8_t *)malloc(sizeof(uint8_t) * VSID_BYTE);
	uint32_t vsid32 = 0;
	int status = get32and8arr(buf, vsid_s, &vsid32, vsid);
	if (status != SUCCESS) return status;

	if (nvgre.nvi[vsid[0]][vsid[1]][vsid[2]] != NULL) {
		_soc_printf(soc, CTL_BUFLEN, "ERROR: The instance (vsid: %s) has already existed.\n", vsid_s);
		return LOGIC_FAILED;
	}

	struct sockaddr_storage maddr;
	memcpy(&maddr, &nvgre.maddr, sizeof(struct sockaddr_storage));
	if (argc == 3) {
		if (get_sockaddr(&maddr, argv[2]) < 0) {
			_soc_printf(soc, CTL_BUFLEN, "ERROR: Invalid IP[v4|v6] address: %s", argv[2]);
			return LOGIC_FAILED;
		}

		if (maddr.ss_family != nvgre.maddr.ss_family) {
			_soc_printf(soc, CTL_BUFLEN, "ERROR: Invalid address family: %s (Expected %s)", argv[2], (nvgre.maddr.ss_family == AF_INET) ? "IPv4" : "IPv6");
			return LOGIC_FAILED;
		}
	}

	nvgre_i *v = add_nvi(buf, vsid, maddr);
	if (v == NULL) {
		_soc_printf(soc, CTL_BUFLEN, "error is occured in server, please refer \"syslog\".\n");
		return SRV_FAILED;
	}

	pthread_t th;
	pthread_create(&th, NULL, inner_loop_thread, vsid);
	v->th = th;

	_soc_printf(soc, CTL_BUFLEN, "=== Set ===\nvsid\t\t: %"PRIu32"\n", vsid32);
	log_info("vsid: %"PRIu32" is created\n", vsid32);

	return SUCCESS;
}



static int _cmd_drop_nvi(int soc, int cmd_i, int argc, char *argv[])
{
	if (argc != 2) {
		if (argc < 2)
			_soc_printf(soc, CTL_BUFLEN, "ERROR: Too few arguments\n");
		else
			_soc_printf(soc, CTL_BUFLEN, "ERROR: Too many arguments\n");
		return _cmd_usage(soc, cmd_i);
	}

	char buf[CTL_BUFLEN];
	char *vsid_s = argv[1];
	uint32_t vsid32 = 0;
	uint8_t vsid[VSID_BYTE];
	int status = get32and8arr(buf, vsid_s, &vsid32, vsid);
	if (status != SUCCESS) return status;

	if (nvgre.nvi[vsid[0]][vsid[1]][vsid[2]] == NULL) {
		_soc_printf(soc, CTL_BUFLEN, "ERROR: The instance (vsid: %"PRIu32") does not exist.\n", vsid32);
		return LOGIC_FAILED;
	}

	pthread_cancel((nvgre.nvi[vsid[0]][vsid[1]][vsid[2]])->th);
	del_nvi(buf, vsid);

	_soc_printf(soc, CTL_BUFLEN, "=== Unset ===\nvsid\t\t: %"PRIu32"\n", vsid32);
	log_info("vsid: %"PRIu32" is dropped\n", vsid32);

	return SUCCESS;
}



static int _cmd_exit(int soc, int cmd_i, int argc, char *argv[])
{
	if (argc != 1) {
		_soc_printf(soc, CTL_BUFLEN, "ERROR: Too many arguments\n");
		return _cmd_usage(soc, cmd_i);
	}

	uint32_t i, j, k;

	for (i=0; i<NUMOF_UINT8; i++) {
		for (j=0; j<NUMOF_UINT8; j++) {
			for (k=0; k<NUMOF_UINT8; k++) {
				if (nvgre.nvi[i][j][k] != NULL) {
					nvgre_i *v = nvgre.nvi[i][j][k];
					pthread_cancel(v->th);

					uint8_t vsid[3] = {i, j, k};
					del_nvi(NULL, vsid);
				}
			}
		}
	}

	destroy_nvgre();
	log_iexit("Exit by order.\n");
	_soc_printf(soc, CTL_BUFLEN, "Exit by order.\n");
	return SUCCESS;
}



static int _cmd_list(int soc, int cmd_i, int argc, char *argv[])
{
	if (argc != 1) {
		_soc_printf(soc, CTL_BUFLEN, "ERROR: Too many arguments\n");
		return _cmd_usage(soc, cmd_i);
	}

	_soc_printf(soc, CTL_BUFLEN, "\n");
	_show_nvi(soc);

	return SUCCESS;
}



static int _cmd_mac(int soc, int cmd_i, int argc, char *argv[])
{
	if (argc != 2) {
		_soc_printf(soc, CTL_BUFLEN, ((argc < 2) ? "Too few arguments\n" : "Too many arguments\n"));
		return _cmd_usage(soc, cmd_i);
	}

	char buf[CTL_BUFLEN];
	int status = SUCCESS;
	uint8_t vsid[VSID_BYTE];
	uint32_t vsid32 = 0;

	_soc_printf(soc, CTL_BUFLEN, "\n   ----- show MAC table -----   \n");
	status = get32and8arr(buf, argv[1], &vsid32, vsid);
	if (status != SUCCESS) return status;

	if (nvgre.nvi[vsid[0]][vsid[1]][vsid[2]] == NULL) {
		_soc_printf(soc, CTL_BUFLEN, "ERROR: The instance (vsid: %"PRIu32") does not exist.\n", vsid32);
		return LOGIC_FAILED;
	}

	_show_table(soc, nvgre.nvi[vsid[0]][vsid[1]][vsid[2]]->table);

	return SUCCESS;
}



static int _cmd_show(int soc, int cmd_i, int argc, char *argv[])
{
	char *cmd[] = {
		"instance",
		"mac"
	};

	if (argc < 2)
		return _cmd_usage(soc, cmd_i);

	int i;
	int len = sizeof(cmd) / sizeof(*cmd);

	for (i=0; i<len; i++)
		if (str_cmp(cmd[i], argv[1])) break;

	switch (i) {
		case 0: {
				char *cargv[] = {argv[0], "list"};
				return _cmd_list(soc, cmd_i, argc-1, cargv);
			}
			break;
		case 1:
			return _cmd_mac(soc, cmd_i, argc-1, &argv[1]);
			break;
		default:
			return _cmd_usage(soc, cmd_i);
	}

	return SUCCESS;
}



#define HELP_NAME_LEN		"16"
#define HELP_ARG_LEN		"32"

static int _cmd_help(int soc, int cmd_i, int argc, char *argv[])
{
	int i;

	if (argc != 1) {
		_soc_printf(soc, CTL_BUFLEN, "ERROR: Too many arguments\n", 26);
		return _cmd_usage(soc, cmd_i);
	}

	_soc_printf(soc, CTL_BUFLEN, "\n%"HELP_NAME_LEN"s | %-"HELP_ARG_LEN"s | %s\n", "name", "arguments", "comment");
	_soc_printf(soc, CTL_BUFLEN, "   --------------+----------------------------------+---------------\n");

	for (i=0; i<_cmd_len; i++)
		_soc_printf(soc, CTL_BUFLEN, "%"HELP_NAME_LEN"s   %-"HELP_ARG_LEN"s : %s\n", _cmd_t[i].name, (_cmd_t[i].arg == NULL)? "":_cmd_t[i].arg, _cmd_t[i].comment);

	return CMD_HELP;
}



static int _cmd_clear(int soc, int cmd_i, int argc, char *argv[])
{
	char buf[CTL_BUFLEN];

	if (argc < 2) {
		_soc_printf(soc, CTL_BUFLEN, "ERROR: Too few arguments\n");
		return _cmd_usage(soc, cmd_i);
	}

	int num = (argc == 3 && str_cmp(argv[1], "force")) ? 2 : 1;
	char *vsid_s = argv[num];
	uint8_t vsid[VSID_BYTE];
	uint32_t vsid32 = 0;
	int status = get32and8arr(buf, vsid_s, &vsid32, vsid);
	if (status != SUCCESS) return status;

	nvgre_i *vi = nvgre.nvi[vsid[0]][vsid[1]][vsid[2]];
	if (vi == NULL) {
		_soc_printf(soc, CTL_BUFLEN, "ERROR: The instance (vsid: %"PRIu32") does not exist.\n", vsid32);
		return LOGIC_FAILED;
	}

	if (num == 2) {
		vi->table = clear_table_all(vi->table);
		_soc_printf(soc, CTL_BUFLEN, "INFO : The instance(vsid: %"PRIu32")'s MAC address table is recreated.\n", vsid32);
	} else {
		int num = clear_table_timeout(vi->table, vi->timeout);
		_soc_printf(soc, CTL_BUFLEN, "INFO : Timeouted MAC address (%d) were deleted from vsid %"PRIu32"\n", num, vsid32);
	}

	return SUCCESS;
}



static int _cmd_flush(int soc, int cmd_i, int argc, char *argv[])
{
	if (argc < 2) {
		_soc_printf(soc, CTL_BUFLEN, "ERROR: Too few arguments\n");
		return _cmd_usage(soc, cmd_i);
	}

	char *argv_w[] = {"clear", "force", argv[1]};
	return _cmd_clear(soc, cmd_i, 3, argv_w);
}



static int _cmd_time(int soc, int cmd_i, int argc, char *argv[])
{
	if (argc < 2) {
		_soc_printf(soc, CTL_BUFLEN, "ERROR: Too few arguments\n");
		return _cmd_usage(soc, cmd_i);
	}

	int *timep;
	char buf[CTL_BUFLEN];

	if (str_cmp(argv[1], "default")) {
		timep = &nvgre.timeout;
	} else {
		char *vsid_s = argv[1];
		uint32_t vsid32 = 0;
		uint8_t vsid[VSID_BYTE];
		int status = get32and8arr(buf, vsid_s, &vsid32, vsid);
		if (status != SUCCESS) return status;
	
		if (nvgre.nvi[vsid[0]][vsid[1]][vsid[2]] == NULL) {
			_soc_printf(soc, CTL_BUFLEN, "ERROR: The instance (vsid: %"PRIu32") does not exist.\n", vsid32);
			return LOGIC_FAILED;
		}
		timep = &(nvgre.nvi[vsid[0]][vsid[1]][vsid[2]]->timeout);
	}

	int time = atoi(argv[2]);
	if (time <= 0) {
		_soc_printf(soc, CTL_BUFLEN, "ERROR: Cannot convert value of timeout: %s\n", argv[2]);
		return LOGIC_FAILED;
	}

	*timep = time;
	_soc_printf(soc, CTL_BUFLEN, "INFO : vsid %s timeout values is set: %d\n", argv[1], time);

	return SUCCESS;
}



static int _cmd_add(int soc, int cmd_i, int argc, char *argv[])
{
	if (argc < 4) {
		_soc_printf(soc, CTL_BUFLEN, "ERROR: Too few arguments\n");
		return _cmd_usage(soc, cmd_i);
	}

	char buf[CTL_BUFLEN];
	char *vsid_s = argv[1];
	uint8_t vsid[VSID_BYTE];
	uint32_t vsid32 = 0;
	int status = get32and8arr(buf, vsid_s, &vsid32, vsid);
	if (status != SUCCESS) return status;

	if (nvgre.nvi[vsid[0]][vsid[1]][vsid[2]] == NULL) {
		_soc_printf(soc, CTL_BUFLEN, "ERROR: The instance (vsid: %"PRIu32") does not exist.\n", vsid32);
		return LOGIC_FAILED;
	}

	uint8_t mac[MAC_LEN];

	if ( inet_atom(mac, argv[2]) == -1 ) {
		_soc_printf(soc, CTL_BUFLEN, "ERROR: Invalid MAC address \"%s\"\n", argv[2]);
		return _cmd_usage(soc, cmd_i);
	}

	struct sockaddr_storage addr;
	if ( get_sockaddr(&addr, argv[3]) != 0 ) {
		_soc_printf(soc, CTL_BUFLEN, "ERROR: Invalid IP[v4|v6] address \"%s\"\n", argv[3]);
		return _cmd_usage(soc, cmd_i);
	}

	if ( add_data(nvgre.nvi[vsid[0]][vsid[1]][vsid[2]]->table, mac, &addr) < 0 ) {
		log_bperr(buf, "malloc");
		_soc_printf(soc, CTL_BUFLEN, "%s\n", buf);
		return SRV_FAILED;
	}

	mtos(buf, mac);
	_soc_printf(soc, CTL_BUFLEN, "INFO : added, vsid %"PRIu32": %s => %s\n", vsid32, buf, argv[3]);
	return SUCCESS;
}



static int _cmd_del(int soc, int cmd_i, int argc, char *argv[])
{
	if (argc < 3) {
		_soc_printf(soc, CTL_BUFLEN, "ERROR: Too few arguments\n");
		return _cmd_usage(soc, cmd_i);
	}

	char buf[CTL_BUFLEN];
	char *vsid_s = argv[1];
	uint8_t vsid[VSID_BYTE];
	uint32_t vsid32 = 0;
	int status = get32and8arr(buf, vsid_s, &vsid32, vsid);
	if (status != SUCCESS) return status;

	if (nvgre.nvi[vsid[0]][vsid[1]][vsid[2]] == NULL) {
		_soc_printf(soc, CTL_BUFLEN, "ERROR: The instance (vsid: %"PRIu32") does not exist.\n", vsid32);
		return LOGIC_FAILED;
	}

	uint8_t mac[MAC_LEN];

	if ( inet_atom(mac, argv[2]) == -1 ) {
		_soc_printf(soc, CTL_BUFLEN, "ERROR: Invalid MAC address \"%s\"\n", argv[2]);
		return _cmd_usage(soc, cmd_i);
	}

	mtos(buf, mac);
	if ( del_data(nvgre.nvi[vsid[0]][vsid[1]][vsid[2]]->table, mac) < 0 ) {
		_soc_printf(soc, CTL_BUFLEN, "INFO : No such MAC address in table: \"%s\"\n", argv[2]);
		return SUCCESS;
	}

	_soc_printf(soc, CTL_BUFLEN, "INFO : deleted, vsid %"PRIu32": %s\n", vsid32, buf);
	return SUCCESS;
}



#define INFO_PAD "32"

static int _cmd_info(int soc, int cmd_i, int argc, char *argv[])
{
	if (argc > 2) {
		_soc_printf(soc, CTL_BUFLEN, "ERROR: Too many arguments\n");
		return _cmd_usage(soc, cmd_i);
	}

	char buf[CTL_BUFLEN];

	if (argc == 2) {
		char *vsid_s = argv[1];
		uint8_t vsid[VSID_BYTE];
		uint32_t vsid32 = 0;
		int status = get32and8arr(buf, vsid_s, &vsid32, vsid);
		if (status != SUCCESS) return status;
	
		if (nvgre.nvi[vsid[0]][vsid[1]][vsid[2]] == NULL) {
			_soc_printf(soc, CTL_BUFLEN, "ERROR: The instance (vsid: %"PRIu32") does not exist.\n", vsid32);
			return LOGIC_FAILED;
		}

		_soc_printf(soc, CTL_BUFLEN, "----- vsid: %s information -----\n", vsid_s);
		_soc_printf(soc, CTL_BUFLEN, "%-"INFO_PAD"s: %s\n", "Tap interface", nvgre.nvi[vsid[0]][vsid[1]][vsid[2]]->tap->name);
		_soc_printf(soc, CTL_BUFLEN, "%-"INFO_PAD"s: %s\n", "Address family", (nvgre.nvi[vsid[0]][vsid[1]][vsid[2]]->maddr.ss_family == AF_INET) ? "IPv4" : "IPv6");
		_soc_printf(soc, CTL_BUFLEN, "%-"INFO_PAD"s: %s\n", "Multicast IP address", get_straddr(&nvgre.nvi[vsid[0]][vsid[1]][vsid[2]]->maddr));
		_soc_printf(soc, CTL_BUFLEN, "%-"INFO_PAD"s: %d\n", "Cache time out", nvgre.nvi[vsid[0]][vsid[1]][vsid[2]]->timeout);

		return SUCCESS;
	}

	_soc_printf(soc, CTL_BUFLEN, "----- "DAEMON_NAME" information -----\n");
	_soc_printf(soc, CTL_BUFLEN, "%-"INFO_PAD"s: %s\n", "Address family", (nvgre.family == AF_INET) ? "IPv4" : "IPv6");
	_soc_printf(soc, CTL_BUFLEN, "%-"INFO_PAD"s: %s\n", "Multicast interface", (nvgre.if_name == NULL) ? "Auto" : nvgre.if_name);
	_soc_printf(soc, CTL_BUFLEN, "%-"INFO_PAD"s: %s\n", "Default multicast address", nvgre.cmaddr);
	_soc_printf(soc, CTL_BUFLEN, "%-"INFO_PAD"s: %s\n", "Unix Socket path", nvgre.udom);
	_soc_printf(soc, CTL_BUFLEN, "%-"INFO_PAD"s: %d\n", "Default cache time out", nvgre.timeout);

	return SUCCESS;
}



/***********************
 ***********************
 ***                 ***
 *** Pirvate Utility ***
 ***                 ***
 ***********************
 ***********************/



#define vsid_PAD_LEN "11"

static void _show_nvi(int soc)
{
	nvgre_i ****nvi = nvgre.nvi;

	_soc_printf(soc, CTL_BUFLEN, "%"vsid_PAD_LEN"s | %s\n", "vsid", "Multicast address");
	_soc_printf(soc, CTL_BUFLEN, " -----------+----------------\n");

	int i,j,k;
	for (i=0; i<NUMOF_UINT8; i++)
		for (j=0; j<NUMOF_UINT8; j++)
			for (k=0; k<NUMOF_UINT8; k++)
				if (nvi[i][j][k] != NULL) {
					uint32_t vsid32 = To32(i, j, k);
					_soc_printf(soc, CTL_BUFLEN, "%"vsid_PAD_LEN""PRIu32" : %s\n", vsid32, get_straddr(&nvi[i][j][k]->maddr));
				}
}



static void _show_table(int soc, list **table)
{
	int cnt = 0;
	unsigned int table_size = get_table_size();

	list **root;
	list *lp;

	for (root = table; root-table < table_size; root++) {
		if (*root == NULL) continue;
		_soc_printf(soc, CTL_BUFLEN, "%7d: ", (int)(root-table));

		for (lp = *root; lp != NULL; lp = lp->next) {

			if (lp->data == NULL) {
				_soc_printf(soc, CTL_BUFLEN, "NULL\n");
				continue;
			}

			uint8_t *hwaddr = (lp->data)->hw_addr;
			_soc_printf(soc, CTL_BUFLEN, "%02X%02X:%02X%02X:%02X%02X => %s, ", hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5], get_straddr(&(lp->data)->addr));
			cnt++;
		}
		_soc_printf(soc, CTL_BUFLEN, "NULL\n", 5);
	}

	_soc_printf(soc, CTL_BUFLEN, "\nCount: %d\n", cnt);
}



static void _soc_printf(int soc, const int len, const char *fmt, ...)
{
	char buf[CTL_BUFLEN];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, len, fmt, ap);
	va_end(ap);
	int ret = write(soc, buf, strlen(buf));
	if (ret < 0) log_perr("cmd.soc_printf.write");
}



/******************
 ******************
 ***            ***
 *** DEBUG only ***
 ***            ***
 ******************
 ******************/



#ifdef DEBUG



static int _cmd_debug(int soc, int cmd_i, int argc, char *argv[])
{
	enable_debug();
	_soc_printf(soc, CTL_BUFLEN, "Entering DEBUG mode\n");
	return SUCCESS;
}



#endif /* DEBUG */
