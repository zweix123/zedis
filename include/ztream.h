#pragma once

#include <iostream>
#include <string>
#include <utility>

#include <cstddef>
#include <bitset>
#include <optional>

#include <type_traits>
#include <typeinfo>

std::ostream &operator<<(std::ostream &os, const std::byte &b) {
    return os << std::bitset<8>(std::to_integer<int>(b));
}

template<typename T>
std::ostream &operator<<(
    typename std::enable_if<std::is_enum<T>::value, std::ostream>::type &os,
    const T &e) {
    return os << "\033[93m" << typeid(e).name() << "\033[0m::\033[36m"
              << static_cast<typename std::underlying_type<T>::type>(e)
              << "\033[0m";
}

template<typename T>
std::ostream &operator<<(std::ostream &os, const std::optional<T> &op) {
    if (op.has_value()) os << op.value();
    else
        os << "nil";
    return os;
}

void print_class(const std::string &str) {
    std::cout << ")";
}

template<typename T1, typename T2, typename... Args>
void print_class(const std::string &str, T1 &&arg1, T2 &&arg2, Args &&...args) {
    static_assert(
        sizeof...(args) % 2 == 0, "The number of arguments must be even.");
    if (str.size()) std::cout << "\033[33m" << str << "\033[0m( ";

    std::cout << "\033[34m" << arg1 << "\033[0m"
              << " = " << arg2 << " ";

    print_class("", std::forward<Args>(args)...);
}

// template<typename... Args>
// void print_class(const std::string &str, Args &&...args) {
//     static_assert(
//         sizeof...(args) % 2 == 0, "The number of arguments must be even.");
//     if (str.size()) std::cout << "\033[33m" << str << "\033[0m( ";
//     ((std::cout << "\033[34m" << std::forward<Args>(args) << "\033[0m"
//                 << " = " << std::forward<Args>(std::next(args)) << " "),
//      ...);
//     std::cout << ")";
// }