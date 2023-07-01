#pragma once

#include "common.h"
#include "file.h"

namespace zedis {
enum class ConnState {
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2,
};
class Conn {
  private:
    File m_f;
    ConnState m_conn_state;

  public:
    void check() { m_f.check(); }
    explicit Conn(File &&f, ConnState conn_state)
        : m_f(std::move(f)), m_conn_state{conn_state} {}
    int32_t receive() { //  state_request
        Bytes buf;
        auto rv = m_f.readByte(buf, 4);
        uint32_t len = buf.getNumber<uint32_t>(4);
        m_f.readByte(buf, 4);
        uint32_t n = buf.getNumber<uint32_t>(4);

        for (int i = 0; i < n; i++) {
            m_f.readByte(buf, 4);
            len = buf.getNumber<uint32_t>(4);
            m_f.readByte(buf, len);
            auto cmd = buf.getStringView(len);
            std::cout << cmd << "\n";
        }

        return 0;
    }
};
} // namespace zedis