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

#define DEFAULT_BOND_NAME "bond0"
#define DEFAULT_SLEEP_TIME 5000 // fed to usleep()

#define NUM_INTERFACES 2
#define NUM_ESSIDS 2

char interfaces[NUM_INTERFACES][64] = {"ath0", "wlan0"};
char essids[NUM_ESSIDS][64] = {"cleanslatewifi1", "cleanslatewifi2"};

/* ----------------------------------------------------------------------------- */

void set_active_slave_command(char* command, char* bond_name, char* interface){
	sprintf(command, "ifenslave -c %s %s", bond_name, interface);
}

void set_association_command(char* command, char* interface, char* essid){
	sprintf(command, "iwconfig %s essid %s ap auto", interface, essid);
}

void set_deassociation_command(char* command, char* interface){
	sprintf(command, "iwconfig %s essid \"xxxx\" ap off", interface);
}

void execute_command(char* command){
	fprintf(stderr, "Executing: %s\n", command);	
	system(command);
}

void send_ioctl_command(int skfd, int ioctl_command, struct ifreq *ifr){
	if (ioctl(skfd, ioctl_command, ifr) < 0) {
		printf("ioctl call failed :(\n");
		exit(-1);
	}
}

int main(int argc, char *argv[])
{
	char bond_name[64] = DEFAULT_BOND_NAME; // Interface name
	int sleep_time = DEFAULT_SLEEP_TIME;  // Sleep duration in usec
	char command[128];

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

	// Variables for switching
	int cur_interface, cur_essid;
	int next_interface, next_essid;

	/* Initial bonding tx interface */
	cur_interface = 0;
	set_active_slave_comand(command, bond_name, cur_interface);
	execute_command(command);

	/* Initial AP association */
	cur_essid = 0;
	set_association_command(command, interfaces[cur_interface], essids[cur_essid]);
	execute_command(command);

	/* Switching */
	while(1){
		char menu_selection[16];
		printf("=== Hoolock client program ===\n");
		printf("1) Switch AP\n");
		printf("q) Quit\n");

		printf("\nEnter option:\n");
		scanf("%s", menu_selection);

		if(menu_selection[0] == 'q') break;
		switch(menu_selection[0]){
			case '1':
				printf("Starting switching, sleep time = %d usec", sleep_time);
				next_interface = (cur_interface + 1) % NUM_INTERFACES;
				next_essid = (cur_essid + 1) % NUM_ESSIDS;
				
				// Associate with next essid
				set_association_command(command, interfaces[next_interface], essids[next_essid]);
				execute_command(command);

				// Send "make" ioctl command to bonding driver
				send_ioctl_command(skfd, SIOCBONDHOOLOCKMAKE, &ifr);

				// Sleep
				usleep(sleep_time);

				// Send "break" ioctl command to bonding driver
				send_ioctl_command(skfd, SIOCBONDHOOLOCKBREAK, &ifr);	

				// Deassociate with original essid
				set_deassociation_command(command, interfaces[cur_interface]);
				execute_command(command);
				
				cur_interface = next_interface;
				cur_essid = next_essid;
				break;
			default:
				fprintf(stderr, "Error! Invalid option.\n");
		}
	}

	if (ioctl(skfd, SIOCBONDHOOLOCKTEST, &ifr) < 0) {
		printf("ioctl call failed :(\n");
		return -1;
	}
	
	printf("Done with ioctl-test\n");
	return 0;

}
