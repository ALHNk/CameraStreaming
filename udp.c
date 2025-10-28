#include "udp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

static struct sockaddr_in server_addr;

int udp_init(int port) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { perror("socket"); return -1; }
     
    int opt = 1;
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) < 0) {
        perror("setsockopt broadcast");
        close(sockfd);
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));   
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    // if(bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    // {
    //     perror("bind failed");
    //     close(sockfd);
    //     exit(EXIT_FAILURE);
    // }

    printf("UDP server started at port: %d \n", port);

    return sockfd;
}

int udp_send(int sockfd, const void *data, size_t size) {
    ssize_t sent = sendto(sockfd, data, size, 0,
                          (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (sent < 0) { perror("sendto"); return -1; }
    return 0;
}

void udp_close(int sockfd) {
    close(sockfd);
}
