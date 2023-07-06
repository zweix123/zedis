#pragma once

#include "common.h"

#include <iostream>
#include <cstring>
#include <cstddef> // std::byte
#include <vector>
#include <string_view>

namespace zedis {

class File;

class Bytes {
  private:
    std::vector<std::byte> data{};
    std::size_t pos{0};

    friend class File;

  public:
    Bytes() = default;

    std::size_t size() const { return data.size(); }
    void reset() { pos = 0; }
    void clear() {
        data.clear();
        reset();
    }
    bool is_read_end() const { return pos == data.size(); }

    friend std::ostream &operator<<(std::ostream &os, const Bytes &bytes) {
        std::cout << std::hex;
        for (int i = 0; i < bytes.data.size(); i++)
            os << (i == 0 ? "[" : "") << "0x"
               << std::to_integer<int>(bytes.data[i])
               << (i + 1 == bytes.data.size() ? "]" : ", ");
        std::cout << std::dec;
        os << " len = " << bytes.size();
        return os;
    }

    void appendString(const std::string &str) {
        const auto size = str.size();
        const auto data_size = data.size();
        data.resize(data_size + size);
        std::memcpy(data.data() + data_size, str.data(), size);
    }
    void appendStringView(const std::string_view &str_view) {
        const auto size = str_view.size();
        const auto data_size = data.size();
        data.resize(data_size + size);
        std::memcpy(data.data() + data_size, str_view.data(), size);
    }

    template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
    void appendNumber(const T &num, int N) {
        const auto data_size = data.size();
        data.resize(data_size + N);
        std::memcpy(data.data() + data_size, &num, N);
    }

    void appendBytes_move(Bytes &&other) {
        if (this == &other) { return; }
        data.insert(
            data.end(), std::make_move_iterator(other.data.begin()),
            std::make_move_iterator(other.data.end()));
        other.data.clear();
    }

    std::string_view getStringView(int len) {
        const auto data_size = data.size();
        const auto read_size =
            std::min(data_size - pos, static_cast<std::size_t>(len));
        // if len > remain size, not handle;
        auto view = std::string_view(
            reinterpret_cast<const char *>(data.data() + pos), read_size);
        pos += read_size;
        return view;
    }

    template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
    T getNumber(int N) {
        T num = 0;
        std::memcpy(&num, data.data() + pos, N);
        pos += N;
        return num;
    }
};

} // namespace zedis
