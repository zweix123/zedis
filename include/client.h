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
    Client() : m_f{make_socket()} {
        //  m_f.set_nb();
    }

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
        auto ok = m_f.writeByte_b(buff);
        assert(ok);
    }

    void handle_response(Bytes &buff, std::string pre = "") {
        auto type = static_cast<SerType>(buff.getNumber<int>(1));

        uint32_t str_len, arr_len;
        std::string_view str, msg;
        int64_t flag;
        CmdRes res_code;

        switch (type) {
            case SerType::SER_NIL:
                std::cout << pre << "[nil]\n";
                break;
            case SerType::SER_ERR:
                res_code = static_cast<CmdRes>(buff.getNumber<uint32_t>(4));
                str_len = buff.getNumber<uint32_t>(4);
                msg = buff.getStringView(str_len);
                std::cout << pre << "[err]: " << res_code << " " << msg << "\n";
                break;
            case SerType::SER_STR:
                str_len = buff.getNumber<uint32_t>(4);
                str = buff.getStringView(str_len);
                std::cout << pre << "[str]: " << str << "\n";
                break;
            case SerType::SER_INT:
                flag = buff.getNumber<int64_t>(8);
                std::cout << pre << "[int]: " << flag << "\n";
                break;
            case SerType::SER_ARR:
                std::cout << pre << "[arr]: \n";
                arr_len = buff.getNumber<uint32_t>(4);
                for (int i = 0; i < arr_len; ++i) {
                    handle_response(buff, pre + "  ");
                }
                break;
            default:
                msg("bad response");
        }
    }

    void receive() {
        Bytes buff;
        auto ok = m_f.readByte_b(buff, 4);
        assert(ok);
        auto len = buff.getNumber<uint32_t>(4);
        ok = m_f.readByte_b(buff, len);
        assert(ok);
        handle_response(buff);
    }
};

} // namespace zedis