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
#include <errno.h>
#include <sys/time.h>

#define BUFSIZE 1024
#define DATASIZE 2048
#define HEADERSIZE 8
#define PACKETSIZE HEADERSIZE + DATASIZE


int exit_cmd(char *command, int sockfd, struct sockaddr_in clientaddr, int clientlen);
int delete_cmd(char *command, int sockfd, struct sockaddr_in clientaddr, int clientlen);
int ls_cmd(char *command, int sockfd, struct sockaddr_in clientaddr, int clientlen);
int get_cmd(char *command, int sockfd, struct sockaddr_in clientaddr, int clientlen);
int put_cmd(char *command, int sockfd, struct sockaddr_in clientaddr, int clientlen);

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
  //struct hostent *hostp; /* client host info */
  //char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  char packet[PACKETSIZE] = {0};
  int bytes_read;
  char command[BUFSIZE] = {0};

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

  //Set the timeout
  struct timeval timeout;

  timeout.tv_sec = 0; // 5 seconds timeout
  timeout.tv_usec = 500;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
      perror("Error setting timeout");
      exit(1);
  }

  /* 
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  while (1) {

    /*
     * recvfrom: receive a UDP datagram from a client
     */
    bzero(command, BUFSIZE);
    bzero(packet, PACKETSIZE);
    bytes_read = recvfrom(sockfd, packet, PACKETSIZE, 0,
		 (struct sockaddr *) &clientaddr, (socklen_t *)&clientlen);
    if (bytes_read < 0)
      continue;
      //error("ERROR in recvfrom");

    memcpy(command, packet + HEADERSIZE, BUFSIZE);
    
    if(strncmp(command, "exit", 4) == 0){
        exit_cmd(command, sockfd, clientaddr, clientlen);
    } else if(strncmp(command, "delete", 6) == 0){
        delete_cmd(command, sockfd, clientaddr, clientlen);
    } else if(strncmp(command, "ls", 2) == 0){
        ls_cmd(command, sockfd, clientaddr, clientlen);
    } else if(strncmp(command, "get", 3) == 0){
        get_cmd(command, sockfd, clientaddr, clientlen);
    } else if(strncmp(command, "put", 3) == 0){
        put_cmd(command, sockfd, clientaddr, clientlen);
    } else {
        continue;
    }

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

int exit_cmd(char *command, int sockfd, struct sockaddr_in clientaddr, int clientlen){
    char return_message[] = "Goodbye.\n";
    char packet[PACKETSIZE];
    int packet_number = 1;
    char term_string[] = "\n\r\n\r";
    int bytes_sent;

    memcpy(packet, &packet_number, sizeof(int));
    memcpy(packet + 4, term_string, (4 * sizeof(char)));
    memcpy(packet + HEADERSIZE, return_message, strlen(return_message));

    bytes_sent = sendto(sockfd, packet, 8 + strlen(return_message), 0, 
      (struct sockaddr *) &clientaddr, clientlen);
    if (bytes_sent < 0) 
      error("ERROR in \"exit\" sendto");

    return 0;
}
int delete_cmd(char *command, int sockfd, struct sockaddr_in clientaddr, int clientlen){
    char packet[PACKETSIZE] = {0};
    char fail_msg[] = "Could not delete file.\n";
    char suc_msg[] = "File Deleted Successfully.\n";
    char term_string[] = "\n\r\n\r";
    char rm_cmd[50] = "rm ";
    int bytes_sent;
    memcpy(rm_cmd + 3, command + 6, strlen(command) - 6);
    printf("command to execute: %s", rm_cmd);

    if(system(rm_cmd) != 0){
      memcpy(packet + HEADERSIZE, fail_msg, strlen(fail_msg));
      bytes_sent = sendto(sockfd, packet, HEADERSIZE + strlen(fail_msg), 0, 
        (struct sockaddr *) &clientaddr, clientlen);
      if (bytes_sent < 0) 
        error("ERROR in \"delete fail\" sendto");
    } else {
      memcpy(packet + HEADERSIZE, suc_msg, strlen(suc_msg));
      bytes_sent = sendto(sockfd, packet, HEADERSIZE + strlen(suc_msg), 0, 
        (struct sockaddr *) &clientaddr, clientlen);
      if (bytes_sent < 0) 
        error("ERROR in \"delete suc\" sendto");
    }

    bzero(packet, PACKETSIZE);
    memcpy(packet + 4, term_string, strlen(term_string));
    bytes_sent = sendto(sockfd, packet, 8, 0, 
        (struct sockaddr *) &clientaddr, clientlen);
      if (bytes_sent < 0) 
        error("ERROR in \"delete fail\" sendto");

    return 0;
}

