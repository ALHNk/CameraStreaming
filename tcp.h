#ifndef TCP_H
#define TCP_H

#include <stdio.h>
#include <stdlib.h>
// #include <fcntl.h>
// #include <unistd.h>
// #include <sys/ioctl.h>
// #include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int tcp_init();
int tcp_close(int client_fd);

#endif