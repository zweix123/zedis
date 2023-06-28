#include "common.h"

static int32_t send_req(int fd, const std::vector<std::string> &cmd) {
    uint32_t len = 4;
    for (const std::string &s : cmd) { len += 4 + s.size(); }
    if (len > k_max_msg) { return -1; }

    char wbuf[4 + k_max_msg];
    memcpy(&wbuf[0], &len, 4);
    uint32_t n = cmd.size();
    memcpy(&wbuf[4], &n, 4);
    size_t cur = 8;
    for (const std::string &s : cmd) {
        uint32_t p = (uint32_t)s.size();
        memcpy(&wbuf[cur], &p, 4);
        memcpy(&wbuf[cur + 4], s.data(), s.size());
        cur += 4 + s.size();
    }
#ifdef DEBUG
    std::cout << "block: " << block2string(wbuf) << "\n";
#endif
    return write_all(fd, wbuf, 4 + len);
}

static int32_t read_res(int fd) {
    errno = 0;

    char rbuf[4 + k_max_msg + 1];
#ifdef DEBUG
    std::cout << "start 4 byte read\n";
#endif
    int32_t err = read_full(fd, rbuf, 4);
#ifdef DEBUG
    std::cout << "4 byte read end\n";
#endif
    if (err) {
        errno == 0 ? msg("EOF") : msg("read() error");
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, rbuf, 4);
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }
#ifdef DEBUG
    std::cout << "len = " << len << "\n";
#endif

#ifdef DEBUG
    std::cout << "start body read\n";
#endif
    err = read_full(fd, &rbuf[4], len);
#ifdef DEBUG
    std::cout << "read body end\n";
#endif
    if (err) {
        msg("read() error");
        return err;
    }

    // print the result
    uint32_t rescode = 0;
    if (len < 4) {
        msg("bad response");
        return -1;
    }
    memcpy(&rescode, &rbuf[4], 4);
    printf("server says: [%u] %.*s\n", rescode, len - 4, &rbuf[8]);
    return 0;
}

int main(int argc, char **argv) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { err("socket()"); }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // 127.0.0.1
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) { err("connect"); }

    std::vector<std::string> cmd;
    for (int i = 1; i < argc; ++i) { cmd.push_back(argv[i]); }
#ifdef DEBUG
    std::cout << "开始发送\n";
#endif
    int32_t err = send_req(fd, cmd);
#ifdef DEBUG
    std::cout << "发送完毕\n";
#endif
    if (err) { goto L_DONE; }
#ifdef DEBUG
    std::cout << "开始接收\n";
#endif
    err = read_res(fd);
#ifdef DEBUG
    std::cout << "接受完毕\n";
#endif
    if (err) { goto L_DONE; }

L_DONE:
    close(fd);
    return 0;
}