#include "common.h"
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

int main() {
    //
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { err("socket()"); }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // 127.0.0.1
    //
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) { err("connect"); }

    //
    char msg[] = "hello";
    ssize_t n = write(fd, msg, strlen(msg));
    if (n < 0) { perror("write error"); }

    char rbuf[64] = {};
    n = read(fd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) { err("read error"); }
    printf("server says: %s\n", rbuf);

    close(fd);
    return 0;
}