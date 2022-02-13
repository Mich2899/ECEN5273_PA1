/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define BUFSIZE 1024
#define FILE2KB 2*1000*8

int sockfd, n;
char buf[BUFSIZE];
int serverlen;
struct sockaddr_in serveraddr;  
char savefile[BUFSIZE];

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

/*
 * get - wrapper for get
 */
void get(char* filename){
  int writeret;
  char newbuf[BUFSIZE];
  long pointer_size;

  printf("Opening the file!\n");
  int filefd = open(filename, O_CREAT| O_RDWR, 0777);
  if(filefd==-1){
    error("Error in file open!\n");
  }

  bzero(buf, BUFSIZE);

  printf("Receive the size of the file!\n");
  n = recvfrom(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serveraddr, &serverlen);
    if (n < 0) 
      error("ERROR in recvfrom");

  //save the filesize
  long filesize = (long)*buf;

    buf[0] = 'A';

  /*Acknowledgement*/
  n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
  if (n < 0) 
    error("ERROR in sendto");

  //Make a loop here to send chunks of file in size of 2KB and recieve ack after every send
  for(long i=0; i<filesize; i+=FILE2KB*sizeof(char)){

    bzero(newbuf, BUFSIZE);

    /*receive data*/
    n = recvfrom(sockfd, newbuf, strlen(buf), 0, (struct sockaddr *)&serveraddr, &serverlen);
    if (n < 0) 
      error("ERROR in recvfrom");

    /*write into the file*/
    writeret = write(filefd, newbuf, FILE2KB*sizeof(char));

    buf[0] = 'A';
    /*Acknowledgement*/
    n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
    if (n < 0) 
      error("ERROR in sendto");
  }
  close(filefd);

}

/*
 * put - wrapper for put
 */
void put(char* filename){

}

/*
 * exit - wrapper for exit
 */
 void exit_func(){
   close(sockfd);
   exit(0);
 }

int main(int argc, char **argv) {
    int portno;  
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];
    char input[BUFSIZE];
    char file[BUFSIZE];

    /* check command line arguments */
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* get a message from the user */
    bzero(buf, BUFSIZE);
    printf("Please enter msg: ");
    fgets(buf, BUFSIZE, stdin);

    /* send the message to the server */
    serverlen = sizeof(serveraddr);
    n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
    if (n < 0) 
      error("ERROR in sendto");
    
    /* print the server's reply */
    n = recvfrom(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serveraddr, &serverlen);
    if (n < 0) 
      error("ERROR in recvfrom");
    printf("Echo from server: %s", buf);
    
    while(1){

      printf("Enter one of the following commands:\n"
          "1. get [filename]\n"
          "2. put [filename]\n"
          "3. delete [filename]\n"
          "4. ls\n"
          "5. exit\n");
       
        /* get input from user */
        bzero(buf, BUFSIZE);
        bzero(input, BUFSIZE);
        fgets(input, BUFSIZE, stdin);     

        n = sendto(sockfd, input, BUFSIZE, 0, (struct sockaddr *)&serveraddr, serverlen);
        if (n < 0) 
          error("ERROR in sendto");   

        if((strncmp(input, "get ", 4)) == 0){

          strncpy(file, input+4, strlen(input));

          for(int i=0;i<strlen(file);i++){
            if(file[i]=='\n'){
              file[i]='\0';
          }
          }
          
          printf("filename:%s\n",file);
          get(file);
          printf("File received!!!\n");
        }

        else if((strncmp(input, "put", 3)) == 0){
          strncpy(file, input+4, strlen(input));
          put(file);          
        }

        else if((strncmp(input, "delete", 6)) == 0){
          strncpy(file, input+6, strlen(input));
          //delete the file          
          /* print the server's reply */
          n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, &serverlen);
          if (n < 0) 
            error("ERROR in recvfrom");
          printf("File deleted from server: %s\n", buf);          
        }

        else if((strncmp(input, "ls", 2)) == 0){        
          /* print the server's reply */
          n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, &serverlen);
          if (n < 0) 
            error("ERROR in recvfrom");
          printf("ls output: %s\n", buf);  
        }

        else if((strncmp(input, "exit", 4)) == 0){
              /* print the server's reply */
          n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, &serverlen);
          if (n < 0) 
            error("ERROR in recvfrom");
          printf("%s", buf);    

          //exit          
          exit_func();
        }

        else{
          printf("Unknown command!! Please enter command from the given list\n");
        }

    }

    return 0;
}
