#pragma once

#include "common.h"
#include "file.h"
#include "connect.h"

#include <sys/socket.h> // socket syscall
#include <arpa/inet.h>  // net address transform
#include <netinet/ip.h> // ip data strcut

#include <vector>
#include <string>
#include <unordered_map>

namespace zedis {
class Server {
  private:
    File m_f;
    std::unordered_map<int, std::unique_ptr<Conn>> fd2conn;

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

    void addNewConn(std::unique_ptr<Conn> conn) {
        auto conn_fd = conn->get_fd();
        assert(
            fd2conn.find(conn_fd) != fd2conn.end()
            || fd2conn[conn_fd] == nullptr);
        fd2conn[conn_fd] = std::move(conn);
    }

    int32_t acceptConn() {
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);

        int connfd =
            accept(m_f.data(), (struct sockaddr *)&client_addr, &socklen);

        if (connfd < 0) {
            msg("accept() error");
            return -1;
        }

        File connf{connfd};

        std::unique_ptr<Conn> conn =
            std::make_unique<Conn>(std::move(connf), ConnState::STATE_REQ);

        if (!conn) return -1; // 不用析构, connfd已经由connf接管

        addNewConn(std::move(conn));
        return 0;
    }

    void join() { // fb是套接字
        // set the listen fd to nonblocking mode
        m_f.set_nb();

        // the event loop
        std::vector<struct pollfd> poll_args;
        while (true) {
            // prepare the arguments of the poll()
            poll_args.clear();
            // for convenience, the listening fd is put in the first position
            struct pollfd pfd = {m_f.data(), POLLIN, 0};
            poll_args.push_back(pfd);
            // connection fds
            for (auto const &[fd, conn] : fd2conn) {
                if (!conn) continue;
                struct pollfd pfd = {};
                pfd.fd = conn->get_fd();
                pfd.events = conn->get_event();
                pfd.events = pfd.events | POLLERR;
                poll_args.push_back(pfd);
            }

            // poll for active fds
            // the timeout argument doesn't matter here
            int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), 1000);
            if (rv < 0) { err("poll"); }

            // sprocess active connections
            for (size_t i = 1; i < poll_args.size(); ++i) {
                if (poll_args[i].revents) {
#ifdef DEBUG
                    std::cout << "start a connect io\n";
#endif
                    fd2conn[poll_args[i].fd]->connection_io();
                    // client closed normally, or something bad happened.
                    // destroy this connection
                    if (fd2conn[poll_args[i].fd]->is_end()) {
                        fd2conn.erase(poll_args[i].fd);
                    }
                }
            }

            // try to accept a new connection if the listening fd is active
            if (poll_args[0].revents) { (void)acceptConn(); }
        }
    }
};
} // namespace zedis