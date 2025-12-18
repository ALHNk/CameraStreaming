#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "camera.h"
#include "udp.h"

#define FIRST_CAMERA "/dev/video2"
#define SECOND_CAMERA "/dev/video0"
#define DISCOVERY_PORT 5000
#define FIRST_STREAM_PORT 5001 
#define SECOND_STREAM_PORT 5002 

char* dest_ip;

extern int defish(unsigned char* in_data, size_t in_size,
                      unsigned char** out_data, size_t* out_size);

struct cam_thread_arg {
    const char *device;
    int port;
};

void *camera_thread(void *arg) {
    struct cam_thread_arg *cfg = arg;

    while (dest_ip == NULL) {
        usleep(100000); 
    }

    int fd = camera_open(cfg->device);
    if (fd < 0) pthread_exit(NULL);

    struct buffer *buffers;
    int buffer_count;
    if (camera_start(fd, &buffers, &buffer_count) < 0) pthread_exit(NULL);

    struct udp_sender sender = udp_init(cfg->port, dest_ip);

    void *frame;
    size_t size;

    while (1) {
        if (camera_capture(fd, buffers, buffer_count, &frame, &size) == 0) {
            udp_send(&sender, frame, size);
            // udp_send_fragmented(&sender, frame, size);
        }
        // usleep(1000);
    }

    camera_release(fd, buffers, buffer_count);
    udp_close(&sender);

    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t t1, t2;

    struct cam_thread_arg cam1 = { FIRST_CAMERA, FIRST_STREAM_PORT };
    struct cam_thread_arg cam2 = { SECOND_CAMERA,  SECOND_STREAM_PORT };

    if (argc == 5) {
        cam1.device = argv[1];
        cam1.port = atoi(argv[2]);
        cam2.device = argv[3];
        cam2.port = atoi(argv[4]);
    }

    printf("Waiting for client discovery on port %d...\n", DISCOVERY_PORT);
    dest_ip = get_client_ip(DISCOVERY_PORT);
    
    if (dest_ip == NULL) {
        fprintf(stderr, "Failed to discover client\n");
        return 1;
    }


    pthread_create(&t1, NULL, camera_thread, &cam1);
    pthread_create(&t2, NULL, camera_thread, &cam2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}
