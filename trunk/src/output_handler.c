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

#define PORT 21000

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
     int sockfd, portno, clilen;
     struct sockaddr_in serv_addr, cli_addr;
     int exp_cnt = 0;
     if(argc < 3) {
	     error("Usage: output_handler dir_name start_seq");
     }
     exp_cnt = atoi(argv[2]);
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
	     char buffer[256], fname[50];
	     int newsockfd, n;
	     FILE *fp;

	     sprintf(fname, "%s/out-%d.txt", argv[1], exp_cnt);

	     newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	     if (newsockfd < 0) 
		     error("ERROR on accept");
	     printf("Received connection\n");

	     printf("Opening file %s for output\n", fname);
	     fp = fopen(fname, "w");


	     do {
		     // Read the command
		     bzero(buffer,256);
		     n = read(newsockfd, buffer, 255);
		     if (n < 0) 
			     error("ERROR reading from socket");
		     fprintf(fp, "%s", buffer);
	     }while(n > 0);

	     fclose(fp);
	     close(newsockfd);
	     printf("Closing file %s\n", fname);
	     exp_cnt++;
     }
     return 0; 
}
