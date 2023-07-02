#pragma once

#include "common.h"
#include "file.h"

#include <sys/socket.h> // socket syscall
#include <arpa/inet.h>  // net address transform
#include <netinet/ip.h> // ip data strcut

#include <vector>
#include <string>

namespace zedis {
class Client {
  private:
    File m_f;

  public:
    Client() : m_f{make_socket()} {}

    int make_socket() {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) { err("socket()"); }

        struct sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_port = ntohs(1234);
        addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // 127.0.0.1
        int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
        if (rv) { err("connect"); }

        return fd;
    }

    void send(const std::vector<std::string> &cmds) {
        Bytes buff;

        uint32_t len = 4;
        for (const std::string &cmd : cmds) { len += 4 + cmd.size(); }

        buff.appendNumber(len, 4);
        uint32_t n = cmds.size();
        buff.appendNumber(n, 4);

        for (const std::string &s : cmds) {
            uint32_t p = (uint32_t)s.size();
            buff.appendNumber(p, 4);
            buff.appendString(s);
        }
        int32_t rv = m_f.writeByte(buff);

        assert(rv == buff.size()); // err not handle
    }

    void receive() {
        Bytes buff;
        auto rv = m_f.readByte(buff, 4 + 4);
        assert(rv == 8);
        auto len = buff.getNumber<int32_t>(4);
        auto res_code = static_cast<CmdRes>(buff.getNumber<int>(4));
        auto msg_len = len - 8;
        rv = m_f.readByte(buff, msg_len);
        assert(rv == msg_len);
        auto msg = buff.getStringView(msg_len);
        std::cout << "[" << res_code << "]: " << msg << "\n";
    }
};

} // namespace zedis