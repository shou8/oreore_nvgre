#ifndef SOCK_H_INCLUDED
#define SOCK_H_INCLUDED



#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#ifndef __linux__
#include <netinet/in.h>
#endif /* __linux__ */



#define UNIX_SOCK_SERVER 0
#define UNIX_SOCK_CLIENT 1



int init_gre_sock(sa_family_t family);
int init_unix_sock(char *path, int csflag);
int join_mcast4_group(int sock, struct in_addr *maddr, char *if_name);
int leave_mcast4_group(int sock, struct in_addr *maddr, char *if_name);
int join_mcast6_group(int sock, struct in6_addr *maddr, char *if_name);
int leave_mcast6_group(int sock, struct in6_addr *maddr, char *if_name);



#endif /* SOCK_H_INCLUDED */
