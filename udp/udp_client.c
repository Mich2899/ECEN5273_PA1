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

#define BUFSIZE 1024

int sockfd;
char buf1[BUFSIZE];
/*
 * get - wrapper for get
 */
void get(char* filename){

}

/*
 * put - wrapper for put
 */
void put(char* filename){

}

/* 
 * delete - wrapper for delete
 */
void delete(char* filename){

}

/*
 * ls - wrapper for ls
 */
 void ls(){
  FILE* filep;
   filep = popen("ls *","r");
   fread(buf1, 1, BUFSIZE, filep);
   printf("Output ls: %s\n", buf1);
   pclose(filep);
 }

/*
 * exit - wrapper for exit
 */
 void exit_func(){
   printf("Exiting......\n");
   close(sockfd);
   exit(0);
 }

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char **argv) {
    int portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;    
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];
    char input[BUFSIZE];
    char* file=NULL;

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
    n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
    if (n < 0) 
      error("ERROR in sendto");
    
    /* print the server's reply */
    n = recvfrom(sockfd, buf, strlen(buf), 0, &serveraddr, &serverlen);
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
        bzero(input, BUFSIZE);
        fgets(input, BUFSIZE, stdin);

        if((strncmp(input, "get", 3)) == 0){
          file = input+4;
          get(file);
        }

        else if((strncmp(input, "put", 3)) == 0){
          file = input+4;
          put(file);          
        }

        else if((strncmp(input, "delete", 6)) == 0){
          n = sendto(sockfd, input, BUFSIZE, 0, &serveraddr, serverlen);
            if (n < 0) 
              error("ERROR in sendto");             
        }

        else if((strncmp(input, "ls", 2)) == 0){
          ls();
          n = sendto(sockfd, buf1, BUFSIZE, 0, &serveraddr, serverlen);
            if (n < 0) 
              error("ERROR in sendto");     
          
        }

        else if((strncmp(input, "exit", 4)) == 0){
            n = sendto(sockfd, input, 4, 0, &serveraddr, serverlen);
            if (n < 0) 
              error("ERROR in sendto");     

          exit_func();
        }

        else{
          printf("Unknown command!! Please enter command from the given list\n");
        }

    }

    return 0;
}
