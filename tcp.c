#include "tcp.h"
#define TCP_PORT 12345

int tcp_connect(const char* dest_ip, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return -1; }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(dest_ip);

    printf("Connecting TCP to %s:%d...\n", dest_ip, port);
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(fd);
        return -1;
    }
    printf("TCP connected to client\n");
    return fd;
}

int tcp_close(int fd) {
    if (fd >= 0) close(fd);
    return 0;
}