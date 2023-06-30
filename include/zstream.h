#pragma once

#include <cstddef>
#include <bitset>

#include <iostream>
#include <type_traits>

#include <typeinfo>

std::ostream &operator<<(std::ostream &os, std::byte b) {
    return os << std::bitset<8>(std::to_integer<int>(b));
}
template<typename T>
std::ostream &operator<<(
    typename std::enable_if<std::is_enum<T>::value, std::ostream>::type &os,
    const T &e) {
    return os << "\033[93m" << typeid(e).name() << "\033[0m::"
              << "\033[36m"
              << static_cast<typename std::underlying_type<T>::type>(e)
              << "\033[0m";
}