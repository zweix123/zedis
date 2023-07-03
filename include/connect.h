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
        int loop_num = 0;
        while (try_fill_buffer()) {
        }
    }
    bool try_fill_buffer() {
        auto rv = m_f.readByte(rbuf, 4);
        if (rv < 0 && errno == EAGAIN) { return false; }
        if (rv < 0) {
            err("read() error");
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
        assert(rv == len);

        int loop_num = 0;
        while (try_one_request()) {
        }
        return (m_state == ConnState::STATE_REQ);
    }

    bool try_one_request() {
        if (rbuf.size() == 0) return false;
        // assert(len + 4 == rbuf.size());
        // len had handled

        std::vector<std::string_view> cmds;
        parse_req(rbuf, cmds);

        Bytes out;
        interpret(cmds, out);

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