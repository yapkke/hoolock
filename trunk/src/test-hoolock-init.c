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
#define DEFAULT_SLEEP_TIME 10000 // fed to usleep()

#define NUM_INTERFACES 2
#define NUM_ESSIDS 2

#define bond_mac_addr "00:1c:f0:ed:98:5a"

char interfaces[NUM_INTERFACES][64] = {"ath0", "wlan0"};
char essids[NUM_ESSIDS][64] = {"cleanslatewifi1", "cleanslatewifi2"};

/* ----------------------------------------------------------------------------- */

void set_active_slave_command(char* command, char* bond_name, char* interface){
	sprintf(command, "ifenslave -c %s %s", bond_name, interface);
}

void set_association_command(char* command, char* interface, char* essid){
	sprintf(command, "iwconfig %s essid %s", interface, essid);
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
	int interface_index;

	if(argc > 1) {
		strcpy(bond_name, argv[1]);
	}
	if(argc > 2){
		interface_index = atoi(argv[2]);
	}
	else interface_index = 0;

	/* Make sure bonding is using the required mac address (ath0 in cleanslate testbed) */
	/*
	sprintf(command, "ifconfig bond down");
	execute_command(command);
	sprintf(command, "ifconfig bond0 hw ether %s", bond_mac_addr);
	execute_command(command);
	sprintf(command, "ifconfig bond up");
	execute_command(command);
	sprintf(command, "ifconfig bond0");
	execute_command(command);
	*/

	/* Initial AP association */
	int cur_interface = interface_index;
	int cur_essid = interface_index;
	printf("Initialization:\nAssociating %s with %s\n", interfaces[cur_interface], essids[cur_essid]);
	set_association_command(command, interfaces[cur_interface], essids[cur_essid]);
	execute_command(command);
	
	sleep(1);

	/* Initial bonding tx interface */
	printf("Setting active slave to %s\n", interfaces[cur_interface]);
	set_active_slave_command(command, bond_name, interfaces[cur_interface]);
	execute_command(command);
	return 0;
}

