#include "tcp.h"
#define PORT 12345

int server_fd = -1; 

int tcp_init()
{
    if (server_fd >= 0) {
        close(server_fd);
        server_fd = -1;
    }
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { 
        perror("socket"); 
        return -1; 
    }
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { 
        perror("bind"); 
        close(server_fd);
        server_fd = -1;
        return -1; 
    }
    
    if (listen(server_fd, 1) < 0) { 
        perror("listen"); 
        close(server_fd);
        server_fd = -1;
        return -1; 
    }
    
    printf("Waiting for TCP connection on port %d...\n", PORT);
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) { 
        perror("accept"); 
        close(server_fd);
        server_fd = -1;
        return -1; 
    }
    
    printf("TCP client connected\n");
    
    close(server_fd);
    server_fd = -1;
    
    return client_fd;
}

int tcp_close(int client_fd)
{
    if (client_fd >= 0) {
        close(client_fd);
    }
    
    if (server_fd >= 0) {
        close(server_fd);
        server_fd = -1;
    }
    
    return 0;
}