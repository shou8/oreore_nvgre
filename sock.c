#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <net/if.h>

#ifndef OS_LINUX
#include <netinet/in.h>
#endif /* OS_LINUX */

#include "base.h"
#include "log.h"
#include "netutil.h"



#define CON_NUM		1



const struct in_addr inaddr_any = { INADDR_ANY };



int init_gre_sock(sa_family_t family)
{
	int sock;

	if ((sock = socket(family, SOCK_RAW, IPPROTO_GRE)) < 0) {
		log_pcrit("socket");
		return -1;
	}

	return sock;
}



int init_unix_sock(char *dom, int csflag)
{
	int sock;
	struct sockaddr_un addr;

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX; // AF_LOCAL
	if (dom == NULL)
		strlcpy(addr.sun_path, DEFAULT_UNIX_DOMAIN, sizeof(addr.sun_path));
	else
		strlcpy(addr.sun_path, dom, sizeof(addr.sun_path));

	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		log_pcrit("unix.socket");
		return -1;
	}

	if (csflag == 0) {

		unlink(addr.sun_path);

		if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
			log_pcrit("unix.bind");
			close(sock);
			return -1;
		}

		if (listen(sock, CON_NUM) < 0) {
			log_pcrit("unix.listen");
			close(sock);
			return -1;
		}

	} else {

		if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
			log_pcrit("unix.connect");
			close(sock);
			return -1;
		}
	}

	return sock;
}



/*
 * Multicast Settings
 */

int join_mcast4_group(int sock, struct in_addr *maddr, char *if_name)
{
	struct ip_mreq mreq;
	char maddr_s[16];

	memset(&mreq, 0, sizeof(mreq));
	mreq.imr_multiaddr = *maddr;
	mreq.imr_interface = (if_name != NULL) ? get_addr(if_name) : inaddr_any;

#ifdef DEBUG
	if (get_status()) {
		inet_ntop(AF_INET, &mreq.imr_multiaddr, maddr_s, sizeof(maddr_s));
		log_debug("Join mcast_addr4 : %s\n", maddr_s);
	}
#endif /* DEBUG */
	
	if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0) {
		log_perr("setsockopt");
		log_err("Fail to set IP_ADD_MEMBERSHIP\n");
		inet_ntop(AF_INET, &mreq.imr_multiaddr, maddr_s, sizeof(maddr_s));
		log_err("mcast_addr: %s\n", maddr_s);
		inet_ntop(AF_INET, &mreq.imr_interface, maddr_s, sizeof(maddr_s));
		log_err("if_addr   : %s\n", maddr_s);
		return -1;
	}

	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, (char *)&mreq.imr_interface, sizeof(mreq.imr_interface)) < 0) {
		log_perr("setsockopt");
		log_err("Fail to set IP_MULTICAST_IF\n");
		inet_ntop(AF_INET, &mreq.imr_multiaddr, maddr_s, sizeof(maddr_s));
		log_err("mcast_addr: %s\n", maddr_s);
		inet_ntop(AF_INET, &mreq.imr_interface, maddr_s, sizeof(maddr_s));
		log_err("if_addr   : %s\n", maddr_s);
		return -1;
	}

	return 0;
}



int leave_mcast4_group(int sock, struct in_addr *maddr, char *if_name)
{
	struct ip_mreq mreq;
	char maddr_s[16];

	memset(&mreq, 0, sizeof(mreq));
	mreq.imr_multiaddr = *maddr;
	mreq.imr_interface = get_addr(if_name);

#ifdef DEBUG
	if (get_status()) {
		inet_ntop(AF_INET, &mreq.imr_multiaddr, maddr_s, sizeof(maddr_s));
		log_debug("Leave mcast_addr4: %s\n", maddr_s);
	}
#endif /* DEBUG */

	if (setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0) {
		inet_ntop(AF_INET, &mreq.imr_multiaddr, maddr_s, sizeof(maddr_s));
		log_perr("setsockopt");
		log_err("Fail to set IP_DROP_MEMBERSHIP\n");
		log_err("mcast_addr: %s\n", maddr_s);
		return -1;
	}

	return 0;
}




int join_mcast6_group(int sock, struct in6_addr *maddr, char *if_name)
{
	struct ipv6_mreq mreq6;
	char maddr_s[16];

	memset(&mreq6, 0, sizeof(mreq6));
	mreq6.ipv6mr_multiaddr = *maddr;
	mreq6.ipv6mr_interface = (if_name != NULL) ? if_nametoindex(if_name) : 0;

#ifdef DEBUG
	if (get_status()) {
		inet_ntop(AF_INET6, &mreq6.ipv6mr_multiaddr, maddr_s, sizeof(maddr_s));
		log_debug("Join mcast_addr6 : %s\n", maddr_s);
	}
#endif /* DEBUG */

	if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, (char *)&mreq6.ipv6mr_interface, sizeof(mreq6.ipv6mr_interface)) < 0) {
		log_perr("setsockopt");
		log_err("Fail to set IPV6_MULTICAST_IF\n");
		inet_ntop(AF_INET6, &mreq6.ipv6mr_multiaddr, maddr_s, sizeof(maddr_s));
		log_err("mcast_addr: %s\n", maddr_s);
		inet_ntop(AF_INET, &mreq6.ipv6mr_interface, maddr_s, sizeof(maddr_s));
		log_err("if_addr   : %s\n", maddr_s);
		return -1;
	}

#ifdef OS_LINUX
	if (setsockopt(sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char *)&mreq6, sizeof(mreq6)) < 0) {
#else
	if (setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, (char *)&mreq6, sizeof(mreq6)) < 0) {
#endif /* OS_LINUX */
		log_perr("setsockopt");
		log_err("Fail to set IPV6_ADD_MEMBERSHIP\n");
		log_err("Interface : %u\n", mreq6.ipv6mr_interface);
		return -1;
	}

	return 0;
}



int leave_mcast6_group(int sock, struct in6_addr *maddr, char *if_name)
{
	struct ipv6_mreq mreq6;
	char maddr_s[16];

	memset(&mreq6, 0, sizeof(mreq6));
	mreq6.ipv6mr_multiaddr = *maddr;
	mreq6.ipv6mr_interface = (if_name != NULL) ? if_nametoindex(if_name) : 0;

#ifdef DEBUG
	if (get_status()) {
		inet_ntop(AF_INET6, &mreq6.ipv6mr_multiaddr, maddr_s, sizeof(maddr_s));
		log_debug("Join mcast_addr6 : %s\n", maddr_s);
	}
#endif /* DEBUG */

#ifdef OS_LINUX
	if (setsockopt(sock, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, (char *)&mreq6.ipv6mr_interface, sizeof(mreq6.ipv6mr_interface)) < 0) {
#else
	if (setsockopt(sock, IPPROTO_IPV6, IPV6_LEAVE_GROUP, (char *)&mreq6.ipv6mr_interface, sizeof(mreq6.ipv6mr_interface)) < 0) {
#endif /* OS_LINUX */
		inet_ntop(AF_INET6, &mreq6.ipv6mr_multiaddr, maddr_s, sizeof(maddr_s));
		log_perr("setsockopt");
		log_err("Fail to set IPV6_DROP_MEMBERSHIP\n");
		log_err("mcast_addr: %s\n", maddr_s);
		return -1;
	}

	return 0;
}



