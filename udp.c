#include "udp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdint.h>

char* get_client_ip(int discovery_port) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("discovery socket");
        return NULL;
    }
    
    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;
    bind_addr.sin_port = htons(discovery_port);
    
    if (bind(sockfd, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
        perror("bind discovery");
        close(sockfd);
        return NULL;
    }
    
    printf("Waiting for discovery on port %d...\n", discovery_port);
    
    char buffer[64];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    while (1) {
        ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                             (struct sockaddr *)&client_addr, &addr_len);
        
        if (n > 0 && strncmp(buffer, "DISCOVER", 8) == 0) {
            // Send ACK back to client
            const char *ack = "ACK";
            sendto(sockfd, ack, strlen(ack), 0,
                   (struct sockaddr *)&client_addr, sizeof(client_addr));
            
            // Extract client IP
            char *client_ip = malloc(INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
            
            printf("Client discovered: %s\n", client_ip);
            close(sockfd);
            return client_ip;
        }
    }
    
    close(sockfd);
    return NULL;
}

struct udp_sender udp_init(int port, char* dest_ip) {
    struct udp_sender u;
    u.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (u.sockfd < 0) perror("socket");


    if(strcmp(dest_ip, "255.255.255.255") == 0)
    {
        int opt = 1;
        setsockopt(u.sockfd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));
    }
    

    // INCREASE SEND BUFFER SIZE
    int sndbuf = 1024 * 1024; // 1MB buffer
    if (setsockopt(u.sockfd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf)) < 0) {
        perror("setsockopt SO_SNDBUF");
    }
    
    // SET PRIORITY (helps with Wi-Fi scheduling)
    int priority = 6;
    if (setsockopt(u.sockfd, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority)) < 0) {
        perror("setsockopt SO_PRIORITY");
    }
    
    // DISABLE NAGLE-LIKE BEHAVIOR
    int nodelay = 1;
    if (setsockopt(u.sockfd, IPPROTO_IP, IP_TOS, &nodelay, sizeof(nodelay)) < 0) {
        perror("setsockopt IP_TOS");
    }

    memset(&u.addr, 0, sizeof(u.addr));   
    u.addr.sin_family = AF_INET;
    u.addr.sin_port = htons(port);
    u.addr.sin_addr.s_addr = inet_addr(dest_ip);

    printf("UDP sender on port %d\n", port);

    return u;
}

int udp_send(struct udp_sender *u, const void *data, size_t size) {
    ssize_t sent = sendto(u->sockfd, data, size, 0,
                          (struct sockaddr *)&u->addr, sizeof(u->addr));
    if (sent < 0) perror("sendto");
    return 0;
}

static uint32_t frame_counter = 0;

int udp_send_fragmented(struct udp_sender *u, const void *data, size_t size) {
    const unsigned char *ptr = data;
    uint16_t total_chunks = (uint16_t)((size + MAX_UDP_PAYLOAD - 1) / MAX_UDP_PAYLOAD);
    uint32_t frame_id = frame_counter++;

    for (uint16_t i = 0; i < total_chunks; i++) {
        size_t chunk_size = (size > MAX_UDP_PAYLOAD) ? MAX_UDP_PAYLOAD : size;

        unsigned char packet[MAX_UDP_PACKET];
        UdpChunkHeader hdr = { htonl(frame_id), htons(i), htons(total_chunks) };
        memcpy(packet, &hdr, sizeof(hdr));
        memcpy(packet + sizeof(hdr), ptr, chunk_size);

        ssize_t sent = sendto(u->sockfd, packet, sizeof(hdr) + chunk_size, 0,
                              (struct sockaddr *)&u->addr, sizeof(u->addr));
        if (sent < 0) { perror("sendto"); return -1; }

        ptr += chunk_size;
        size -= chunk_size;
        usleep(200); // give network breathing room
    }
    return 0;
}
int udp_receive_fragmented(int sockfd, unsigned char *out_buf, size_t out_buf_size, size_t *out_size)
{
    unsigned char packet[MAX_UDP_PACKET];
    uint32_t current_frame_id = UINT32_MAX;
    uint16_t total_chunks = 0;
    uint16_t chunks_received = 0;
    *out_size = 0;

    while (1) {
        ssize_t n = recvfrom(sockfd, packet, sizeof(packet), 0, NULL, NULL);
        if (n < 0) { perror("recvfrom"); return -1; }
        if (n < (ssize_t)sizeof(UdpChunkHeader)) continue;

        UdpChunkHeader hdr;
        memcpy(&hdr, packet, sizeof(hdr));
        uint32_t frame_id    = ntohl(hdr.frame_id);
        uint16_t chunk_index = ntohs(hdr.chunk_index);
        uint16_t chunk_total = ntohs(hdr.total_chunks);

        if (current_frame_id == UINT32_MAX) {
            current_frame_id = frame_id;
            total_chunks     = chunk_total;
        }

        if (frame_id != current_frame_id) {
            current_frame_id = frame_id;
            total_chunks     = chunk_total;
            chunks_received  = 0;
            *out_size        = 0;
        }

        size_t payload_size = (size_t)n - sizeof(UdpChunkHeader);
        size_t offset       = (size_t)chunk_index * MAX_UDP_PAYLOAD;

        if (offset + payload_size > out_buf_size) {
            fprintf(stderr, "reassembly buffer overflow\n");
            return -1;
        }

        memcpy(out_buf + offset, packet + sizeof(UdpChunkHeader), payload_size);
        *out_size = offset + payload_size > *out_size ? offset + payload_size : *out_size;
        chunks_received++;

        if (chunks_received >= total_chunks)
            return 0;
    }
}

int udp_bind(int port) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(port),
        .sin_addr.s_addr = INADDR_ANY
    };
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("udp_bind"); return -1;
    }
    return fd;
}

void udp_close(struct udp_sender *u) {
    close(u->sockfd);
}
