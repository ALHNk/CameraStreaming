#ifndef UDP_H
#define UDP_H

#include <stddef.h>
#include <arpa/inet.h>

#define MAX_UDP_PAYLOAD 1400
#define MAX_UDP_PACKET (MAX_UDP_PAYLOAD + sizeof(UdpChunkHeader))

typedef struct {
    uint32_t frame_id;
    uint16_t chunk_index;
    uint16_t total_chunks;
} UdpChunkHeader;



struct udp_sender {
    int sockfd;
    struct sockaddr_in addr;
};

char* get_client_ip(int discovery_port);

struct udp_sender udp_init(int port, char* dest_ip);
int udp_send(struct udp_sender *u, const void *data, size_t size);
int udp_send_fragmented(struct udp_sender *u, const void *data, size_t size);
void udp_close(struct udp_sender *u);

#endif
