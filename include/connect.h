#pragma once

#include "common.h"
#include "file.h"
#include "interpret.h"

namespace zedis {
enum class ConnState {
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2,
};
class Conn {
  private:
    File m_f;
    ConnState m_state;
    Bytes rbuf, wbuf;

  public:
    void check() { m_f.check(); }
    explicit Conn(File &&f, ConnState conn_state)
        : m_f(std::move(f)), m_state{conn_state}, rbuf{}, wbuf{} {}
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
#ifdef DEBUG
        std::cout << "entet connect io, state: " << m_state << "\n";
#endif
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
    }
    bool try_fill_buffer() {
#ifdef DEBUG
        std::cout << "entet try_fill_buffer\n";
#endif
        auto rv = m_f.readByte(rbuf, 4);
        if (rv < 0 && errno == EAGAIN) { return false; }
        if (rv < 0) {
            msg("read() error");
            m_state = ConnState::STATE_END;
            return false;
        }
        if (rv == 0) {
            if (rbuf.size() > 0) msg("unexpected EOF");
            else
                msg("EOF");
            m_state = ConnState::STATE_END;
            return false;
        }
        assert(rbuf.size() == 4);
        auto len = rbuf.getNumber<uint32_t>(4);
        rv = m_f.readByte(rbuf, len);
        while (try_one_request()) {}
        return (m_state == ConnState::STATE_REQ);
    }

    bool try_one_request() {
        uint32_t len;

        if (rbuf.size() < 4) return false;

        // uint32_t len = rbuf.getNumber<uint32_t>(4);

        // assert(4 + len == rbuf.size());
        // if (4 + len > rbuf.size()) return false;

        std::vector<std::string> cmds;
        // int32_t rv = parse_req(rbuf, cmds);
        auto &data = rbuf;
        auto cmd_num = data.getNumber<uint32_t>(4);
        while (cmd_num--) {
            auto cmd_len = data.getNumber<uint32_t>(4);
            cmds.emplace_back(data.getStringView(cmd_len));
        }

        auto [res_code, res_msg] = interpret(cmds);
        // 错误未处理,
        // int32_t err = do_request(&rbuf[4], len, &rescode, &wbuf[4 + 4],
        // &wlen); if (err) {
        //     state = ConnState::STATE_END;
        //     return false;
        // }

        //

        len = 4 + 4 + res_msg.size();
        std::cout << "res_msg = " << res_msg << "\n";
        std::cout << "wbuf len -> " << len << "\n";
        wbuf.appendNumber(len, 4);
        wbuf.appendNumber(static_cast<int>(res_code), 4);
        wbuf.appendString(res_msg);

        uint32_t rescode = 0;
        uint32_t wlen = 0;

        m_state = ConnState::STATE_RES;
        state_response();

        return (m_state == ConnState::STATE_REQ);
    }

    void state_response() {
        while (try_flush_buffer()) {}
    }

    bool try_flush_buffer() {
        ssize_t rv = m_f.writeByte(wbuf);

        if (rv < 0 && errno == EAGAIN) return false;
        if (rv < 0) {
            msg("write() error");
            m_state = ConnState::STATE_END;
            return false;
        }
        if (rv == wbuf.size()) {
            m_state = ConnState::STATE_REQ;
            rbuf.clear();
            wbuf.clear();
            return false;
        }
        return true;
    }
};
} // namespace zedis