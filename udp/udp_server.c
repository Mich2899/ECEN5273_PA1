/* 
 * udpserver.c - A simple UDP echo server 
 * usage: udpserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 1024

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char **argv) {
  int sockfd; /* socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */
  char file[BUFSIZE];

  /* 
   * check command line arguments 
   */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /* 
   * socket: create the parent socket 
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  /* 
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  while (1) {

    /*
     * recvfrom: receive a UDP datagram from a client
     */
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0,
		 (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      error("ERROR in recvfrom");

    /* 
     * gethostbyaddr: determine who sent the datagram
     */
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
			  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("server received datagram from %s (%s)\n", 
	   hostp->h_name, hostaddrp);
    printf("server received %ld/%d bytes: %s\n", strlen(buf), n, buf);
    
    /* 
     * sendto: echo the input back to the client 
     */
    n = sendto(sockfd, buf, strlen(buf), 0, 
	       (struct sockaddr *) &clientaddr, clientlen);
    if (n < 0) 
      error("ERROR in sendto");


    if((strncmp(buf, "exit", 4)) == 0){
      printf("Exiting...\n");
      close(sockfd);
      exit(0);
    }

    if((strncmp(buf, "delete", 6)) == 0){
      strncpy(file, "rm", 2);
      strncpy(file+2, buf+6, BUFSIZE);
      system(file);
      bzero(file, BUFSIZE);
    }

    if((strncmp(buf, "get", 3)) == 0){

      //Null pointer
      char* source=NULL;
      printf("opening the file!\n");
      //file to read from
      FILE* filep =popen(buf+3, "r");

      if(filep!= NULL){
        printf("Seeking the end!\n");
        //Find the EOF
        if(fseek(filep,0L, SEEK_END) == 0){
          //get the size of the file
          long sourcesize = ftell(filep);

          //store the size of file in a buffer and send that first
          file[0] = (char)sourcesize;
          printf("Sending the size of file!\n");
          n = sendto(sockfd, file, 1, 0, (struct sockaddr *) &clientaddr, clientlen);
          if (n < 0) 
            error("ERROR in sendto");                
          
          if(sourcesize == -1){
            error("Error in filesize!!\n");
          }
          
          //malloc required size for the buffer
          source = malloc(sizeof(char) * sourcesize);

          printf("Starting to copy from zero!\n");
          //Go back to start to copy the contents
          if(fseek(filep,0L, SEEK_SET)!=0){
            error("Error in fseek!!\n");
          }

          //read the file contents
          size_t newLen = fread(source, sizeof(char), sourcesize, filep);

          //apend null after eof
          source[newLen++] = '\0';

      printf("Sending the actual file!\n");
      //send the actual file contents
      n = sendto(sockfd, source, newLen, 0, (struct sockaddr *) &clientaddr, clientlen);
      if (n < 0) 
        error("ERROR in sendto");      
     
        }
      } 
    }    
  }
}