int ls_cmd(char *command, int sockfd, struct sockaddr_in clientaddr, int clientlen){
    FILE *fp;
    char data[DATASIZE] = {0};
    char packet[PACKETSIZE] = {0};
    char term_string[] = "\n\r\n\r";
    int packet_number = 1;
    int packet_number_ret;
    int bytes_sent;
    int bytes_read;
    //int ack = 0;

    fp = popen(command, "r");
    while(fgets(data, DATASIZE, fp) != NULL){
      memcpy(packet, &packet_number, sizeof(int));
      memcpy(packet + HEADERSIZE, data, strlen(data));

      bytes_sent = sendto(sockfd, packet, HEADERSIZE + strlen(data), 0, (struct sockaddr *) &clientaddr, clientlen);

      bytes_read = recvfrom(sockfd, &packet_number_ret, sizeof(int), 0, (struct sockaddr *) &clientaddr, (socklen_t *)&clientlen);
      while (bytes_read < 0) {
          printf("Did not get ack for packet no: %d\n", packet_number);
          bytes_sent = sendto(sockfd, packet, HEADERSIZE + strlen(data), 0, (struct sockaddr *) &clientaddr, clientlen);
          bytes_read = recvfrom(sockfd, &packet_number_ret, sizeof(int), 0, (struct sockaddr *) &clientaddr, (socklen_t *)&clientlen);
          if(packet_number_ret == packet_number){
            break;
          }
      }

      bzero(packet, HEADERSIZE + strlen(data));
      packet_number++;
    }

    memcpy(packet + 4, term_string, 4);
    bytes_sent = sendto(sockfd, packet, 8, 0, (struct sockaddr *) &clientaddr, clientlen);

    pclose(fp);

    return 0;
}

int get_cmd(char *command, int sockfd, struct sockaddr_in clientaddr, int clientlen){
    setbuf(stdout, NULL);

    FILE *fp;
    char packet[PACKETSIZE] = {0};
    char data[DATASIZE] = {0};
    char *filename;
    int bytes_read;
    int bytes_read_from_file;
    int bytes_sent;
    int packet_number = 1;
    int client_reply = 1;
    //int ack = 0;
    char term_string[4] = "\n\r\n\r";
    int total_bytes_read = 0;

    filename = command + 4;
    filename[strlen(filename) - 1] = '\0';
    printf("filename: \"%s\"\n", filename);

    fp = fopen(filename, "rb");
    if(fp == NULL){
      perror("Error opening file\n");
      exit(EXIT_FAILURE);
    }

    while ((bytes_read_from_file = fread(data, 1, DATASIZE, fp)) > 0) {
      total_bytes_read += bytes_read_from_file;
      //printf("Packet Number: %d\n", packet_number);
      memcpy(packet, &packet_number, sizeof(int));
      memcpy(packet + HEADERSIZE, data, bytes_read_from_file);

      bytes_sent = sendto(sockfd, packet, HEADERSIZE + bytes_read_from_file, 0, (struct sockaddr *) &clientaddr, clientlen);
      if (bytes_sent < 0) 
        error("ERROR in \"ls\" sendto");

      bytes_read = recvfrom(sockfd, &client_reply, sizeof(int), 0, (struct sockaddr *) &clientaddr, (socklen_t *)&clientlen);
      while (bytes_read < 0) {
          printf("Did not get ack for packet no: %d\n", packet_number);
          bytes_sent = sendto(sockfd, packet, HEADERSIZE + bytes_read_from_file, 0, (struct sockaddr *) &clientaddr, clientlen);
          bytes_read = recvfrom(sockfd, &client_reply, sizeof(int), 0, (struct sockaddr *) &clientaddr, (socklen_t *)&clientlen);
          if(client_reply == packet_number){
            break;
          }
      }

      packet_number++;
      bzero(packet, HEADERSIZE + bytes_read_from_file);
    }

    memcpy(packet + 4, term_string, 4);
    bytes_sent = sendto(sockfd, packet, 8, 0, (struct sockaddr *) &clientaddr, clientlen);

    fclose(fp);
    printf("Total Bytes Sent: %d\n", total_bytes_read);

    return 0;
}

