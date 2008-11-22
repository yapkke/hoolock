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
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) {
        error("ERROR opening socket");
     }
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
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
		     if(buffer[5] == '0') {
			     if((eth0_pid = fork()) == 0) {
				     /* child process */
				     close(newsockfd);
				     close(sockfd);
				     printf("Make: mobile0SW <====> openflow0SW\n");
				     system("dpipe vde_plug /home/nikhilh/vm/Hoolock/vde/ctlmobile0SW = wirefilter = vde_plug /home/nikhilh/vm/Hoolock/vde/ctlopenflow0SW");
			     }
		     }
		     else if(buffer[5] == '1') {
			     if((eth1_pid = fork()) == 0) {
				     /* child process */
				     close(newsockfd);
				     close(sockfd);
				     printf("Make: mobile1SW <====> openflow1SW\n");
				     system("dpipe vde_plug /home/nikhilh/vm/Hoolock/vde/ctlmobile1SW = wirefilter = vde_plug /home/nikhilh/vm/Hoolock/vde/ctlopenflow1SW");
			     }
		     }
		     else {
			     printf("Bad make command! Shouldn't be seeing this.\n");
		     }
	     }
	     else if(strncmp(buffer, "break", 4) == 0) {
		     if(buffer[5] == '0') {
			     printf("Break: mobile0SW <====> openflow0SW\n");
			     kill(eth0_pid, 9);
			     eth0_pid = -1;
		     }
		     else if(buffer[5] == '1') {
			     printf("Break: mobile1SW <====> openflow1SW\n");
			     kill(eth1_pid, 9);
			     eth1_pid = -1;
		     }
		     else {
			     printf("Bad break command! Shouldn't be seeing this.\n");
		     }
	     }

	     n = write(newsockfd,"Plumber executed the command!", 29);
	     if (n < 0) 
		     error("ERROR writing to socket");
	     close(newsockfd);
     }
     return 0; 
}
