#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "camera.h"
#include "udp.h"

// #define SECRET "Camera"
#define DEFAULT_PORT 5001
#define DEFAULT_DEVICE "/dev/video4"

int main(int argc, char *argv[]) {
    char *device;
    int port;
    if(argc < 3)
    {
        device =  DEFAULT_DEVICE;
        port = DEFAULT_PORT;
    }
    else 
    {
        device =  argv[1];
        port = atoi(argv[2]);
    }
    int fd = camera_open(device);
    if (fd < 0) return 1;

    struct buffer *buffers;
    int buffer_count;
    if (camera_start(fd, &buffers, &buffer_count) < 0) return 1;

    int sockfd = udp_init(port);
    if (sockfd < 0) return 1;

    printf("Streaming started (UDP)...\n");

    void *frame;
    size_t size;
    while (1) {
        if (camera_capture(fd, buffers, buffer_count, &frame, &size) == 0) {
            udp_send(sockfd, frame, size);
        }
        usleep(1000); // reduce CPU load
    }

    camera_release(fd, buffers, buffer_count);
    udp_close(sockfd);
    return 0;
}
