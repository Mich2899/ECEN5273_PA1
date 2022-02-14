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

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char **argv) {
    int portno;  
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];
    char input[BUFSIZE];
    char file[BUFSIZE];
    int sockfd, n;
    int serverlen;
    struct sockaddr_in serveraddr;  
    char savefile[BUFSIZE];
    int packet=0;

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
    printf("Please enter test msg: ");
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
    printf("Echo from server: %s\n", buf);
    
    while(1){

      char ack[]={"ACK"};

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


        if((strncmp(input, "delete", 6)) == 0){
          /*Send input*/
          n = sendto(sockfd, input, BUFSIZE, 0, (struct sockaddr *)&serveraddr, serverlen);
          if (n < 0) 
            error("ERROR in sendto");     

          //strncpy(file, input+6, strlen(input));
          //delete the file          
          /* print the server's reply */
          n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, &serverlen);
          if (n < 0) 
            error("ERROR in recvfrom");
          printf("File deleted from server: %s\n", buf);          
        }

        else if((strncmp(input, "ls", 2)) == 0){   
         
          /*Send input*/
          n = sendto(sockfd, input, BUFSIZE, 0, (struct sockaddr *)&serveraddr, serverlen);
          if (n < 0) 
            error("ERROR in sendto");             

          /* print the server's reply */
          n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, &serverlen);
          if (n < 0) 
            error("ERROR in recvfrom");
          printf("ls output: %s\n", buf);  
        }

        else if((strncmp(input, "exit", 4)) == 0){

          /*Send input*/
          n = sendto(sockfd, input, BUFSIZE, 0, (struct sockaddr *)&serveraddr, serverlen);
          if (n < 0) 
            error("ERROR in sendto");   

              /* print the server's reply */
          n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, &serverlen);
          if (n < 0) 
            error("ERROR in recvfrom");
          printf("%s", buf);    

          //exit          
          close(sockfd);
          exit(0);
        }

                /*if command is get*/
        else if((strncmp(input, "get ", 4)) == 0){

          int filesize;   
          int writeret;          

          /*Send input*/
          n = sendto(sockfd, input, BUFSIZE, 0, (struct sockaddr *)&serveraddr, serverlen);
          if (n < 0) 
            error("ERROR in sendto");  

          // /*ACK*/
          // n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, &serverlen);
          // if (n < 0) 
          //   error("ERROR in recvfrom");
          // printf("msg: %s\n", buf);  

              //printf("Receive the size of the file!\n");
              //Receive the size of the file
              n = recvfrom(sockfd, &filesize, sizeof(int), 0,
              (struct sockaddr *) &serveraddr, &serverlen);
              if (n < 0)
                error("ERROR in recvfrom");                  

              /*Acknowledgement*/
              n = sendto(sockfd, ack, strlen(ack), 0, 
              (struct sockaddr *)&serveraddr, serverlen);
              if (n < 0) 
                error("ERROR in sendto");

          /*Copy the filename*/
          strncpy(file, input+4, strlen(input));
          
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
              int flag=0;

              //Make a loop here to receive chunks of file in size of 2KB and recieve ack after every send
              for(int i=0; i<=filesize;i+=2000){

                bzero(newbuf, 2000);

                /*receive data*/
                n = recvfrom(sockfd, newbuf, 2000, 0,
                 (struct sockaddr *)&serveraddr, &serverlen);
                if (n < 0) 
                  error("ERROR in recvfrom");
                
                /*write into the file*/
                writeret = write(filefd, newbuf, n);

                /*Acknowledgement*/
                n = sendto(sockfd, ack, strlen(ack), 0,
                 (struct sockaddr *)&serveraddr, serverlen);
                if (n < 0) 
                  error("ERROR in sendto");
              }
              close(filefd);

          printf("File received!!!\n");
        }

        else if((strncmp(input, "put ", 4)) == 0){

        struct stat finfo;
        off_t filesize;     
        bzero(file, BUFSIZE);

        n = sendto(sockfd, input, BUFSIZE, 0, (struct sockaddr *)&serveraddr, serverlen);
        if (n < 0) 
          error("ERROR in sendto");  

        strncpy(file, input+4, strlen(input)-4);
        //put logic        

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

        filesize = finfo.st_size;

        /*send the filesize*/
        n = sendto(sockfd, &finfo.st_size, sizeof(off_t), 0, 
            (struct sockaddr *) &serveraddr, serverlen);
        if (n < 0) 
          error("ERROR in sendto");       

        n = recvfrom(sockfd, buf, BUFSIZE, 0,
        (struct sockaddr *) &serveraddr, &serverlen);
        if (n < 0)
          error("ERROR in recvfrom");
          printf("Ack msg:%s\n", buf);
             
        char readbuf[2000];
        int readret=0;
        bzero(readbuf, 2000);
        bzero(buf,BUFSIZE);

          //Make a loop here to send chunks of file in size of 2KB and recieve ack after every send
          //for(off_t i=0; i<=filesize; i+= FILE2KB*sizeof(char)){

            while((readret = read(filefd, readbuf, 2000))>0){        
               
              // printf("readret:%d\n", readret);
              //send filesize 
                  n = sendto(sockfd, readbuf, readret, 0, 
                (struct sockaddr *) &serveraddr, serverlen);
                  if (n < 0) 
                    error("ERROR in sendto");  
                  printf("Packet number:%d\n", packet++);

                  //receive ack
                  n = recvfrom(sockfd, buf, BUFSIZE, 0,
                  (struct sockaddr *) &serveraddr, &serverlen);
                  if (n < 0)
                    error("ACK not receieved!!"); 
                  printf("ACK: %s\n", buf);

                  bzero(readbuf,2000);
                  bzero(buf, BUFSIZE);                  
            }
      close(filefd);
        }        

        else{
          printf("Unknown command!! Please enter command from the given list\n");
        }

    }

    return 0;
}
