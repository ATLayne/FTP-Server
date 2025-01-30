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
#include <sys/time.h>

#define BUFSIZE 1024
#define DATASIZE 2048
#define HEADERSIZE 8
#define PACKETSIZE HEADERSIZE + DATASIZE

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
int put_cmd(char *buf, int sockfd, struct sockaddr_in serveraddr, int serverlen);

int main(int argc, char **argv) {
    int sockfd, portno;
    int serverlen = 0;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];
    struct timeval timeout;

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


    // Set the timeout
    timeout.tv_sec = 5; // 5 seconds timeout
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        perror("Error setting timeout");
        exit(1);
    }

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

    /* get a command from the user */
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

        if(strncmp(buf, "put", 3) == 0){
            if(put_cmd(buf, sockfd, serveraddr, serverlen)){
                perror("put command failed\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    return 0;
}

void exit_cmd(char *buf, int sockfd, struct sockaddr_in serveraddr, int serverlen){
    setbuf(stdout, NULL);
    
    char packet[PACKETSIZE] = {0};
    int bytes_sent;
    int bytes_read;
    int packet_number = 0;
    char term_string[4];

    memcpy(packet, &packet_number, sizeof(int));
    memcpy(packet + HEADERSIZE, buf, strlen(buf));

    serverlen = sizeof(serveraddr);
    bytes_sent = sendto(sockfd, packet, HEADERSIZE + strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
    if (bytes_sent < 0) 
        error("ERROR in sendto");

    while ((bytes_read = recvfrom(sockfd, packet, PACKETSIZE, 0, (struct sockaddr *)&serveraddr, (socklen_t *)&serverlen)) > 0) {
        if (bytes_read < 0) 
            error("ERROR in recvfrom");

        memcpy(term_string, packet + 4, (4 * sizeof(char)));

        if(strncmp(term_string, "\n\r\n\r", 4) == 0) {
            printf("%s", packet + HEADERSIZE);
            bzero(buf, BUFSIZE);
            exit(EXIT_SUCCESS);
        }
    }
}

int ls_cmd(char *buf, int sockfd, struct sockaddr_in serveraddr, int serverlen){
    setbuf(stdout, NULL);
    
    char packet[PACKETSIZE] = {0};
    int bytes_sent;
    int bytes_read;
    int packet_number = 0;
    char term_string[4];

    memcpy(packet, &packet_number, sizeof(int));
    memcpy(packet + HEADERSIZE, buf, strlen(buf));

    serverlen = sizeof(serveraddr);
    bytes_sent = sendto(sockfd, packet, HEADERSIZE + strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
    if (bytes_sent < 0) 
        error("ERROR in sendto");

    while((bytes_read = recvfrom(sockfd, packet, PACKETSIZE, 0, (struct sockaddr *)&serveraddr, (socklen_t *)&serverlen)) > 0){
        if (bytes_read < 0) 
            error("ERROR in recvfrom");
        
        memcpy(&packet_number, packet, sizeof(int));
        memcpy(term_string, packet + sizeof(int), (4 * sizeof(char)));
        
        if(strncmp(term_string, "\n\r\n\r", 4) == 0){
            bzero(packet, PACKETSIZE);
            return EXIT_SUCCESS;
        }
        
        printf("%s", packet + HEADERSIZE);
        bzero(packet, bytes_read);

        bytes_sent = sendto(sockfd, &packet_number, sizeof(int), 0, (struct sockaddr *)&serveraddr, serverlen);
    }
    
    return EXIT_FAILURE;
}

int delete_cmd(char *buf, int sockfd, struct sockaddr_in serveraddr, int serverlen){
    setbuf(stdout, NULL);

    char packet[PACKETSIZE] = {0};
    int bytes_sent;
    int bytes_read;
    int packet_number = 0;
    char term_string[4];

    memcpy(packet, &packet_number, sizeof(int));
    memcpy(packet + HEADERSIZE, buf, strlen(buf));

    serverlen = sizeof(serveraddr);
    bytes_sent = sendto(sockfd, packet, HEADERSIZE + strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
    if (bytes_sent < 0) 
        error("ERROR in sendto");

    while ((bytes_read = recvfrom(sockfd, packet, PACKETSIZE, 0, (struct sockaddr *)&serveraddr, (socklen_t *)&serverlen)) > 0){
        if (bytes_read < 0) 
            error("ERROR in recvfrom");

        memcpy(term_string, packet + 4, (4 * sizeof(char)));
        
        if(strncmp(packet + 4, "\n\r\n\r", 4) == 0){
            bzero(buf, BUFSIZE);
            return(EXIT_SUCCESS);
        }

        printf("%s", packet + HEADERSIZE);
    }

    return EXIT_FAILURE;
}

int get_cmd(char *buf, int sockfd, struct sockaddr_in serveraddr, int serverlen){
    setbuf(stdout, NULL);

    FILE *fp;
    char filename[50] = {0};
    char packet[PACKETSIZE];
    int bytes_sent;
    int bytes_read;
    int packet_number = 0;
    char term_string[4];

    memcpy(filename, buf + 4, strlen(buf) - 5);
    fp = fopen(filename, "w");
    
    memcpy(packet, &packet_number, sizeof(int));
    memcpy(packet + HEADERSIZE, buf, strlen(buf));

    serverlen = sizeof(serveraddr);
    bytes_sent = sendto(sockfd, packet, HEADERSIZE + strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
    if (bytes_sent < 0) 
        error("ERROR in sendto");

    while ((bytes_read = recvfrom(sockfd, packet, PACKETSIZE, 0, (struct sockaddr *)&serveraddr, (socklen_t *)&serverlen)) > 0){
        if (bytes_read < 0) 
            error("ERROR in recvfrom");
        
        memcpy(&packet_number, packet, sizeof(int));
        memcpy(term_string, packet +  4, 4 * sizeof(char));
        
        if(strncmp(term_string, "\n\r\n\r", 4) == 0){
            bzero(buf, BUFSIZE);
            bzero(packet, PACKETSIZE);
            printf("\n\nterm_string received\n");
            fclose(fp);
            return(EXIT_SUCCESS);
        }

        fwrite(packet + HEADERSIZE, sizeof(char), bytes_read - HEADERSIZE, fp);

        bytes_sent = sendto(sockfd, &packet_number, sizeof(int), 0, (struct sockaddr *)&serveraddr, serverlen);

        bzero(packet, PACKETSIZE);
    }

    return 0;
}

int put_cmd(char *buf, int sockfd, struct sockaddr_in serveraddr, int serverlen){
    setbuf(stdout, NULL);

    FILE *fp;
    char filename[50] = {0};
    char packet[PACKETSIZE];
    char data[DATASIZE];
    int bytes_sent;
    int bytes_read;
    int packet_number = 0;
    char term_string[4] = "\n\r\n\r";
    int ack = 0;
    int bytes_read_from_file;
    int packet_number_returned;

    memcpy(filename, buf + 4, strlen(buf) - 5);
    fp = fopen(filename, "rb");
    if(fp == NULL){
        perror("Error opening file\n");
        exit(EXIT_FAILURE);
    }

    printf("Filename: %s\n", filename);

    memcpy(packet, &packet_number, sizeof(int));
    memcpy(packet + HEADERSIZE, buf, strlen(buf));

    serverlen = sizeof(serveraddr);
    bytes_sent = sendto(sockfd, packet, HEADERSIZE + strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
    if (bytes_sent < 0) 
        error("ERROR in sendto");

    bytes_read = recvfrom(sockfd, packet, PACKETSIZE, 0, (struct sockaddr *)&serveraddr, (socklen_t *)&serverlen);
    if (bytes_read < 0) 
        error("ERROR in recvfrom");

    memcpy(&ack, packet + HEADERSIZE, sizeof(int));

    if(ack != 1){
        printf("No ack from server. Exiting\n");
        exit(EXIT_FAILURE);
    } else {
        while ((bytes_read_from_file = fread(data, 1, DATASIZE, fp)) > 0) {
            printf("bytes_read_from_file: %d\n", bytes_read_from_file);
            printf("Packet Number: %d\n", packet_number);
            
            memcpy(packet, &packet_number, sizeof(int));
            memcpy(packet + HEADERSIZE, data, bytes_read_from_file);

            do {
                bytes_sent = sendto(sockfd, packet, HEADERSIZE + bytes_read_from_file, 0, (struct sockaddr *)&serveraddr, serverlen);
                    if (bytes_sent < 0) 
                    error("ERROR in \"ls\" sendto");

                bytes_read = recvfrom(sockfd, &packet_number_returned, sizeof(int), 0, (struct sockaddr *)&serveraddr, (socklen_t *)&serverlen);

                if(packet_number_returned == packet_number){
                ack = 1;
                } else {
                ack = 0;
                }

            } while (ack == 0);
            
            packet_number++;
            bzero(packet, HEADERSIZE + bytes_read_from_file);
        }

        bzero(packet, PACKETSIZE);
        memcpy(packet + 4, term_string, 4);
        bytes_sent = sendto(sockfd, packet, HEADERSIZE, 0, (struct sockaddr *)&serveraddr, serverlen);
            if (bytes_sent < 0) 
            error("ERROR in \"ls\" sendto");
    }

    return 0;
}
