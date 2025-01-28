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

#define BUFSIZE 2048
#define PACKETSIZE 1024

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

void execute_command(char *command, int sockfd, struct sockaddr_in clientaddr, int clientlen) {
    printf("Command Recieved: %s\n", command);
    char exit_cmd[] = "exit";
    char ls_cmd[] = "ls";
    char delete_cmd[] = "delete";
    char get_cmd[] = "get";
    char put_cmd[] = "put";
    
    if(strncmp(command, exit_cmd, 4) == 0){
        printf("Exiting\n");
        char ret_msg[] = "Server is terminating.\n";
        int n = sendto(sockfd, ret_msg, strlen(ret_msg), 0, 
	       (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0) 
          error("ERROR in \"exit\" sendto");

        exit(EXIT_SUCCESS);
    }

    if(strncmp(command, ls_cmd, 2) == 0){
      FILE *fp;
      char buffer[BUFSIZE] = {0};
      int n;

      fp = popen(command, "r");
      while(fgets(buffer, sizeof(buffer), fp) != NULL){
        n = sendto(sockfd, buffer, strlen(buffer), 0, 
	       (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0) 
          error("ERROR in \"ls\" sendto");
      }

      pclose(fp);
    }

    if(strncmp(command, delete_cmd, 6) == 0) {
      int n;
      char fail_msg[] = "Could not delete file.\n";
      char suc_msg[] = "File Deleted Successfully.\n";
      char rm_cmd[50] = "rm";
      strcat(rm_cmd, (command + 6));
      printf("command to execute: %s\n", rm_cmd);

      if(system(rm_cmd) != 0){
        n = sendto(sockfd, fail_msg, strlen(fail_msg), 0, 
	       (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0) 
          error("ERROR in \"delete fail\" sendto");
      } else {
        n = sendto(sockfd, suc_msg, strlen(suc_msg), 0, 
	       (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0) 
          error("ERROR in \"delete suc\" sendto");
      }
    }

    if(strncmp(command, get_cmd, 3) == 0){
      FILE *fp;
      char buffer[BUFSIZE] = {0};
      char *filename = command + 4;
      int n;

      filename[strlen(filename) - 1] = '\0';
      printf("filename: \"%s\"\n", filename);

      fp = fopen(filename, "rb");
      if(fp == NULL){
        perror("Error opening file\n");
        exit(EXIT_FAILURE);
      }
      
      size_t bytes_read;
      int pack_num = 0;

      while ((bytes_read = fread(buffer, 1, BUFSIZE, fp)) > 0) {
        char packet[sizeof(int) + BUFSIZE];
        //memcpy(packet + sizeof(), pack_num, sizeof(int));
        memcpy(packet + sizeof(int), buffer, bytes_read);
        packet[0] = pack_num;
        n = sendto(sockfd, packet, bytes_read + sizeof(int), 0, (struct sockaddr *) &clientaddr, sizeof(clientaddr));
        if (n < 0) {
            perror("Error sending data");
            exit(1);
        }
        
        pack_num++;
      }

      /*
      n = sendto(sockfd, buffer, 1024, 0, 
	       (struct sockaddr *) &clientaddr, clientlen);
      if (n < 0) 
        error("ERROR in \"ls\" sendto");
      */
      

      printf("bytes_read: %d\n", bytes_read);
      printf("n: %d\n", n);

      fclose(fp);

    }


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

    execute_command(buf, sockfd, clientaddr, clientlen);

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
    printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);
    
    /* 
     * sendto: echo the input back to the client 
     */
    n = sendto(sockfd, buf, strlen(buf), 0, 
	       (struct sockaddr *) &clientaddr, clientlen);
    if (n < 0) 
      error("ERROR in sendto");
  }
}
