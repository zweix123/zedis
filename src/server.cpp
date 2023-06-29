#include "common.h"
#include "request.h"

enum class ConnState {
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2,
};

class Conn {
  private:
    int fd{-1};
    ConnState state{ConnState::STATE_REQ};
    size_t rbuf_size{0};
    uint8_t rbuf[4 + k_max_msg]{};
    size_t wbuf_size{0};
    size_t wbuf_sent{0};
    uint8_t wbuf[4 + k_max_msg]{};

  public:
    friend std::ostream &operator<<(std::ostream &out, const Conn &obj) {
        out << "Conn(\n";
        out << "\tfd = " << obj.fd << ",";
        out << "\tstate = " << obj.state << ",";
        return out;
    }

    Conn(
        int m_fd = -1,
        ConnState m_state = ConnState::STATE_REQ,
        size_t m_rbuf_size = 0,
        size_t m_wbuf_size = 0,
        size_t m_wbuf_sent = 0)
        : fd{m_fd}
        , state{m_state}
        , rbuf_size{m_rbuf_size}
        , wbuf_size{m_wbuf_size}
        , wbuf_sent{m_wbuf_sent} {}

    int get_fd() const { return fd; }
    short int get_event() const {
        if (state == ConnState::STATE_REQ) return POLLIN;
        else if (state == ConnState::STATE_RES)
            return POLLOUT;
        else
            assert(0);
    }
    bool is_end() const { return state == ConnState::STATE_END; }

    bool try_one_request() {
        if (rbuf_size < 4) return false;
        uint32_t len = 0;
        memcpy(&len, &rbuf[0], 4);
        if (len > k_max_msg) {
            msg("too long");
            state = ConnState::STATE_END;
            return false;
        }
        if (4 + len > rbuf_size) return false;

        uint32_t rescode = 0;
        uint32_t wlen = 0;

        int32_t err = do_request(&rbuf[4], len, &rescode, &wbuf[4 + 4], &wlen);
        if (err) {
            state = ConnState::STATE_END;
            return false;
        }
        wlen += 4;

        memcpy(&wbuf[0], &wlen, 4);
        memcpy(&wbuf[4], &rescode, 4);
        wbuf_size = 4 + wlen;
        size_t remain = rbuf_size - 4 - len;

        if (remain) { memmove(rbuf, &rbuf[4 + len], remain); }
        rbuf_size = remain;

        state = ConnState::STATE_RES;
        state_response();

        return (state == ConnState::STATE_REQ);
    }

    void state_response() {
        while (try_flush_buffer()) {}
    }

    bool try_flush_buffer() {
        ssize_t rv = 0;
        do {
            size_t remain = wbuf_size - wbuf_sent;
            rv = write(fd, &wbuf[wbuf_sent], remain);

        } while (rv < 0 && errno == EINTR);
        if (rv < 0 && errno == EAGAIN) return false;
        if (rv < 0) {
            msg("write() error");
            state = ConnState::STATE_END;
            return false;
        }
        wbuf_sent += (size_t)rv;
        assert(wbuf_sent <= wbuf_size);
        if (wbuf_sent == wbuf_size) {
            state = ConnState::STATE_REQ;
            wbuf_sent = 0;
            wbuf_size = 0;
            return false;
        }
        return true;
    }

    void state_request() {
        while (try_fill_buffer()) {}
    }

    bool try_fill_buffer() {
        assert(rbuf_size < sizeof(rbuf));
        ssize_t rv = 0;

        do {
            size_t cap = sizeof(rbuf) - rbuf_size;
            rv = read(fd, &rbuf[rbuf_size], cap);
        } while (rv < 0 && errno == EINTR);
        if (rv < 0 && errno == EAGAIN) { return false; }
        if (rv < 0) {
            msg("read() error");
            state = ConnState::STATE_END;
            return false;
        }
        if (rv == 0) {
            if (rbuf_size > 0) msg("unexpected EOF");
            else
                msg("EOF");
            state = ConnState::STATE_END;
            return false;
        }
        rbuf_size += (size_t)rv;
        assert(rbuf_size <= sizeof(rbuf));

        while (try_one_request()) {}
        return (state == ConnState::STATE_REQ);
    }

    void connection_io() {
        if (state == ConnState::STATE_REQ) {
            state_request();
        } else if (state == ConnState::STATE_RES) {
            state_response();
        } else {
            assert(0);
        }
    }
};

class EventEngine {
  private:
    std::map<int, std::unique_ptr<Conn>> fd2conn;

  public:
    EventEngine() {}
    void put(std::unique_ptr<Conn> conn) {
        auto conn_fd = conn->get_fd();
        assert(
            fd2conn.find(conn_fd) != fd2conn.end()
            || fd2conn[conn_fd] == nullptr);
        fd2conn[conn->get_fd()] = std::move(conn);
    }
    int32_t accept_new_conn(int fd) {
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
        if (connfd < 0) {
            msg("accept() error");
            return -1; // error
        }
        fd_set_nb(connfd);
        std::unique_ptr<Conn> conn =
            std::make_unique<Conn>(connfd, ConnState::STATE_REQ, 0, 0, 0);
        if (!conn) {
            close(connfd);
            return -1;
        }

        put(std::move(conn));
        return 0;
    }
    void join(int fd) { // fb是套接字
        // set the listen fd to nonblocking mode
        fd_set_nb(fd);

        // the event loop
        std::vector<struct pollfd> poll_args;
        while (true) {
            // prepare the arguments of the poll()
            poll_args.clear();
            // for convenience, the listening fd is put in the first position
            struct pollfd pfd = {fd, POLLIN, 0};
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

            // process active connections
            for (size_t i = 1; i < poll_args.size(); ++i) {
                if (poll_args[i].revents) {
                    fd2conn[poll_args[i].fd]->connection_io();
                    // client closed normally, or something bad happened.
                    // destroy this connection
                    if (fd2conn[poll_args[i].fd]->is_end()) {
                        fd2conn.erase(poll_args[i].fd);
                    }
                }
            }

            // try to accept a new connection if the listening fd is active
            if (poll_args[0].revents) { (void)accept_new_conn(fd); }
        }
    }
};

int main() {
    EventEngine event_engine{};

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { err("socket()"); }

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0); // wildcard address 0.0.0.0
    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv) { err("bind()"); }

    // listen
    rv = listen(fd, SOMAXCONN);
    if (rv) { err("listen()"); }

    event_engine.join(fd);

    return 0;
}