int put_cmd(char *command, int sockfd, struct sockaddr_in clientaddr, int clientlen){
    setbuf(stdout, NULL);

    FILE *fp;
    char packet[PACKETSIZE] = {0};
    char *filename;
    int bytes_read;
    int bytes_sent;
    int packet_number = 0;
    int last_packet_number = 0;
    int ack = 0;
    char term_string[4];
    int total_bytes_read = 0;

    filename = command + 4;
    filename[strlen(filename) - 1] = '\0';
    //printf("filename: \"%s\"\n", filename);

    fp = fopen(filename, "w");
    if(fp == NULL){
      perror("Error opening file\n");
      exit(EXIT_FAILURE);
    }

    ack = 1;
    memcpy(packet + HEADERSIZE, &ack, sizeof(int));

    clientlen = sizeof(clientaddr);
        
    while(1){
        bytes_sent = sendto(sockfd, packet, HEADERSIZE + sizeof(int), 0, (struct sockaddr *)&clientaddr, clientlen);
        if (bytes_sent < 0) 
            error("ERROR in sendto");

        bytes_read = recvfrom(sockfd, packet, PACKETSIZE, 0, (struct sockaddr *)&clientaddr, (socklen_t *)&clientlen);
        bzero(packet, PACKETSIZE);
        if(bytes_read < 0){
          continue;
        } else {
          memcpy(&last_packet_number, packet, sizeof(int));
          break;
        }
    }

    printf("File Transfering\n");
    while (1){
        bytes_read = recvfrom(sockfd, packet, PACKETSIZE, 0, (struct sockaddr *)&clientaddr, (socklen_t *)&clientlen);
        if(bytes_read < 0)
          continue;
        
        memcpy(&packet_number, packet, sizeof(int));

        if(packet_number == last_packet_number){
            //last_packet_number = packet_number;
            //printf("Discarding Packet No: %d\n", packet_number);
            bytes_sent = sendto(sockfd, &packet_number, sizeof(int), 0, (struct sockaddr *)&clientaddr, clientlen);
            bzero(packet, PACKETSIZE);
            continue;
        }
        total_bytes_read += (bytes_read - HEADERSIZE);
        memcpy(term_string, packet +  4, 4 * sizeof(char));
        
        if(strncmp(term_string, "\n\r\n\r", 4) == 0){
            bzero(command, BUFSIZE);
            bzero(packet, PACKETSIZE);
            //printf("term_string received\n");
            fclose(fp);
            printf("Total Bytes Received: %d\n",total_bytes_read);
            return(EXIT_SUCCESS);
        }

        //printf("Packet Number: %d\n", packet_number);
        fwrite(packet + HEADERSIZE, sizeof(char), bytes_read - HEADERSIZE, fp);

        bytes_sent = sendto(sockfd, &packet_number, sizeof(int), 0, (struct sockaddr *)&clientaddr, clientlen);

        last_packet_number = packet_number;
        bzero(packet, PACKETSIZE);
    }

    fclose(fp);

    return 0;
}
