#ifndef CAMERA_H
#define CAMERA_H

#include <linux/videodev2.h>
#include <stdlib.h>

#define WIDTH 640
#define HEIGHT 480
#define BUFFER_COUNT 4

struct buffer {
    void   *start;
    size_t  length;
};

int camera_open(const char *device);
int camera_start(int fd, struct buffer **buffers_out, int *buffer_count);
int camera_capture(int fd, struct buffer *buffers, int buffer_count, void **frame_out, size_t *size_out);
void camera_release(int fd, struct buffer *buffers, int buffer_count);

#endif
