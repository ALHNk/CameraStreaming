#include "udp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


struct udp_sender udp_init(int port) {
    struct udp_sender u;
    u.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (u.sockfd < 0) perror("socket");

    int opt = 1;
    setsockopt(u.sockfd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));

    memset(&u.addr, 0, sizeof(u.addr));   
    u.addr.sin_family = AF_INET;
    u.addr.sin_port = htons(port);
    u.addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    printf("UDP sender on port %d\n", port);

    return u;
}

int udp_send(struct udp_sender *u, const void *data, size_t size) {
    ssize_t sent = sendto(u->sockfd, data, size, 0,
                          (struct sockaddr *)&u->addr, sizeof(u->addr));
    if (sent < 0) perror("sendto");
    return 0;
}

void udp_close(struct udp_sender *u) {
    close(u->sockfd);
}
