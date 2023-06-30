#include "common.h"

#include <vector>
#include <cstddef> // std::byte
#include <string_view>

#include <cstring>

namespace zedis {

class File;

class Bytes {
  private:
    std::vector<std::byte> data;
    std::size_t pos = 0;
    friend class File;

  public:
    // 构造函数
    Bytes() = default;

    // 将字符串追加到字节数组后面
    void appendString(const std::string &str) {
        const auto size = str.size();
        const auto data_size = data.size();
        data.resize(data_size + size);
        std::memcpy(data.data() + data_size, str.data(), size);
    }

    // 将数字转换为字节数组追加到字节数组后面
    template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
    void appendNumber(const T &num, int N) {
        const auto data_size = data.size();
        data.resize(data_size + N);
        std::memcpy(data.data() + data_size, &num, N);
    }

    // 从当前位置开始获取指定长度的字符串
    std::string_view getStringView(int len) {
        const auto data_size = data.size();
        const auto read_size =
            std::min(data_size - pos, static_cast<std::size_t>(len));
        auto view = std::string_view(
            reinterpret_cast<const char *>(data.data() + pos), read_size);
        pos += read_size;
        return view;
    }

    // 从当前位置开始获取指定长度的数字
    template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
    T getNumber(int N) {
        T num;
        std::memcpy(&num, data.data() + pos, N);
        pos += N;
        return num;
    }
    int getPos() const { return pos; }
    std::byte getPosValue() const { return data[pos]; }
    friend std::ostream &operator<<(std::ostream &os, const Bytes &bytes) {
        os << "=======================================\n";
        int num = 0;
        for (const auto &byte : bytes.data) {
            os << byte << ((++num) % 4 == 0 ? ",\n" : ", ");
        }
        os << "=======================================";
        return os;
    }
};

} // namespace zedis
