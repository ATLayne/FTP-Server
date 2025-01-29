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

void exit_cmd(char *buf, int sockfd, struct sockaddr_in serveraddr, int serverlen);
int ls_cmd(char *buf, int sockfd, struct sockaddr_in serveraddr, int serverlen);
int delete_cmd(char *buf, int sockfd, struct sockaddr_in serveraddr, int serverlen);
int get_cmd(char *buf, int sockfd, struct sockaddr_in serveraddr, int serverlen);
void put_cmd(char *buf, int sockfd, struct sockaddr_in serveraddr, int serverlen);

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
    while (1) {
        bzero(buf, BUFSIZE);
        printf("Please enter command: ");
        fgets(buf, BUFSIZE, stdin);

        if(strncmp(buf, "exit", 4) == 0){
            exit_cmd(buf, sockfd, serveraddr, serverlen);
        }

        if(strncmp(buf, "ls", 2) == 0){
            if(ls_cmd(buf, sockfd, serveraddr, serverlen)){
                perror("ls command failed\n");
                exit(EXIT_FAILURE);
            }
        }

        if(strncmp(buf, "delete", 6) == 0){
            if(delete_cmd(buf, sockfd, serveraddr, serverlen)){
                perror("delete command failed\n");
                exit(EXIT_FAILURE);
            }
        }

        if(strncmp(buf, "get", 3) == 0){
            if(get_cmd(buf, sockfd, serveraddr, serverlen)){
                perror("get command failed\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    
    /* send the message to the server */
    serverlen = sizeof(serveraddr);
    n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
    if (n < 0) 
      error("ERROR in sendto");
    
    /* print the server's reply */
    printf("Command: %s\n", buf);
    while((n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, &serverlen)) > 0){
    
        if (n < 0) 
            error("ERROR in recvfrom");

        printf("%s", buf);
        bzero(buf, n);
    }

    return 0;
}

void exit_cmd(char *buf, int sockfd, struct sockaddr_in serveraddr, int serverlen){
    printf("In exit_cmd\n");
    int n;
    char term_string[4];

    serverlen = sizeof(serveraddr);
    n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
    if (n < 0) 
        error("ERROR in sendto");

    while ((n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, &serverlen)) > 0) {
        if (n < 0) 
            error("ERROR in recvfrom");

        memcpy(term_string, buf + 4, (4 * sizeof(char)));

        if(strncmp(term_string, "\n\n\n\n", 4) == 0) {
            printf("Server acknowledged termination\nExiting...\n");
            exit(EXIT_SUCCESS);
        }
    }
}

int ls_cmd(char *buf, int sockfd, struct sockaddr_in serveraddr, int serverlen){
    int n;
    int packet_number = 0;
    char term_string[4];
    printf("Command: %s", buf);

    serverlen = sizeof(serveraddr);
    n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
    if (n < 0) 
        error("ERROR in sendto");

    while((n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, &serverlen)) > 0){
        
        if (n < 0) 
            error("ERROR in recvfrom");
        
        memcpy(&packet_number, buf, sizeof(int));
        memcpy(term_string, buf + sizeof(int), (4 * sizeof(char)));
        
        if(strncmp(term_string, "\n\n\n\n", 4) == 0){
            bzero(buf, BUFSIZE);
            return EXIT_SUCCESS;
        }
        
        printf("Packet No. %d %s", packet_number, buf + sizeof(int) + (4 * sizeof(char)));
        bzero(buf, n);

        n = sendto(sockfd, &packet_number, sizeof(int), 0, (struct sockaddr *)&serveraddr, serverlen);
    }
}

int delete_cmd(char *buf, int sockfd, struct sockaddr_in serveraddr, int serverlen){
    int n;

    serverlen = sizeof(serveraddr);
    n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
    if (n < 0) 
        error("ERROR in sendto");

    while ((n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, &serverlen)) > 0){
        if (n < 0) 
            error("ERROR in recvfrom");
        
        if(strncmp(buf + 4, "\n\n\n\n", 4) == 0){
            bzero(buf, BUFSIZE);
            return(EXIT_SUCCESS);
        }

        printf("%s", buf);
    }
}

int get_cmd(char *buf, int sockfd, struct sockaddr_in serveraddr, int serverlen){
    int n;
    char res_buf[8 + 2048];
    int packet_number;

    serverlen = sizeof(serveraddr);
    n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
    if (n < 0) 
        error("ERROR in sendto");

    while ((n = recvfrom(sockfd, res_buf, 2056, 0, (struct sockaddr *)&serveraddr, &serverlen)) > 0){
        if (n < 0) 
            error("ERROR in recvfrom");
        
        memcpy(&packet_number, res_buf, sizeof(int));
        
        if(strncmp(res_buf + 4, "\n\r\n\r", 4) == 0){
            bzero(buf, BUFSIZE);
            printf("term_string received\n");
            return(EXIT_SUCCESS);
        }

        printf("%s", res_buf + 8);

        n = sendto(sockfd, &packet_number, sizeof(int), 0, (struct sockaddr *)&serveraddr, serverlen);

        bzero(res_buf, 2056);
    }

    return 0;
}
