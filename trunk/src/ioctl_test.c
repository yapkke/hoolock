#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <net/if_arp.h>
#include <linux/if_ether.h>
#include <linux/if_bonding.h>
#include <linux/sockios.h>

typedef unsigned long long u64;	/* hack, so we may include kernel's ethtool.h */
typedef __uint32_t u32;		/* ditto */
typedef __uint16_t u16;		/* ditto */
typedef __uint8_t u8;		/* ditto */
#include <linux/ethtool.h>

int main(int argc, char *argv[])
{
	/* Open a basic socket */
	int skfd = -1;
	char *bond_name;
	if(argc < 1) {
		printf("Need bond name!\n");
		return -1;
	}
	bond_name = argv[1];
	if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Failed to open socket\n");
		return -1;
	}

	struct ifreq ifr;
	strcpy(ifr.ifr_name, bond_name);
	if (ioctl(skfd, SIOCBONDTEST, &ifr) < 0) {
		printf("IOCTL call failed :(\n");
		return -1;
	}
	
	printf("Done with ioctl-test\n");
	return 0;

}
