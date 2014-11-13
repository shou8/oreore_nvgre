#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <pwd.h>
#include <errno.h>

#include "base.h"
#include "util.h"
#include "sock.h"
#include "net.h"
#include "log.h"



#define CTL_BUF_LEN DEFAULT_BUFLEN * 16

#define CLIENT_VERSION	"1.0"
#define STDOUT			1



void client_usage(void);
void usage(void);
int argv_to1str(char *buf, int argc, char **argv);
//int argv_to1str_w(char *buf, int optind, int argc, char **argv);



static const struct option options[] = {
	{"socket", required_argument, NULL, 's'},
#ifdef DEBUG
//	{"Debug", no_argument, NULL, 'D'},
#endif
	{"help", no_argument, NULL, 'h'},
	{"version", no_argument, NULL, 'v'},
	{0, 0, 0, 0}
};



int main(int argc, char *argv[]) {

	int opt, sock;
	extern int optind, opterr;
	extern char *optarg;
	char *dom = NULL;
	char rbuf[CTL_BUF_LEN]; // To read buffer
	char wbuf[CTL_BUF_LEN]; // To write buffer

	opterr = 0;
	if (argc == 1) client_usage();

	while ((opt = getopt_long(argc, argv, "s:hdv", options, NULL)) != -1) {
		switch (opt) {
			case 's':
				dom = optarg;
				continue;
			case 'v':
				fprintf(stderr, "VXLAN client version "CLIENT_VERSION"\n");
				exit(EXIT_SUCCESS);
			case 'h':
				client_usage();
				break;
			default:
				break;
		}
		break;
	}

	if ((sock = init_unix_sock(dom, UNIX_SOCK_CLIENT)) < 0) {
		fprintf(stderr, "ERROR: Cannot init unix domain socket.\n");
		if (getuid() == 0)
			fprintf(stderr, "ERROR: \""DAEMON_NAME"\" is running?\n");
		else
			fprintf(stderr, "ERROR: Are you \"root\"?\n");
		exit(EXIT_FAILURE);
	}

	int len = argv_to1str(wbuf, argc-optind, argv+optind);

	switch (len) {
		case 0:
		case -1:
			fprintf(stderr, "ERROR: Nothing to be done (No operation).\n\n");
			client_usage();
		case -2:
			fprintf(stderr, "ERROR: Too long arguments (Over %d characters).\n\n", CTL_BUF_LEN);
			client_usage();
	}

	if (write(sock, wbuf, len) < 0) {
		fprintf(stderr, "ERROR: Write to socket\n");
		perror("write");
		return -1;
	}

	while ((len = read(sock, rbuf, CTL_BUF_LEN)) > 0)
		if (write(STDOUT, rbuf, len) < 0) {
			fprintf(stderr, "ERROR: Write to stdout\n");
			perror("write");
			return -1;
		}

    return 0;
}



void client_usage(void) {

	usage();
	fprintf(stderr, "        Use help command: "CONTROLLER_NAME" help\n");
	exit(EXIT_SUCCESS);
}



void usage(void) {

	fprintf(stderr, "\n");
	fprintf(stderr, "Usage: "CONTROLLER_NAME" [-h|-v] [-s <socket path>] COMMAND\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "%10s,    --help: Show this usage.\n", "-h");
	fprintf(stderr, "%10s, --version: Show software version.\n", "-v");
	fprintf(stderr, "%10s,  --socket: socket path.\n", "-s");
	fprintf(stderr, "COMMAND:\n");
}



/*
int argv_to1str_w(char *buf, int optind, int argc, char **argv) {

	int len;
	int rlen;

	snprintf(buf, CTL_BUF_LEN, "%d", optind);
	len = strlen(buf);
	buf[len] = ' ';
	len++;

	rlen = argv_to1str(buf + len, optind, argc, argv);
	if (rlen < 0) return rlen;
	return rlen + len;
}
*/



int argv_to1str(char *buf, int argc, char **argv) {

	int i, len;
	char *p = buf;

	if (argc < 1) return -1;

	for (i=0; i<argc; i++) {
		if (p - buf > CTL_BUF_LEN) return -2;
		len = strlen(argv[i]);
		strncpy(p, argv[i], len);
		p[len] = ' ';
		p += len + 1;
	}
	*(--p) = '\0';

	return (int)(p - buf);
}
