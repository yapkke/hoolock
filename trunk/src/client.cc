#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <net/if_arp.h>
#include <linux/if_ether.h>
#include <linux/if_bonding.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <inttypes.h>

/* STL */
#include <iostream>
#include <string>

#include "message.h"

#define PORT 20000

using namespace std;

typedef unsigned long long u64;	/* hack, so we may include kernel's ethtool.h */
typedef __uint32_t u32;		/* ditto */
typedef __uint16_t u16;		/* ditto */
typedef __uint8_t u8;		/* ditto */

int main(int argc, char *argv[])
{
	int ret;

	/* Open a basic socket */
	int skfd = -1;
	char *bond_name;
	if(argc <= 1) {
		printf("Need bond name!\n");
		return -1;
	}
	bond_name = argv[1];
	if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Failed to open socket\n");
		return -1;
	}


	struct ifreq ifr;
	ifr.ifr_data = (void *) malloc(sizeof(struct ifslave));
	strcpy(ifr.ifr_name, bond_name);

	/* Get MAC address */
	string mac, hex_mac;
	long long int n_dec_mac = 0, dec_mac = 0;
	if ((ret = ioctl(skfd, SIOCGIFHWADDR, &ifr)) < 0) {
		printf("GetHWAdr ioctl call failed : %d\n", ret);
		return -1;
	}
	memcpy(&n_dec_mac, ifr.ifr_hwaddr.sa_data, 6);
	dec_mac = (long long)ntohl((long)n_dec_mac) << 16 | (long long)ntohs((short)(n_dec_mac >> 32));
	printf("Mac Address in hex : %llx\n", (long long int)dec_mac);

	while(1) {
		char act_slave[IFNAMSIZ], pas_slave[IFNAMSIZ];
		char buffer[256], plumb_cmd[50];
		char ip[] = "10.0.2.2";

		// Create socket
		int sock;
		struct sockaddr_in *remote = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in *));
		remote->sin_family = AF_INET;
		int tmpres = inet_pton(AF_INET, ip, (void *)(&(remote->sin_addr.s_addr)));
		if( tmpres < 0)  
		{
			perror("Can't set remote->sin_addr.s_addr");
			exit(1);
		}
		else if(tmpres == 0)
		{
			fprintf(stderr, "%s is not a valid IP address\n", ip);
			exit(1);
		}
		remote->sin_port = htons(PORT);

		/* Get active and passive slave names */
		if ((ret = ioctl(skfd, SIOCBONDHOOLOCKGETACTIVE, &ifr)) < 0) {
			printf("BondGetActive ioctl call failed : %d\n", ret);
			return -1;
		}
		strcpy(act_slave, ((struct ifslave *)ifr.ifr_data)->slave_name);
		printf("Active slave : %s\n", act_slave);

		if ((ret = ioctl(skfd, SIOCBONDHOOLOCKGETPASSIVE, &ifr)) < 0) {
			printf("BondGetPassive ioctl call failed : %d\n", ret);
			return -1;
		}
		strcpy(pas_slave, ((struct ifslave *)ifr.ifr_data)->slave_name);
		printf("Passive slave : %s\n", pas_slave);
		printf("-------------------------------------------------\n");


		printf("Hit enter to switch : ");
		getchar();

		/* Fill ifr.ifr_slave with some junk value */
		//strcpy((char *)ifr.ifr_slave, slave);

		/* Make */
		printf("Calling BondMake\n");

		// Tell plumber
		if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
			perror("Can't create TCP socket");
			exit(1);
		}
		if(connect(sock, (struct sockaddr *)remote, sizeof(struct sockaddr)) < 0){
			perror("Could not connect");
			exit(1);
		}
		strcpy(plumb_cmd, "make x");
		plumb_cmd[5] = pas_slave[3];
		tmpres = write(sock, plumb_cmd, strlen(plumb_cmd));
		if (tmpres < 0) 
			perror("ERROR writing to socket");
		bzero(buffer,256);
		tmpres = read(sock, buffer, 255);
		if (tmpres < 0) 
			perror("ERROR reading from socket");
		printf("%s\n",buffer);
		close(sock);

		if ((ret = ioctl(skfd, SIOCBONDHOOLOCKMAKE, &ifr)) < 0) {
			printf("BondMake ioctl call failed : %d\n", ret);
			return -1;
		}

		/* sleep */
		sleep(0.1);

		/* Break */
		printf("Calling BondBreak\n");

		// Tell plumber
		if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
			perror("Can't create TCP socket");
			exit(1);
		}
		if(connect(sock, (struct sockaddr *)remote, sizeof(struct sockaddr)) < 0){
			perror("Could not connect");
			exit(1);
		}
		strcpy(plumb_cmd, "break x");
		plumb_cmd[5] = act_slave[3];
		tmpres = write(sock, plumb_cmd, strlen(plumb_cmd));
		if (tmpres < 0) 
			perror("ERROR writing to socket");
		bzero(buffer,256);
		tmpres = read(sock, buffer, 255);
		if (tmpres < 0) 
			perror("ERROR reading from socket");
		printf("%s\n",buffer);
		close(sock);

		if ((ret = ioctl(skfd, SIOCBONDHOOLOCKBREAK, &ifr)) < 0) {
			printf("BondBreak ioctl call failed : %d\n", ret);
			return -1;
		}

		printf("-------------------------------------------------\n");
	}

	printf("Smooth exit\n");
	return 0;

}
