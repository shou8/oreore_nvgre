#include <stdlib.h>
#include <string.h>	
#include <fcntl.h>
#include <errno.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "base.h"
#include "log.h"



int tap_alloc(char *dev) {

	int fd;
	struct ifreq ifr;
 
	if ( (fd = open("/dev/net/tun", O_RDWR)) < 0)
		log_pcexit("open");

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	strncpy(ifr.ifr_name, dev, IFNAMSIZ-1);

	if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) {
		close(fd);
		log_pcrit("TUNSETIFF");
		return -1;
	}

	return fd;
}



int tap_up(char *dev) {

	int fd;
	struct ifreq ifr;

	if ( (fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		log_pcrit("socket");
		return -1;
	}

	strncpy(ifr.ifr_name, dev, IFNAMSIZ-1);
	if (ioctl(fd, SIOCGIFFLAGS, &ifr) != 0) {
		log_pcrit("ioctl");
		return -1;
	}

	ifr.ifr_flags |= IFF_UP;
	if (ioctl(fd, SIOCSIFFLAGS, &ifr) != 0) {
		close(fd);
		log_pcrit("ioctl");
		return -1;
	}

	close(fd);

	return 0;
}

