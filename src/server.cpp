#include "common.h"
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

static void do_something(int connfd) {
    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
        msg("read() error");
        return;
    }
    printf("client says: %s\n", rbuf);

    char wbuf[] = "world";
    n = write(connfd, wbuf, strlen(wbuf));
    if (n < 0) { err("write error"); }
}

int main() {
    //
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { err("socket()"); }

    // config
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0); // wildcard address 0.0.0.0
    //
    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv) { err("bind()"); }

    //
    rv = listen(fd, SOMAXCONN);
    if (rv) { err("listen()"); }

    while (true) {
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);  // 会阻塞
        if (connfd < 0) {
            continue; // error
        }

        do_something(connfd);

        close(connfd);
    }

    return 0;
}
