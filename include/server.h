#pragma once

#include "common.h"
#include "file.h"
#include "connect.h"

#include <sys/socket.h> // socket syscall
#include <arpa/inet.h>  // net address transform
#include <netinet/ip.h> // ip data strcut

#include <vector>
#include <string>

namespace zedis {
class Server {
  private:
    File m_f;

  public:
    Server() : m_f{make_socket()} {}
    int make_socket() {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) { err("socket()"); }

        int val = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

        struct sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_port = ntohs(1234);
        addr.sin_addr.s_addr = ntohl(0); // wildcard address 0.0.0.0
        int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
        if (rv) { err("bind()"); }

        rv = listen(fd, SOMAXCONN);
        if (rv) { err("listen()"); }

        return fd;
    }

    std::unique_ptr<Conn> acceptConn() {
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);

        int connfd =
            accept(m_f.data(), (struct sockaddr *)&client_addr, &socklen);

        if (connfd < 0) {
            msg("accept() error");
            return nullptr;
        }

        File connf{connfd};

        std::unique_ptr<Conn> conn =
            std::make_unique<Conn>(std::move(connf), ConnState::STATE_REQ);

        if (!conn) {
            close(connfd);
            return nullptr;
        }

        return conn;
    }
};
} // namespace zedis