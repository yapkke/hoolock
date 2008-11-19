#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
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
	if(argc <= 1) {
		printf("Need bond name!\n");
		return -1;
	}
	printf("Getting argv1\n");
	bond_name = argv[1];
	printf("Opening socket\n");
	if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Failed to open socket\n");
		return -1;
	}

	printf("Copying bond name to ifr\n");
	struct ifreq ifr;
	strcpy(ifr.ifr_name, bond_name);

	while(1) {
		char cmd[50];
		printf("Entering while loop\n");

		/* Get active and passive slave names */
		if (ioctl(skfd, SIOCBONDHOOLOCKGETACTIVE, &ifr) < 0) {
			printf("BondGetActive ioctl call failed :(\n");
			return -1;
		}
		printf("Active slave : %s\n", ((struct ifslave *)ifr.ifr_data)->slave_name);
		if (ioctl(skfd, SIOCBONDHOOLOCKGETPASSIVE, &ifr) < 0) {
			printf("BondGetPassive ioctl call failed :(\n");
			return -1;
		}
		printf("Passive slave : %s\n", ((struct ifslave *)ifr.ifr_data)->slave_name);
		printf("-------------------------------------------------\n");

		printf("Command : ");
		scanf("%s", cmd);
		if(strcmp(cmd, "q") == 0)
			break;

		/* Make */
		printf("Calling BondMake\n");
		if (ioctl(skfd, SIOCBONDHOOLOCKMAKE, &ifr) < 0) {
			printf("BondMake ioctl call failed :(\n");
			return -1;
		}

		/* sleep */
		sleep(0.1);

		/* Break */
		printf("Calling BondBreak\n");
		if (ioctl(skfd, SIOCBONDHOOLOCKBREAK, &ifr) < 0) {
			printf("BondBreak ioctl call failed :(\n");
			return -1;
		}

		printf("-------------------------------------------------\n");
	}

	printf("Smooth exit\n");
	return 0;

}
