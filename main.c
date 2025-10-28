#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "camera.h"
#include "udp.h"

// #define SECRET "Camera"
#define PORT 5000

int main() {

    int fd = camera_open("/dev/video2");
    if (fd < 0) return 1;

    struct buffer *buffers;
    int buffer_count;
    if (camera_start(fd, &buffers, &buffer_count) < 0) return 1;

    int sockfd = udp_init(PORT);
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
