#include "common.h"

#include <iostream>
#include <cstring>
#include <cstddef> // std::byte
#include <bitset>
#include <vector>
#include <string_view>

namespace zedis {

class File;

class Bytes {
  private:
    std::vector<std::byte> data;
    std::size_t m_pos = 0, d_pos = 0;
    /* 我们希望对Bytes的任何交互都是有副作用
     * + 内存向Bytes写: 直接在vector的末尾添加, 不同维护
     * + 内存从Bytes读: 由m_pos维护
     * + Bytes向fd写 : 由d_pos维护
     * + Bytes从fd读 : 干嘛干嘛, 你还想一边往里写再一边往里读?
     */
    friend class File;

  public:
    Bytes() = default;

    void reset_m() { m_pos = 0; }
    void reset_d() { d_pos = 0; }
    void reset() {
        reset_m();
        reset_d();
    }
    std::size_t size() const { return data.size(); }
    void clear() {
        data.clear();
        reset();
    }

    friend std::ostream &operator<<(std::ostream &os, const Bytes &bytes) {
        for (int i = 0; i < bytes.data.size(); i++)
            os << (i == 0 ? "[" : "") << "0x" << std::hex
               << std::to_integer<int>(bytes.data[i])
               << (i + 1 == bytes.data.size() ? "]" : ", ");
        return os;
    }

    void appendString(const std::string &str) {
        const auto size = str.size();
        const auto data_size = data.size();
        data.resize(data_size + size);
        std::memcpy(data.data() + data_size, str.data(), size);
    }

    template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
    void appendNumber(const T &num, int N) {
        const auto data_size = data.size();
        data.resize(data_size + N);
        std::memcpy(data.data() + data_size, &num, N);
    }

    std::string_view getStringView(int len) {
        const auto data_size = data.size();
        const auto read_size =
            std::min(data_size - m_pos, static_cast<std::size_t>(len));
        auto view = std::string_view(
            reinterpret_cast<const char *>(data.data() + m_pos), read_size);
        m_pos += read_size;
        return view;
    }

    template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
    T getNumber(int N) {
        T num;
        std::memcpy(&num, data.data() + m_pos, N);
        m_pos += N;
        return num;
    }
};

} // namespace zedis
