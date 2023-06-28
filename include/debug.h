#pragma once

#include "common.h"
#include <iomanip>
#include <sstream>
#include <type_traits>
#include <typeinfo>

std::string block2string(const char *block) {
    return "";
    std::stringstream ss;

    ss << "{";

    uint32_t block_len = 0;
    memcpy(&block_len, block, 4);
    if (block_len < 4) return "-1";

    uint32_t cmd_num = 0;
    memcpy(&cmd_num, &block[4], 4);

    ss << "\033[93m" << block_len << "\033[0mb, ";
    ss << "\033[93m" << cmd_num << "\033[0mc, ";

    ss << "[";
    size_t cur = 8;
    for (int i = 0; i < cmd_num; ++i) {
        if (cur + 4 > block_len) return "-1";
        uint32_t cmd_len = 0;
        memcpy(&cmd_len, &block[cur], 4);
        std::string_view cmd{
            reinterpret_cast<const char *>(block + cur + 4), cmd_len};

        ss << "\033[93m" << std::setw(2) << cmd_len << "\033[0m \"" << cmd
           << "\"\033[0m" << (i == cmd_num - 1 ? " " : ", ");

        cur += 4 + cmd_len;
    }
    ss << "]";

    assert(cur == block_len + 4);
    ss << "}";

    return ss.str();
}

std::string block2string(const uint8_t *block) {
    return "";
    std::stringstream ss;

    ss << "{";

    uint32_t block_len = 0;
    memcpy(&block_len, block, 4);

    uint32_t cmd_num = 0;
    memcpy(&cmd_num, block + 4, 4);

    ss << "\033[93m" << block_len << "\033[0mb, ";
    ss << "\033[93m" << cmd_num << "\033[0mc, ";

    ss << "[";
    size_t cur = 8;
    for (int i = 0; i < cmd_num; ++i) {
        uint32_t cmd_len = 0;
        memcpy(&cmd_len, block + cur, 4);
        std::string_view cmd{
            reinterpret_cast<const char *>(block + cur + 4), cmd_len};

        ss << "\033[93m" << std::setw(2) << cmd_len << "\033[0m \"" << cmd
           << "\"\033[0m" << (i == cmd_num - 1 ? " " : ", ");

        cur += 4 + cmd_len;
    }
    ss << "]";

    assert(cur == block_len + 4);
    ss << "}";

    return ss.str();
}

template<typename T>
std::ostream &operator<<(
    typename std::enable_if<std::is_enum<T>::value, std::ostream>::type
        &ostream,
    const T &e) {
    return ostream << "\033[93m" << typeid(e).name() << "\033[0m::"
                   << "\033[36m"
                   << static_cast<typename std::underlying_type<T>::type>(e)
                   << "\033[0m";
}