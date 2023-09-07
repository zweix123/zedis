#pragma once

#include "common.h"
#include "connect.h"
#include "execute.h"
#include "file.h"
#include "heap.h"
#include "list.h"

#include <arpa/inet.h>  // net address transform
#include <netinet/ip.h> // ip data strcut
#include <sys/socket.h> // socket syscall

#include <string>
#include <unordered_map>
#include <vector>

namespace zedis {

const uint64_t k_idle_timeout_ms = 5 * 1000;

class Server {
  private:
    File m_f;
    std::unordered_map<int, std::shared_ptr<Conn>> fd2conn;
    DList head;
    Heap &heap;
    TheadPool &tp;

  public:
    Server() : m_f{make_socket()}, heap{core::m_heap}, tp{core::tp} {
        thread_pool_init(&tp, 4);
    }

    static int make_socket() {
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

    void addNewConn(std::shared_ptr<Conn> conn) {
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

        std::shared_ptr<Conn> conn =
            std::make_shared<Conn>(std::move(connf), ConnState::STATE_REQ);
        head.insert_before(&conn->idle_node);

        if (!conn) {
            return -1; // 不用析构, connfd已经由connf接管
        }

        addNewConn(conn);
        return 0;
    }

    uint32_t next_timer_ms() {
        uint64_t now_us = get_monotonic_usec();
        uint64_t next_us = std::numeric_limits<uint64_t>::max();

        if (!head.empty()) {
            Conn *next = container_of(head.next, Conn, idle_node);
            next_us =
                std::min(next_us, next->idle_start + k_idle_timeout_ms * 1000);
        }

        if (!heap.empty()) { next_us = std::min(next_us, heap.get_min()); }

        if (next_us == std::numeric_limits<uint64_t>::max()) { return 10000; }

        if (next_us <= now_us) { return 0; }

        return (uint32_t)((next_us - now_us) / 1000);
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
                if (!conn) { continue; }
                struct pollfd pfd = {};
                pfd.fd = conn->get_fd();
                pfd.events = conn->get_event();
                pfd.events = pfd.events | POLLERR;
                poll_args.push_back(pfd);
            }

            // poll for active fds
            int timeout_ms = (int)next_timer_ms();
            int rv =
                poll(poll_args.data(), (nfds_t)poll_args.size(), timeout_ms);
            if (rv < 0) { err("poll"); }

            // sprocess active connections
            for (size_t i = 1; i < poll_args.size(); ++i) {
                if (poll_args[i].revents) {
                    auto conn = fd2conn[poll_args[i].fd];
                    // conn->connection_io();
                    conn->start_connection_io(&head);

                    // client closed normally, or something bad happened.
                    // destroy this connection
                    if (fd2conn[poll_args[i].fd]->is_end()) {
                        conn_done(conn.get());
                    }
                }
            }

            // handle timers
            process_timers();

            // try to accept a new connection if the listening fd is active
            if (poll_args[0].revents) { (void)acceptConn(); }
        }
    }
    void process_timers() {
        uint64_t now_us = get_monotonic_usec();

        // idle timers
        while (!head.empty()) {
            Conn *next = container_of(head.next, Conn, idle_node);
            uint64_t next_us = next->idle_start + k_idle_timeout_ms * 1000;
            if (next_us >= now_us + 1000) {
                // not ready, the extra 1000us is for the ms resolution of
                // poll()
                break;
            }

            std::cout << "removing idle connection: " << next->get_fd() << "\n";
            conn_done(next);

            // TTL timers
            const size_t k_max_works = 2000;
            size_t nworks = 0;
            while (!heap.empty() && heap.get_min() < now_us) {
                core::Entry *ent =
                    container_of(heap.get_min_ref(), core::Entry, heap_idx);
                HNode *node =
                    core::m_map.pop(&ent->node, [](HNode *lhs, HNode *rhs) {
                        return lhs == rhs;
                    });
                assert(node == &ent->node);
                delete ent;

                if (nworks++ >= k_max_works) { break; }
                // 这里要注意的, 在实际情况可能同时有大量的键过期,
                // 这个释放的时间可能很长, 这里只是粗暴的限制每次次数
            }
        }
    }

    void conn_done(Conn *conn) {
        fd2conn.erase(conn->get_fd());
        conn->idle_node.detach();
    }
};
} // namespace zedis