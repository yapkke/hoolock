#include <unistd.h>
#include <signal.h>
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
#define OH_PORT 21000
#define NOX_PORT 1304

using namespace std;

typedef unsigned long long u64;	/* hack, so we may include kernel's ethtool.h */
typedef __uint32_t u32;		/* ditto */
typedef __uint16_t u16;		/* ditto */
typedef __uint8_t u8;		/* ditto */

int main(int argc, char *argv[])
{

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

	/* Get MAC address 
	 * hexmac: stores the mac address at the end of this block
	 */
	struct ifreq mac_ifr;
	mac_ifr.ifr_data = (void *) malloc(sizeof(struct ifslave));
	strcpy(mac_ifr.ifr_name, bond_name);

	int ret;
	long long int n_dec_mac = 0, dec_mac = 0;
	char hexmac[13], tmpmac[13];
	if ((ret = ioctl(skfd, SIOCGIFHWADDR, &mac_ifr)) < 0) {
		printf("GetHWAdr ioctl call failed : %d\n", ret);
		return -1;
	}
	memcpy(&n_dec_mac, mac_ifr.ifr_hwaddr.sa_data, 6);
	dec_mac = (long long)ntohl((long)n_dec_mac) << 16 | (long long)ntohs((short)(n_dec_mac >> 32));
	sprintf(tmpmac, "%llx", (long long int)dec_mac);
	int i;
	for(i = 0; i < (int)(12 - strlen(tmpmac)); i++) {
		hexmac[i] = '0';
	}
	strcpy(hexmac + i, tmpmac);

	cout << "Starting client program on host : " << hexmac << endl;

	/* Bond ioctl data */
	struct ifreq ifr;
	ifr.ifr_data = (void *) malloc(sizeof(struct ifslave));
	strcpy(ifr.ifr_name, bond_name);

	/* misc variables */
	/*int msg_size = sizeof(struct Hoolock_msg) + 1;
	struct Hoolock_msg *hmsg;
	int bytes_in, bytes_out;*/

	/* plumb socket variables */
	char act_slave[IFNAMSIZ];
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

	/* Get active slave */
	if ((ret = ioctl(skfd, SIOCBONDHOOLOCKGETACTIVE, &ifr)) < 0) {
		printf("BondGetActive ioctl call failed : %d\n", ret);
		return -1;
	}
	strcpy(act_slave, ((struct ifslave *)ifr.ifr_data)->slave_name);

	/* Create the first link */
	if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		perror("Can't create TCP socket");
		exit(1);
	}
	if(connect(sock, (struct sockaddr *)remote, sizeof(struct sockaddr)) < 0){
		perror("Could not connect");
		exit(1);
	}
	strcpy(plumb_cmd, "make x");
	plumb_cmd[5] = act_slave[3];
	cout << "Sending to plumber : " << plumb_cmd << endl;
	tmpres = write(sock, plumb_cmd, strlen(plumb_cmd));
	if (tmpres < 0) 
		perror("ERROR writing to socket");
	bzero(buffer,256);
	tmpres = read(sock, buffer, 255);
	if (tmpres < 0) 
		perror("ERROR reading from socket");
	cout << "Received from plumber : " << buffer << endl;
	close(sock);
	printf("-------------------------------------------------\n");

	/* start background ping */
	system("ping -q -i 0.2 192.168.0.2 &");

	/* Create output_handler socket */
	int iperf_pid = -1;
	int oh_sock;
	struct sockaddr_in *oh_in = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in *));
	oh_in->sin_family = AF_INET;
	tmpres = inet_pton(AF_INET, ip, (void *)(&(oh_in->sin_addr.s_addr)));
	if( tmpres < 0)  
	{
		perror("Can't set oh_in->sin_addr.s_addr");
		exit(1);
	}
	else if(tmpres == 0)
	{
		fprintf(stderr, "%s is not a valid IP address\n", ip);
		exit(1);
	}
	oh_in->sin_port = htons(OH_PORT);

	while(1) {
		/* Get input from user and start iperf server */
		printf("Press <enter> to start iperf server: ");
		getchar();
		/* Connect to output_handler */
		if((oh_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
			perror("Can't create TCP socket to OH");
			exit(1);
		}
		if(connect(oh_sock, (struct sockaddr *)oh_in, sizeof(struct sockaddr)) < 0){
			perror("Could not connect to OH");
			exit(1);
		}

		/* Spawn iperf server */
		iperf_pid = fork();
		if(iperf_pid == 0) {
			// Child process - iperf server
			printf("Starting iperf UDP server\n");
			dup2(oh_sock, 1);
			execl("/usr/bin/iperf", "/usr/bin/iperf", "-i", "2", "-u", "-s", (char*)0);
			return 0;
		}

		/* Get input from user and kill iperf server */
		printf("Press <enter> to kill iperf server: ");
		getchar();
		kill(iperf_pid, 9);
		/* Close connection to output_handler */
		close(oh_sock);
	}

	return 0;

}
