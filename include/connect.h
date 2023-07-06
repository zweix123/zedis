#pragma once

#include "common.h"
#include "file.h"
#include "interpret.h"

#include <poll.h> // poll syscall

namespace zedis {

class Conn {
  private:
    File m_f;
    ConnState m_state;
    Bytes rbuf, wbuf;

  public:
    void check() { m_f.check(); }
    explicit Conn(File &&f, ConnState conn_state)
        : m_f(std::move(f)), m_state{conn_state}, rbuf{}, wbuf{} {
        m_f.set_nb();
    }
    int get_fd() const { return m_f.data(); }
    short int get_event() const {
        if (m_state == ConnState::STATE_REQ) return POLLIN;
        else if (m_state == ConnState::STATE_RES)
            return POLLOUT;
        else
            assert(0);
    }
    bool is_end() const { return m_state == ConnState::STATE_END; }

    void connection_io() {
        if (m_state == ConnState::STATE_REQ) {
            state_request();
        } else if (m_state == ConnState::STATE_RES) {
            state_response();
        } else {
            assert(0);
        }
    }

    void state_request() {
        while (try_fill_buffer()) {}
        //         while (try_fill_buffer()) {}
    }
    bool try_fill_buffer() {
        do {
            auto rv = m_f.readByte_nb(rbuf, 4);
            switch (rv) {
                case -1:
                    msg("nb read() error");
                    m_state = ConnState::STATE_END;
                    return false;
                case 0:
                    break;
                case 1:
                    return false;
                case 2:
                    rbuf.size() ? msg("unexpected EOF") : msg("EOF");
                    m_state = ConnState::STATE_END;
                    return false;
                default:
                    break;
            }
            auto len = rbuf.getNumber<uint32_t>(4);
            rv = m_f.readByte_nb(rbuf, len);
            switch (rv) {
                case -1:
                    msg("nb read() error");
                    m_state = ConnState::STATE_END;
                    return false;
                case 0:
                    break;
                case 1:
                    return false;
                case 2:
                    rbuf.size() ? msg("unexpected EOF") : msg("EOF");
                    m_state = ConnState::STATE_END;
                    return false;
                default:
                    break;
            }
        } while (try_one_request());
        return (m_state == ConnState::STATE_REQ);
    }

    bool try_one_request() {
        std::vector<std::string> cmd;
        auto ok = parse_req(rbuf, cmd);
        if (!ok) {
            msg("bad req");
            m_state = ConnState::STATE_END;
            return false;
        }
        // 数据从Bytes中**拷贝**进cmd中
        Bytes out;
        interpret(cmd, out);

        wbuf.appendNumber(out.size(), 4);
        wbuf.appendBytes_move(std::move(out));

        m_state = ConnState::STATE_RES;

        state_response();
        return (m_state == ConnState::STATE_REQ);
    }

    void state_response() {
        while (try_flush_buffer()) {}
    }

    bool try_flush_buffer() {
        auto rv = m_f.writeByte_nb(wbuf);
        switch (rv) {
            case -1:
                msg("nb write() error");
                m_state = ConnState::STATE_END;
                return false;
            case 0:
                break;
            case 1:
                return false;
            default:
                break;
        }
        if (wbuf.is_read_end()) {
            m_state = ConnState::STATE_REQ;
            rbuf.clear();
            wbuf.clear();
            return false;
        }

        return true;
    }
};

} // namespace zedis