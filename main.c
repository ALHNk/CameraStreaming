#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>
#include "camera.h"
#include "udp.h"
#include "tcp.h"

#define FIRST_CAMERA        "/dev/video2"
#define SECOND_CAMERA       "/dev/video0"
#define DISCOVERY_PORT      5000
#define FIRST_STREAM_PORT   5001
#define SECOND_STREAM_PORT  5002
#define YOLO_SEND_PORT      5010
#define YOLO_RECV_PORT      5011
#define YOLO_HOST           "172.17.0.2"
#define RECV_BUF_SIZE       (8 * 1024 * 1024)


char *dest_ip;
atomic_bool client_alive = ATOMIC_VAR_INIT(0);

extern int defish(unsigned char *in_data, size_t in_size,
                  unsigned char **out_data, size_t *out_size);

struct cam_thread_arg {
    const char *device;
    int port;
};

void *control_thread(void *arg)
{
    printf("ControlThread started\n");
    int sock = *(int *)arg;
    char buf[1];
    while (1) {
        int r = recv(sock, buf, 1, 0);
        if (r <= 0) {
            atomic_store(&client_alive, 0);
            break;
        }
    }
    tcp_close(sock);
    return NULL;
}

void *camera_thread(void *arg)
{
    struct cam_thread_arg *cfg = arg;

    while (dest_ip == NULL)
        usleep(100000);

    int fd = camera_open(cfg->device);
    if (fd < 0) pthread_exit(NULL);

    struct buffer *buffers;
    int buffer_count;
    if (camera_start(fd, &buffers, &buffer_count) < 0) pthread_exit(NULL);

    struct udp_sender sender_to_client = udp_init(cfg->port,      dest_ip);
    struct udp_sender sender_to_yolo   = udp_init(YOLO_SEND_PORT, YOLO_HOST);

    int yolo_recv_sock = udp_bind(YOLO_RECV_PORT);
    if (yolo_recv_sock < 0) pthread_exit(NULL);
    struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
    setsockopt(yolo_recv_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)); 

    unsigned char *yolo_buf = malloc(RECV_BUF_SIZE);
    if (!yolo_buf) pthread_exit(NULL);

    void  *frame;
    size_t size;

    while (atomic_load(&client_alive)) {
        if (camera_capture(fd, buffers, buffer_count, &frame, &size) != 0)
            continue;

        unsigned char *defished      = NULL;
        size_t         defished_size = 0;
        unsigned char *send_buf;
        size_t         send_size;

        if (defish((unsigned char *)frame, size, &defished, &defished_size) == 0) {
            send_buf  = defished;
            send_size = defished_size;
        } else {
            printf("Error sending fisheye\n");
            send_buf  = (unsigned char *)frame;
            send_size = size;
        }

        udp_flush(yolo_recv_sock);
        udp_send_fragmented(&sender_to_yolo, send_buf, send_size);

        size_t labeled_size = 0;
        if (udp_receive_fragmented(yolo_recv_sock, yolo_buf, RECV_BUF_SIZE, &labeled_size) == 0
                && labeled_size > 0) {
            printf("YOLO frame: %zu bytes | start: %02X %02X | end: %02X %02X\n",
            labeled_size,
            yolo_buf[0], yolo_buf[1],
            yolo_buf[labeled_size - 2], yolo_buf[labeled_size - 1]);
            udp_send_fragmented(&sender_to_client, yolo_buf, labeled_size);
            // udp_send(&sender_to_client, yolo_buf, labeled_size);
        } else {
            printf("YOLO recv failed, forwarding pre-labeled frame\n");
            udp_send_fragmented(&sender_to_client, send_buf, send_size);
            // udp_send(&sender_to_client, send_buf, send_size);
        }

        free(defished);
    }

    free(yolo_buf);
    close(yolo_recv_sock);
    camera_release(fd, buffers, buffer_count);
    udp_close(&sender_to_client);
    udp_close(&sender_to_yolo);
    return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t t1, t2, control;
    struct cam_thread_arg cam1 = { FIRST_CAMERA,  FIRST_STREAM_PORT  };
    struct cam_thread_arg cam2 = { SECOND_CAMERA, SECOND_STREAM_PORT };

    if (argc == 5) {
        cam1.device = argv[1];
        cam1.port   = atoi(argv[2]);
        cam2.device = argv[3];
        cam2.port   = atoi(argv[4]);
    }

discover_again:
    printf("Waiting for client discovery on port %d...\n", DISCOVERY_PORT);
    dest_ip = get_client_ip(DISCOVERY_PORT);
    atomic_store(&client_alive, 1);

    if (dest_ip == NULL) {
        fprintf(stderr, "Failed to discover client\n");
        return 1;
    }

    int tcp_sock = tcp_connect(dest_ip, 12345);
    if (tcp_sock < 0) {
        fprintf(stderr, "TCP init failed, retrying...\n");
        dest_ip = NULL;
        sleep(1);
        goto discover_again;
    }

    pthread_create(&control, NULL, control_thread, &tcp_sock);
    pthread_create(&t1,      NULL, camera_thread,  &cam1);
    // pthread_create(&t2,      NULL, camera_thread,  &cam2);

    pthread_join(t1,      NULL);
    // pthread_join(t2,      NULL);
    pthread_join(control, NULL);

    tcp_close(tcp_sock);
    dest_ip = NULL;
    goto discover_again;

    return 0;
}