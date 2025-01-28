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

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

void exit_cmd();
void ls_cmd(char *buf, int sockfd, struct sockaddr_in serveraddr, int serverlen);
void delete_cmd();
void get_cmd();
void put_cmd();

int main(int argc, char **argv) {
    int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];

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

    if(strncmp(buf, "exit", 4) == 0){
        exit_cmd();
    }

    if(strncmp(buf, "ls", 2) == 0){
        ls_cmd(buf, sockfd, serveraddr, serverlen);
    }

    /* send the message to the server */
    serverlen = sizeof(serveraddr);
    n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
    if (n < 0) 
      error("ERROR in sendto");
    
    /* print the server's reply */
    printf("Command: %s\n", buf);
    while((n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen)) > 0){
    
        if (n < 0) 
            error("ERROR in recvfrom");

        printf("%s", buf);
        bzero(buf, n);
    }

    return 0;
}

void exit_cmd(){
    printf("In exit_cmd\n");
    exit(EXIT_SUCCESS);
}

void ls_cmd(char *buf, int sockfd, struct sockaddr_in serveraddr, int serverlen){
    printf("In ls_cmd\n");

    int n;
    char seq_num[sizeof(int)];
    printf("Command: %s\n", buf);

    serverlen = sizeof(serveraddr);
    n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
    if (n < 0) 
      error("ERROR in sendto");

    while((n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen)) > 0){
        memcpy(seq_num, buf, sizeof(int));
        if (n < 0) 
            error("ERROR in recvfrom");

        printf("Packet No. %d %s", seq_num ,buf + sizeof(int));
        bzero(buf, n);
    }

    exit(EXIT_SUCCESS);
}
