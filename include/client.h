#pragma once

#include "common.h"
#include "file.h"
#include "interpret.h"

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

    int32_t send(const std::vector<std::string> &cmds) {
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

        return rv;
    }

    int32_t receive() {
        Bytes buff;
        auto rv = m_f.readByte(buff, 4);
        auto len = buff.getNumber<int32_t>(4);
        rv = m_f.readByte(buff, 4);
        auto res_code = static_cast<CmdRes>(buff.getNumber<int>(4));
        rv = m_f.readByte(buff, len - 8);
        auto msg = buff.getStringView(len - 8);
        std::cout << "res_code: " << res_code << ", " << msg << "\n";

        return 0;
    }
};
} // namespace zedis