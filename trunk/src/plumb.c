/* A simple server using TCP
 * Makes and breaks vde pipes
   The port number is passed as an argument */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 20000

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
     int sockfd, portno, clilen;
     struct sockaddr_in serv_addr, cli_addr;
     pid_t eth0_pid = -1, eth1_pid = -1;
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) {
        error("ERROR opening socket");
     }
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = PORT;
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);

     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
              error("ERROR on binding");

     listen(sockfd,5);
     clilen = sizeof(cli_addr);
     printf("Listening on port number %d....\n", portno);

     while (1) {
	     char buffer[256];
	     int newsockfd, n;

	     newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	     if (newsockfd < 0) 
		     error("ERROR on accept");

	     bzero(buffer,256);

	     // Read the command
	     n = read(newsockfd, buffer, 255);
	     if (n < 0) 
		     error("ERROR reading from socket");

	     // Based on the command fork a process
	     if(strncmp(buffer, "make", 4) == 0) {
		     /* 
		      * <s>If it's a make, both the pipes must be connected</s>
		      * It's the caller's responsibility to ensure at least
		      * on pipe is connected
		      */
		     if(buffer[4] == '0') {
			     if(eth0_pid < 0) {
				     eth0_pid = fork();
				     if(eth0_pid == 0) {
					     // child process 
					     printf("Make: mobile0SW <====> openflow0SW\n");
					     execl("/usr/bin/dpipe", "/usr/bin/dpipe", "vde_plug", "/home/nikhilh/nox/vm/Hoolock/vde/ctlmobile0SW", "=", "wirefilter", "=", "vde_plug", "/home/nikhilh/nox/vm/Hoolock/vde/ctlopenflow0SW", (char*)0);
					     return;
				     }
			     }
		     }
		     else if(buffer[4] == '1') {
			     if(eth1_pid < 0) {
				     eth1_pid = fork();
				     if(eth1_pid == 0) {
					     // child process
					     printf("Make: mobile1SW <====> openflow1SW\n");
					     execl("/usr/bin/dpipe", "/usr/bin/dpipe", "vde_plug", "/home/nikhilh/nox/vm/Hoolock/vde/ctlmobile1SW", "=", "wirefilter", "=", "vde_plug", "/home/nikhilh/nox/vm/Hoolock/vde/ctlopenflow1SW", (char*)0);
					     return;
				     }
			     }
		     }
	     }
	     else if(strncmp(buffer, "break", 5) == 0) {
		     if(buffer[5] == '0') {
			     printf("Break: mobile0SW <====> openflow0SW\n");
			     if(eth0_pid > 0) {
				     kill(eth0_pid, 9);
			     }
			     eth0_pid = -1;
		     }
		     else if(buffer[5] == '1') {
			     printf("Break: mobile1SW <====> openflow1SW\n");
			     if(eth1_pid > 0) {
				     kill(eth1_pid, 9);
			     }
			     eth1_pid = -1;
		     }
		     else {
			     printf("Bad break command! Shouldn't be seeing this.\n");
		     }
	     }
	     else {
		     printf("Bad command! Shouldn't be seeing this.\n");
	     }

	     n = write(newsockfd,"Plumber executed the command!", 29);
	     if (n < 0) 
		     error("ERROR writing to socket");
	     close(newsockfd);
     }
     return 0; 
}
