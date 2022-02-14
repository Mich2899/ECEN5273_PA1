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
#include <errno.h>

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
  int packet=0;

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

    char ack[]={"ACK"};
    /*
     * recvfrom: receive a UDP datagram from a client
     */
    bzero(buf, BUFSIZE);
    bzero(file, BUFSIZE);

    n = recvfrom(sockfd, buf, BUFSIZE, 0,
		 (struct sockaddr *) &clientaddr, (socklen_t * restrict)&clientlen);
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
        //printf("filename: %s\n", buf+6);
        char deletefile[BUFSIZE];

        bzero(deletefile, BUFSIZE);
        bzero(file, BUFSIZE);

        strncpy(file, "rm \0", 4);
        strncpy(file+3, buf+6, BUFSIZE);
        strncpy(deletefile, buf+6, BUFSIZE);
        //printf("file to delete:%s\n", deletefile);
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

        bzero(buf1, BUFSIZE);

        FILE* filep;
        filep = popen("ls *","r");
        size_t readret = fread(buf1, 1, BUFSIZE, filep);
        if(readret<0){
          error("Error in fread!\n");
        }
        printf("Output ls: %s\n", buf1);

          //ack
          n = sendto(sockfd, buf1, strlen(buf1), 0, 
              (struct sockaddr *) &clientaddr, clientlen);
          if (n < 0) 
            error("ERROR in sendto");

        bzero(buf1, BUFSIZE);
        pclose(filep);
    }

    else if((strncmp(buf, "get ", 4)) == 0){

      // char msg[]={"ACK...\n"};

      // n = sendto(sockfd, msg, strlen(msg), 0, 
      //     (struct sockaddr *) &clientaddr, clientlen);
      // if (n < 0) 
      //   error("ERROR in sendto");         

      
        struct stat finfo;
        //off_t filesize;     

        strncpy(file, buf+4, strlen(buf)-4);

        for(int i=0;i<=strlen(file);i++){
          if(file[i]=='\n'){
            file[i]='\0';
          }
        }

        printf("filename:%s\n", file);
        //file to read from
        int filefd = open(file, O_RDWR, 0777);

        //printf("filefd: %d\n",filefd);

        if(filefd==-1){
          error("Error in opening the file!\n");
        }

        int fstatret=fstat(filefd, &finfo);
        //fstat to get file data
        if(fstatret<0){
          error("File does not exist!\n");
        }

        //filesize = finfo.st_size;

        /*send the filesize*/
        n = sendto(sockfd, &finfo.st_size, sizeof(off_t), 0, 
            (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0) 
          error("ERROR in sendto");       

        n = recvfrom(sockfd, buf, BUFSIZE, 0,
        (struct sockaddr *) &clientaddr, (socklen_t * restrict)&clientlen);
        if (n < 0)
          error("ERROR in recvfrom");
        printf("Ack msg:%s\n", buf);
             
        char readbuf[2000];
        int readret=0;
        bzero(readbuf, 2000);
        bzero(buf,BUFSIZE);

          //Make a loop here to send chunks of file in size of 2KB and recieve ack after every send

            while((readret = read(filefd, readbuf, 2000))>0){        
               
              // printf("readret:%d\n", readret);
              //send filesize 
                  n = sendto(sockfd, readbuf, readret, 0, 
                (struct sockaddr *) &clientaddr, clientlen);
                  if (n < 0) 
                    error("ERROR in sendto"); 
                  packet++;
                  printf("Packet number:%d\n", packet);

                  //receive ack
                  n = recvfrom(sockfd, buf, BUFSIZE, 0,
                  (struct sockaddr *) &clientaddr, (socklen_t * restrict)&clientlen);
                  if (n < 0)
                    error("ACK not receieved!!"); 
                  printf("ACK: %s\n", buf);

                  bzero(readbuf,2000);
                  bzero(buf, BUFSIZE);                  
            }
            packet=0;
      close(filefd);
    } 

    else if((strncmp(buf, "put ", 4)) == 0){

          int filesize;   
          int writeret;   

          bzero(file, BUFSIZE);

          // /*ACK*/
          // n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, &serverlen);
          // if (n < 0) 
          //   error("ERROR in recvfrom");
          // printf("msg: %s\n", buf);  

              //printf("Receive the size of the file!\n");
              //Receive the size of the file
              n = recvfrom(sockfd, &filesize, sizeof(int), 0,
              (struct sockaddr *) &clientaddr, (socklen_t * restrict)&clientlen);
              if (n < 0)
                error("ERROR in recvfrom");                  

              /*Acknowledgement*/
              n = sendto(sockfd, ack, strlen(ack), 0, 
              (struct sockaddr *)&clientaddr, clientlen);
              if (n < 0) 
                error("ERROR in sendto");

          /*Copy the filename*/
          strncpy(file, buf+4, strlen(buf));
          
          /*append null to avoid newline*/
          for(int i=0;i<strlen(file);i++){
            if(file[i]=='\n'){
              file[i]='\0';
            }
          }

          printf("filename:%s filesize:%d\n",file, filesize);

              printf("Opening the file!\n");
              int filefd = open(file, O_CREAT| O_RDWR| O_TRUNC, 0777);
              if(filefd==-1){
                error("Error in file open!\n");
              }

              char newbuf[2000];

              //Make a loop here to receive chunks of file in size of 2KB and recieve ack after every send
              for(int i=0; i<=filesize;i+=2000){

                bzero(newbuf, 2000);

                /*receive data*/
                n = recvfrom(sockfd, newbuf, 2000, 0,
                 (struct sockaddr *)&clientaddr, (socklen_t * restrict)&clientlen);
                if (n < 0) 
                  error("ERROR in recvfrom");
                
                /*write into the file*/
                writeret = write(filefd, newbuf, n);
                printf("%d bytes written to file\n", writeret);

                /*Acknowledgement*/
                n = sendto(sockfd, ack, strlen(ack), 0,
                 (struct sockaddr *)&clientaddr, clientlen);
                if (n < 0) 
                  error("ERROR in sendto");
              }
              close(filefd);

          printf("File received!!!\n");
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
