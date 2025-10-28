#ifndef UDP_H
#define UDP_H

#include <stddef.h>

int udp_init(int port);
int udp_send(int sockfd, const void *data, size_t size);
void udp_close(int sockfd);

#endif
