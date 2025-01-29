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

/*
 * Execute the specific command that was supplied to the server
 */
void execute_command(char *command, int sockfd, struct sockaddr_in clientaddr, int clientlen) {
    printf("Command Recieved: %s\n", command);
    char exit_cmd[] = "exit";
    char ls_cmd[] = "ls";
    char delete_cmd[] = "delete";
    char get_cmd[] = "get";
    //char put_cmd[] = "put";
    
    /*
     * exit command
     */
    if(strncmp(command, exit_cmd, 4) == 0){
        printf("Exiting\n");
        char return_message[] = "Server is terminating.\n";
        char packet[8 + BUFSIZE];
        int packet_number = 1;
        char term_string[] = "\n\n\n\n";

        memcpy(packet, &packet_number, sizeof(int));
        memcpy(packet + 4, term_string, (4 * sizeof(char)));
        memcpy(packet + 8, return_message, strlen(return_message));

        int n = sendto(sockfd, packet, 8 + strlen(return_message), 0, 
	       (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0) 
          error("ERROR in \"exit\" sendto");

        exit(EXIT_SUCCESS);
    }

    /*
     * ls command
     */
    if(strncmp(command, ls_cmd, 2) == 0){
      FILE *fp;
      char buffer[BUFSIZE] = {0};
      char term_string[] = "\n\n\n\n";
      int n;
      int packet_number = 0;
      int packet_number_ret;
      int ack = 0;

      char packet[sizeof(int) + (4 * sizeof(char)) + BUFSIZE];
      bzero(packet, sizeof(int) + (4 * sizeof(char)) + BUFSIZE);

      fp = popen(command, "r");
      while(fgets(buffer, sizeof(buffer), fp) != NULL){
        bzero(packet, sizeof(int) + (4 * sizeof(char)) + strlen(buffer));
        memcpy(packet, &packet_number, sizeof(int));
        memcpy(packet + sizeof(int) + (4 * sizeof(char)), buffer, strlen(buffer));

        do {
          n = sendto(sockfd, packet, sizeof(int) + (4 * sizeof(char)) + strlen(buffer), 0, 
          (struct sockaddr *) &clientaddr, clientlen);
          if (n < 0) 
            error("ERROR in \"ls\" sendto");

          n = recvfrom(sockfd, &packet_number_ret, sizeof(int), 0, (struct sockaddr *) &clientaddr, &clientlen);

          printf("Packet Number Returned: %d\n", packet_number_ret);

          if(packet_number_ret == packet_number){
            ack = 1;
          } else {
            ack = 0;
          }

        } while (ack == 0);

        packet_number++;
      }

      bzero(packet, sizeof(int) + (4 * sizeof(char)) + strlen(buffer));
      memcpy(packet + sizeof(int), term_string, strlen(term_string));
      n = sendto(sockfd, packet, sizeof(int) + (4 * sizeof(char)), 0, 
	       (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0) 
          error("ERROR in \"ls\" sendto");

      pclose(fp);
    }

    /*
     * delete command
     */
    if(strncmp(command, delete_cmd, 6) == 0) {
      int n;
      char packet[8 + BUFSIZE];
      char fail_msg[] = "Could not delete file.\n";
      char suc_msg[] = "File Deleted Successfully.\n";
      char term_string[] = "\n\n\n\n";
      char rm_cmd[50] = "rm";
      strcat(rm_cmd, (command + 6));
      printf("command to execute: %s\n", rm_cmd);

      bzero(packet, 8 + BUFSIZE);

      if(system(rm_cmd) != 0){
        memcpy(packet + 8, fail_msg, 8 + strlen(fail_msg));
        n = sendto(sockfd, packet, 8 + strlen(fail_msg), 0, 
	       (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0) 
          error("ERROR in \"delete fail\" sendto");
      } else {
        memcpy(packet + 8, suc_msg, 8 + strlen(suc_msg));
        n = sendto(sockfd, suc_msg, strlen(suc_msg), 0, 
	       (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0) 
          error("ERROR in \"delete suc\" sendto");
      }

      bzero(packet, 8 + BUFSIZE);
      memcpy(packet + 4, term_string, strlen(term_string));
      n = sendto(sockfd, packet, 8, 0, 
	       (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0) 
          error("ERROR in \"delete fail\" sendto");
    }

    /*
     * get command
     */
    if(strncmp(command, get_cmd, 3) == 0){
      setbuf(stdout, NULL);
      FILE *fp;
      char buffer[BUFSIZE] = {0};
      char *filename = command + 4;
      int n;
      char packet[8 + BUFSIZE] = {0};
      size_t bytes_read;
      int packet_number = 1;
      int packet_number_returned;
      int ack = 0;
      char term_string[4] = "\n\r\n\r";

      filename[strlen(filename) - 1] = '\0';
      printf("filename: \"%s\"\n", filename);

      fp = fopen(filename, "rb");
      if(fp == NULL){
        perror("Error opening file\n");
        exit(EXIT_FAILURE);
      }

      while ((bytes_read = fread(buffer, 1, BUFSIZE, fp)) > 0) {
        printf("bytes_read: %d\n", bytes_read);
        printf("buffer len: %d\n", sizeof(buffer) / sizeof(buffer[0]));
        printf("Packet Number: %d\n", packet_number);
        
        memcpy(packet, &packet_number, sizeof(int));
        memcpy(packet + 8, buffer, bytes_read);

        printf("Packet Data: %s\n", packet + 8);
        do {
          n = sendto(sockfd, packet, 8 + bytes_read, 0, (struct sockaddr *) &clientaddr, clientlen);
          if (n < 0) 
            error("ERROR in \"ls\" sendto");

          n = recvfrom(sockfd, &packet_number_returned, sizeof(int), 0, (struct sockaddr *) &clientaddr, &clientlen);

          //printf("Packet Number Returned: %d\n", packet_number_returned);

          if(packet_number_returned == packet_number){
            ack = 1;
          } else {
            ack = 0;
          }

        } while (ack == 0);
        
        packet_number++;
        bzero(packet, 8 + bytes_read);
      }
      memcpy(packet + 4, term_string, 4);
      n = sendto(sockfd, packet, 8, 0, (struct sockaddr *) &clientaddr, clientlen);


      fclose(fp);

    }


}

int main(int argc, char **argv) {
  int sockfd; /* socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  //struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  //char *hostaddrp; /* dotted decimal host addr string */
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
    // hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
		// 	  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    // if (hostp == NULL)
    //   error("ERROR on gethostbyaddr");
    // hostaddrp = inet_ntoa(clientaddr.sin_addr);
    // if (hostaddrp == NULL)
    //   error("ERROR on inet_ntoa\n");
    // printf("server received datagram from %s (%s)\n", 
	  //  hostp->h_name, hostaddrp);
    // printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);
    
    /* 
     * sendto: echo the input back to the client 
     */
    // n = sendto(sockfd, buf, strlen(buf), 0, 
	  //      (struct sockaddr *) &clientaddr, clientlen);
    // if (n < 0) 
    //   error("ERROR in sendto");
  }
}
