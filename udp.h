#ifndef UDP_H
#define UDP_H

#include <stddef.h>
#include <arpa/inet.h>

#define MAX_UDP_PACKET 1400  

struct udp_sender {
    int sockfd;
    struct sockaddr_in addr;
};

struct udp_sender udp_init(int port);
int udp_send(struct udp_sender *u, const void *data, size_t size);
int udp_send_fragmented(struct udp_sender *u, const void *data, size_t size);
void udp_close(struct udp_sender *u);

#endif
