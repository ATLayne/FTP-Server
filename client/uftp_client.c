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
#include <errno.h>

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
    int cmd_status;

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
    timeout.tv_sec = 0; 
    timeout.tv_usec = 1500;
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
        printf("Please enter command: \n");
        printf("get [filename], put [filename], delete [filename], ls, or exit: ");
        fgets(buf, BUFSIZE, stdin);

        if(strncmp(buf, "exit", 4) == 0){
            exit_cmd(buf, sockfd, serveraddr, serverlen);
        } else if(strncmp(buf, "ls", 2) == 0){
            cmd_status = ls_cmd(buf, sockfd, serveraddr, serverlen);
        } else if(strncmp(buf, "delete", 6) == 0){
            cmd_status = delete_cmd(buf, sockfd, serveraddr, serverlen);
        } else if(strncmp(buf, "get", 3) == 0){
            cmd_status = get_cmd(buf, sockfd, serveraddr, serverlen);
        } else if(strncmp(buf, "put", 3) == 0){
            cmd_status = put_cmd(buf, sockfd, serveraddr, serverlen);
        } else {
            printf("Command not recognized\n");
        }
        if(cmd_status != 0){
            printf("Operation Failed\n\n");
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
    int timeout_counter = 0;

    memcpy(packet, &packet_number, sizeof(int));
    memcpy(packet + HEADERSIZE, buf, strlen(buf));

    serverlen = sizeof(serveraddr);
    bytes_sent = sendto(sockfd, packet, HEADERSIZE + strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
    if (bytes_sent < 0) 
        error("ERROR in sendto");

    while (1) {
        bytes_read = recvfrom(sockfd, packet, PACKETSIZE, 0, (struct sockaddr *)&serveraddr, (socklen_t *)&serverlen);
        if(bytes_read < 0){
            timeout_counter++;
            if (timeout_counter == 10)
            {
                printf("Timeout: No response from server\n");
                printf("Exiting...\n");
                exit(-1);
            }
            continue;
        }

        memcpy(term_string, packet + 4, (4 * sizeof(char)));

        if(strncmp(term_string, "\n\r\n\r", 4) == 0) {
            printf("%s", packet + HEADERSIZE);
            bzero(buf, BUFSIZE);
            close(sockfd);
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
    int last_packet_number;
    char term_string[4];
    int timeout_counter = 0;

    memcpy(packet, &packet_number, sizeof(int));
    memcpy(packet + HEADERSIZE, buf, strlen(buf));

    serverlen = sizeof(serveraddr);
    bytes_sent = sendto(sockfd, packet, HEADERSIZE + strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
    if (bytes_sent < 0) 
        error("ERROR in sendto");

    while(1){

        bytes_read = recvfrom(sockfd, packet, PACKETSIZE, 0, (struct sockaddr *)&serveraddr, (socklen_t *)&serverlen);
        if(bytes_read < 0){
            timeout_counter++;
            if (timeout_counter == 10)
            {
                printf("Timeout: No response from server\n");
                return EXIT_FAILURE;
            }
            continue;
        }
        
        memcpy(&packet_number, packet, sizeof(int));
        if(packet_number == last_packet_number){
            //last_packet_number = packet_number;
            //printf("Discarding Packet No: %d\n", packet_number);
            bytes_sent = sendto(sockfd, &packet_number, sizeof(int), 0, (struct sockaddr *)&serveraddr, serverlen);
            bzero(packet, PACKETSIZE);
            continue;
        }
        memcpy(term_string, packet + sizeof(int), (4 * sizeof(char)));
        
        if(strncmp(term_string, "\n\r\n\r", 4) == 0){
            bzero(packet, PACKETSIZE);
            printf("\n\n");
            return EXIT_SUCCESS;
        }
        
        printf("%s", packet + HEADERSIZE);
        bytes_sent = sendto(sockfd, &packet_number, sizeof(int), 0, (struct sockaddr *)&serveraddr, serverlen);
        last_packet_number = packet_number;
        bzero(packet, bytes_read);
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
    int timeout_counter = 0;

    memcpy(packet, &packet_number, sizeof(int));
    memcpy(packet + HEADERSIZE, buf, strlen(buf));

    serverlen = sizeof(serveraddr);
    bytes_sent = sendto(sockfd, packet, HEADERSIZE + strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
    if (bytes_sent < 0) 
        error("ERROR in sendto");

    while (1){
        bytes_read = recvfrom(sockfd, packet, PACKETSIZE, 0, (struct sockaddr *)&serveraddr, (socklen_t *)&serverlen);
        if(bytes_read < 0){
            timeout_counter++;
            if (timeout_counter == 10)
            {
                printf("Timeout: No response from server\n");
                return EXIT_FAILURE;
            }
            continue;
        }

        memcpy(term_string, packet + 4, (4 * sizeof(char)));
        
        if(strncmp(packet + 4, "\n\r\n\r", 4) == 0){
            bzero(buf, BUFSIZE);
            return(EXIT_SUCCESS);
        }

        printf("%s\n\n", packet + HEADERSIZE);
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
    int last_packet_number = 0;
    char term_string[4];

    memcpy(filename, buf + 4, strlen(buf) - 5);
    fp = fopen(filename, "w");
    
    memcpy(packet, &packet_number, sizeof(int));
    memcpy(packet + HEADERSIZE, buf, strlen(buf));

    serverlen = sizeof(serveraddr);
    bytes_sent = sendto(sockfd, packet, HEADERSIZE + strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
    if (bytes_sent < 0) 
        error("ERROR in sendto");

    while (1){

        bytes_read = recvfrom(sockfd, packet, PACKETSIZE, 0, (struct sockaddr *)&serveraddr, (socklen_t *)&serverlen);
        if(bytes_read < 0){
            continue;
        }
        
        memcpy(&packet_number, packet, sizeof(int));
        if(packet_number == last_packet_number){
            last_packet_number = packet_number;
            printf("Discarding Packet No: %d\n", packet_number);
            bytes_sent = sendto(sockfd, &packet_number, sizeof(int), 0, (struct sockaddr *)&serveraddr, serverlen);
            bzero(packet, PACKETSIZE);
            continue;
        }
        memcpy(term_string, packet +  4, 4 * sizeof(char));
        
        if(strncmp(term_string, "\n\r\n\r", 4) == 0){
            bzero(buf, BUFSIZE);
            bzero(packet, PACKETSIZE);
            printf("File Transfer Complete\n\n");
            fclose(fp);
            return(EXIT_SUCCESS);
        }

        //printf("Packet Number: %d\n", packet_number);
        fwrite(packet + HEADERSIZE, sizeof(char), bytes_read - HEADERSIZE, fp);

        bytes_sent = sendto(sockfd, &packet_number, sizeof(int), 0, (struct sockaddr *)&serveraddr, serverlen);

        last_packet_number = packet_number;
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
        return(EXIT_FAILURE);
    }

    memcpy(packet, &packet_number, sizeof(int));
    memcpy(packet + HEADERSIZE, buf, strlen(buf));

    serverlen = sizeof(serveraddr);
    
    
    bytes_sent = sendto(sockfd, packet, HEADERSIZE + strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
        if (bytes_sent < 0) 
            error("ERROR in sendto");
    bytes_read = recvfrom(sockfd, packet, PACKETSIZE, 0, (struct sockaddr *)&serveraddr, (socklen_t *)&serverlen);
    memcpy(&ack, packet + HEADERSIZE, sizeof(int));

    printf("File Transfer In Progress\n");
    packet_number++;
    while ((bytes_read_from_file = fread(data, 1, DATASIZE, fp)) > 0) {
        //printf("Packet Number: %d\n", packet_number);
        memcpy(packet, &packet_number, sizeof(int));
        memcpy(packet + HEADERSIZE, data, bytes_read_from_file);

        bytes_sent = sendto(sockfd, packet, HEADERSIZE + bytes_read_from_file, 0, (struct sockaddr *)&serveraddr, serverlen);
        if (bytes_sent < 0) 
            error("ERROR in \"ls\" sendto");

        bytes_read = recvfrom(sockfd, &packet_number_returned, sizeof(int), 0, (struct sockaddr *)&serveraddr, (socklen_t *)&serverlen);
        while (bytes_read < 0) {
            //printf("Did not get ack for packet no: %d\n", packet_number);
            bytes_sent = sendto(sockfd, packet, HEADERSIZE + bytes_read_from_file, 0, (struct sockaddr *) &serveraddr, serverlen);
            bytes_read = recvfrom(sockfd, &packet_number_returned, sizeof(int), 0, (struct sockaddr *) &serveraddr, (socklen_t *)&serverlen);
            if(packet_number_returned == packet_number){
                break;
            }
        }
        
        packet_number++;
        bzero(packet, HEADERSIZE + bytes_read_from_file);
    }

    bzero(packet, PACKETSIZE);
    memcpy(packet + 4, term_string, 4);
    bytes_sent = sendto(sockfd, packet, HEADERSIZE, 0, (struct sockaddr *)&serveraddr, serverlen);
        if (bytes_sent < 0) 
        error("ERROR in \"ls\" sendto");
    
    printf("File Transfer Complete\n\n");
    

    return 0;
}
