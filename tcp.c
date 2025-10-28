#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>


int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 1);
    int client = accept(server_fd, NULL, NULL);

    int fd = open("/dev/video2", O_RDWR);
    unsigned char buf[1024*1024];
    char *header = 
"HTTP/1.0 200 OK\r\n"
"Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
write(client, header, strlen(header));

while (1) {
    int len = read(fd, buf, sizeof(buf));
    if (len > 0) {
        dprintf(client, "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n", len);
        send(client, buf, len, 0);
        send(client, "\r\n", 2, 0);
    }
}

}
