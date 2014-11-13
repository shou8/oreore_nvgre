#include <stdlib.h>
#include <string.h>	
#include <fcntl.h>
#include <errno.h>
#include <net/if.h>
#ifdef OS_LINUX
#include <linux/if_tun.h>
#else
#include <net/if_tap.h>
#endif
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "base.h"
#include "log.h"



int tap_alloc(char *dev) {

	int fd;
	struct ifreq ifr;
 
#ifdef OS_LINUX
	if ((fd = open("/dev/net/tun", O_RDWR)) < 0)
		log_pcexit("open");

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	strncpy(ifr.ifr_name, dev, IFNAMSIZ-1);

	if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) {
		close(fd);
		log_pcrit("ioctl - TUNSETIFF");
		return -1;
	}
#else
	if ((fd = open(dev, O_RDWR)) < 0) {
		log_pcrit("open");
		if ((fd = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0)
			log_pcexit("socket");

		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, dev, IFNAMSIZ-1);

		if (ioctl(fd, SIOCIFCREATE2, &ifr) == -1) {
			log_pcrit("ioctl - SIOCIFCREATE2");
			close(fd);
			return -1;
		}
	}
#endif

	return fd;
}



int tap_up(char *dev) {

	int fd;
	struct ifreq ifr;

	// Get Accessor
	if ( (fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		log_pcrit("socket");
		return -1;
	}

	// Get Interface information by name
	strncpy(ifr.ifr_name, dev, IFNAMSIZ-1);
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



void tap_destroy(char *dev) {

	int fd;
	struct ifreq ifr;

	if ( (fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		log_pcrit("socket");
		return;
	}

	strncpy(ifr.ifr_name, dev, IFNAMSIZ-1);
	if (ioctl(fd, SIOCIFDESTROY, &ifr) != 0)
		log_pcrit("ioctl - SIOCGIFDESTROY");

	return;
}
