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

	/* Open nox-socket connection 
	 * nox_sock: is the socket to nox
	 */
	int nox_sock;
	char nox_ip[] = "10.0.2.2";
	struct sockaddr_in *nox_host= (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in *));
	nox_host->sin_family = AF_INET;
	int tmpres = inet_pton(AF_INET, nox_ip, (void *)(&(nox_host->sin_addr.s_addr)));
	if( tmpres < 0) {
		perror("Can't set nox_host->sin_addr.s_addr");
		exit(1);
	}
	else if(tmpres == 0) {
		fprintf(stderr, "%s is not a valid IP address\n", nox_ip);
		exit(1);
	}
	nox_host->sin_port = htons(NOX_PORT);
	if((nox_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		perror("Can't create NOX TCP socket");
		exit(1);
	}
	if(connect(nox_sock, (struct sockaddr *)nox_host, sizeof(struct sockaddr)) < 0){
		perror("Could not connect to nox_host");
		exit(1);
	}
	cout << "Connected to NOX controller on IP address " << nox_ip << endl;

	/* Bond ioctl data */
	struct ifreq ifr;
	ifr.ifr_data = (void *) malloc(sizeof(struct ifslave));
	strcpy(ifr.ifr_name, bond_name);

	/* misc variables */
	int msg_size = sizeof(struct Hoolock_msg) + 1;
	struct Hoolock_msg *hmsg;
	int bytes_in, bytes_out;

	/* plumb socket variables */
	char act_slave[IFNAMSIZ], pas_slave[IFNAMSIZ];
	char buffer[256], plumb_cmd[50];
	char ip[] = "10.0.2.2";

	// Create socket
	int sock;
	struct sockaddr_in *remote = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in *));
	remote->sin_family = AF_INET;
	tmpres = inet_pton(AF_INET, ip, (void *)(&(remote->sin_addr.s_addr)));
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


		printf("Press <enter> to switch (or 'q' to quit): ");
		char a = getchar();
		if(a == 'q') {
			close(nox_sock);
			kill(iperf_pid, 9);
			close(oh_sock);
			printf("Smooth exit\n");
			return 0;
		}

		/* Fill ifr.ifr_slave with some junk value */
		//strcpy((char *)ifr.ifr_slave, slave);

		/* Make */

		//Tell nox
		/* Send MAKE_REQ to nox
		 * Wait for MAKE_ACK
		 */
		cout << "Sending MAKE_REQ to nox" << endl;
		char *make_msg = new char[msg_size];
		make_msg[msg_size - 1] = ';';
		hmsg = (struct Hoolock_msg *)make_msg;
		strncpy(hmsg->msg_id, "hoolock", 7);
		strncpy(hmsg->hostMac, hexmac, 12);
		hmsg->cmd = MAKE_REQ;
		bytes_out = 0;
		while(bytes_out < msg_size) {
			tmpres = write(nox_sock, make_msg+bytes_out, msg_size-bytes_out);
			if (tmpres < 0) 
				perror("ERROR writing to nox_sock");
			bytes_out += tmpres;
		}
		delete[] make_msg;

		char *make_ack_msg = new char[msg_size];
		bzero(make_ack_msg, msg_size);
		bytes_in = 0;
		while(bytes_in < msg_size) {
			tmpres = read(nox_sock, make_ack_msg+bytes_in, msg_size-bytes_in);
			if (tmpres < 0) 
				perror("ERROR reading from nox_sock");
			bytes_in += tmpres;
		}
		hmsg = (struct Hoolock_msg *)make_ack_msg;
		if(hmsg->cmd != MAKE_ACK) {
			cout << "Didn't get MAKE_ACK...something's wrong - exiting" << endl;
			return -1;
		}
		delete[] make_ack_msg;
		cout << "Received MAKE_ACK from nox" << endl;

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

		//Tell bond
		cout << "Calling BOND_MAKE" << endl;
		if ((ret = ioctl(skfd, SIOCBONDHOOLOCKMAKE, &ifr)) < 0) {
			printf("BondMake ioctl call failed : %d\n", ret);
			return -1;
		}
		
		/* Artificial sleep */
		sleep(1);

		/* Break */

		//Tell nox
		/* Send BREAK_REQ to nox
		 * Wait for BREAK_ACK
		 */
		cout << "Sending BREAK_REQ to nox" << endl;
		char *break_msg = new char[msg_size];
		break_msg[msg_size - 1] = ';';
		hmsg = (struct Hoolock_msg *)break_msg;
		strncpy(hmsg->msg_id, "hoolock", 7);
		strncpy(hmsg->hostMac, hexmac, 12);
		hmsg->cmd = BREAK_REQ;
		bytes_out = 0;
		while(bytes_out < msg_size) {
			tmpres = write(nox_sock, break_msg+bytes_out, msg_size-bytes_out);
			if (tmpres < 0) 
				perror("ERROR writing to nox_sock");
			bytes_out += tmpres;
		}
		delete[] break_msg;

		char *break_ack_msg = new char[msg_size];
		bzero(break_ack_msg, msg_size);
		bytes_in = 0;
		while(bytes_in < msg_size) {
			tmpres = read(nox_sock, break_ack_msg+bytes_in, msg_size-bytes_in);
			if (tmpres < 0) 
				perror("ERROR reading from nox_sock");
			bytes_in += tmpres;
		}
		hmsg = (struct Hoolock_msg *)break_ack_msg;
		if(hmsg->cmd != BREAK_ACK) {
			cout << "Didn't get BREAK_ACK...something's wrong - exiting" << endl;
			return -1;
		}
		delete[] break_ack_msg;
		cout << "Received BREAK_ACK from nox" << endl;


		//Tell bond
		cout << "Calling BOND_BREAK" << endl;
		if ((ret = ioctl(skfd, SIOCBONDHOOLOCKBREAK, &ifr)) < 0) {
			printf("BondBreak ioctl call failed : %d\n", ret);
			return -1;
		}

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
		plumb_cmd[6] = act_slave[3];
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

		/* Get input from user and kill iperf server */
		printf("Press <enter> to kill iperf server: ");
		getchar();
		kill(iperf_pid, 9);
		/* Close connection to output_handler */
		close(oh_sock);
	}

	return 0;

}
