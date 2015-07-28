#include <stdio.h>
#include <stdlib.h>
#include <string.h>	
#include <fcntl.h>
#include <errno.h>
#include <net/if.h>
#include <limits.h>
#ifdef OS_LINUX
#include <linux/if_tun.h>
#endif /* OS_LINUX */
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "base.h"
#include "log.h"



int tap_alloc(char *dev)
{
	int fd;
	struct ifreq ifr;
 
#ifdef OS_LINUX

	if ((fd = open("/dev/net/tun", O_RDWR)) < 0)
		log_pcexit("open");

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	StrCpy(ifr.ifr_name, dev, sizeof(char) * IFNAMSIZ);

	if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) {
		close(fd);
		log_pcrit("ioctl - TUNSETIFF");
		return -1;
	}

#else /* OS_LINUX */

	char devPath[PATH_MAX];
	snprintf(devPath, PATH_MAX, "/dev/%s", dev);

	if ((fd = open(devPath, O_RDWR)) < 0) {

		if ((fd = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0)
			log_pcexit("socket");
	
		memset(&ifr, 0, sizeof(ifr));
		StrCpy(ifr.ifr_name, dev, sizeof(char) * IFNAMSIZ-1);
	
		if (ioctl(fd, SIOCIFCREATE2, &ifr) == -1) {
			log_pcrit("ioctl - SIOCIFCREATE2");
			close(fd);
			return -1;
		}
	}

#endif /* OS_LINUX */

	return fd;
}



int tap_up(char *dev)
{
	int fd;
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));

	// Get Accessor
	if ( (fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		log_pcrit("socket");
		return -1;
	}

	// Get Interface information by name
	StrCpy(ifr.ifr_name, dev, sizeof(char) * IFNAMSIZ);
	if (ioctl(fd, SIOCGIFFLAGS, &ifr) != 0) {
		close(fd);
		log_pcrit("ioctl - SIOCGIFFLAGS");
		return -1;
	}

	// Set interface to up
	ifr.ifr_flags |= IFF_UP;
	if (ioctl(fd, SIOCSIFFLAGS, &ifr) != 0) {
		close(fd);
		log_pcrit("ioctl - SIOCSIFFLAGS");
		return -1;
	}

	close(fd);

	return 0;
}



int tap_down(char *dev)
{
	int fd;
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));

	if ( (fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		log_pcrit("socket");
		return -1;
	}

	StrCpy(ifr.ifr_name, dev, sizeof(char) * IFNAMSIZ);
	if (ioctl(fd, SIOCGIFFLAGS, &ifr) != 0) {
		close(fd);
		log_pcrit("ioctl - SIOCGIFFLAGS");
		return -1;
	}

	ifr.ifr_flags &= ! IFF_UP;
	if (ioctl(fd, SIOCSIFFLAGS, &ifr) != 0) {
		close(fd);
		log_pcrit("ioctl - SIOCSIFFLAGS");
		return -1;
	}

	close(fd);
	return 0;
}



#ifndef OS_LINUX

void tap_destroy(char *dev)
{
	int fd;
	struct ifreq ifr;

	if ( (fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		log_pcrit("socket");
		return;
	}

	StrCpy(ifr.ifr_name, dev, sizeof(char) * IFNAMSIZ);
	if (ioctl(fd, SIOCIFDESTROY, &ifr) != 0)
		log_pcrit("ioctl - SIOCGIFDESTROY");

	return;
}



#ifndef OS_DARWIN
int tap_rename(char *oldName, char *newName)
{
	int fd = 0;
	struct ifreq ifr;

	if ((fd = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0)
		log_pcexit("socket");

	memset(&ifr, 0, sizeof(ifr));
	StrCpy(ifr.ifr_name, oldName, sizeof(char) * IFNAMSIZ);
	ifr.ifr_data = newName;
	if (ioctl(fd, SIOCSIFNAME, &ifr) < 0) {
		log_perr("ioctl - SIOCSIFNAME");
		return -1;
	}

	close(fd);

	return 0;
}
#endif /* OS_DARWIN */
#endif /* OS_LINUX */



