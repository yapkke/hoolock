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

//#define SIOCBONDHOOLOCKTEST 0x899a

#define DEFAULT_BOND_NAME "bond0"
#define DEFAULT_SLEEP_TIME 5000 // fed to usleep()

int main(int argc, char *argv[])
{
	char bond_name[64] = DEFAULT_BOND_NAME; // Interface name
	int sleep_time = DEFAULT_SLEEP_TIME;  // Sleep duration in usec

	if(argc > 1) {
		strcpy(bond_name, argv[1]);
	}

	if(argc > 2){
		sleep_time = atoi(argv[2]);
	}
	if(sleep_time <= 0) sleep_time = DEFAULT_SLEEP_TIME;

	/* Open a basic socket for communicating with bonding driver */
	int skfd = -1;
	if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Failed to open socket\n");
		return -1;
	}
	
	struct ifreq ifr;
	strcpy(ifr.ifr_name, bond_name);

	
	if (ioctl(skfd, SIOCBONDHOOLOCKTEST, &ifr) < 0) {
		printf("ioctl call failed :(\n");
		return -1;
	}
	
	printf("Done with ioctl-test\n");
	return 0;

}
