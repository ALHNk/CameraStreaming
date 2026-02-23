#ifndef TCP_H
#define TCP_H

#include <stdio.h>
#include <stdlib.h>
// #include <fcntl.h>
#include <unistd.h>
// #include <sys/ioctl.h>
// #include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int tcp_connect(const char* dest_ip, int port); // Jetson connects OUT to Quest
int tcp_close(int fd);

#endif
