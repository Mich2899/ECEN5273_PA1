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
#include <sys/stat.h>
#include <fcntl.h>

#define BUFSIZE 1024
#define FILE2KB 2*1000*8

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
  char buf1[BUFSIZE];

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
    bzero(file, BUFSIZE);
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


    if((strncmp(buf, "exit", 4)) == 0){
          /* 
          * sendto: echo the input back to the client 
          */
          char msg[]={"Exiting...\n"};
          n = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *) &clientaddr, clientlen);
          if (n < 0) 
            error("ERROR in sendto");

        //server side
        printf("Exiting...\n");
        close(sockfd);
        exit(0);
    }

    else if((strncmp(buf, "delete", 6)) == 0){
        printf("filename: %s\n", buf+6);
        char deletefile[BUFSIZE];

        bzero(deletefile, BUFSIZE);
        bzero(file, BUFSIZE);

        strncpy(file, "rm ", 3);
        strncpy(file+3, buf+6, BUFSIZE);
        strncpy(deletefile, buf+6, BUFSIZE);
        printf("file to delete:%s\n", deletefile);
        system(file);

          //ack
          n = sendto(sockfd, deletefile, strlen(deletefile), 0, 
              (struct sockaddr *) &clientaddr, clientlen);
          if (n < 0) 
            error("ERROR in sendto");

        bzero(file, BUFSIZE);
        bzero(deletefile, BUFSIZE);
    }

    else if((strncmp(buf, "ls", 2)) == 0){
        char buf1[BUFSIZE];

        bzero(buf1, BUFSIZE);

        FILE* filep;
        filep = popen("ls *","r");
        size_t readret = fread(buf1, 1, BUFSIZE, filep);
        printf("Output ls: %s, readret: %ld\n", buf1, readret);

          //ack
          n = sendto(sockfd, buf1, strlen(buf1), 0, 
              (struct sockaddr *) &clientaddr, clientlen);
          if (n < 0) 
            error("ERROR in sendto");

        bzero(buf1, BUFSIZE);
        pclose(filep);
    }

    else if((strncmp(buf, "get ", 4)) == 0){

        //Null pointer
        char* source=NULL;
        struct stat* finfo;
        long filesize;     

        strncpy(file, buf+4, strlen(buf));

        for(int i=0;i<=strlen(file);i++){
          if(file[i]=='\n'){
            file[i]='\0';
          }
        }

        printf("filename:%s\n", file);
        //file to read from
        int filefd =open(file, O_CREAT| O_RDWR, 0777);

        if(filefd==-1){
          printf("filefd:%d\n", filefd);
          error("Error in opening the file!\n");
        }

        //fstat to get file data
        if(fstat(filefd, finfo)){
          error("File does not exist!\n");
        }

        //store the filesize
        filesize = (long)finfo->st_size;

        //clear buffer
        bzero(buf, BUFSIZE);
        *source = filesize;

        ssize_t newLen = read(filefd, buf1, filesize);
        printf("Read %ld bytes from file\n", newLen);

        close(filefd);

        //send filesize
        n = sendto(sockfd, source, strlen(source), 0, (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0) 
          error("ERROR in sendto");  

        //receive ack
        n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &clientlen);
        if (n < 1)
          error("ERROR in recvfrom");      

        //Make a loop here to send chunks of file in size of 2KB and recieve ack after every send
        for(long i=0; i<=filesize; i+= FILE2KB*sizeof(char)){

            //clear both the buffer to receive ack
            bzero(buf, BUFSIZE);

            //send filesize
            n = sendto(sockfd, buf1+i, FILE2KB*sizeof(char), 0, (struct sockaddr *) &clientaddr, clientlen);
            if (n < 0) 
              error("ERROR in sendto");  

            //receive ack
            n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &clientlen);
            if (n < 1)
              error("ACK not receieved!!"); 
            printf("ACK: %s\n", buf);

      }

    } 

    else{
        /* 
        * sendto: echo the input back to the client 
        */
        n = sendto(sockfd, buf, strlen(buf), 0, 
            (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0) 
          error("ERROR in sendto");

        }
  }    
}
