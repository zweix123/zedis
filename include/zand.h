#pragma once

#include "common.h"
#include <string>  // 包含std::string类型和相关函数
#include <cstdlib> // 包含srand()和rand()函数
#include <ctime>   // 包含time()函数
#include <random> // 包含std::random_device、std::mt19937和std::uniform_int_distribution<>等类型和相关函数

int g_rand_int(int l, int r) {
    assert(l <= r);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(l, r);
    return dis(gen);
}

std::string g_rand_str(int length) {
    static const char alphanum[] = "0123456789"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz";
    static const int alphanum_len = sizeof(alphanum);

    std::string random_string;
    while (length--) random_string += alphanum[g_rand_int(0, alphanum_len)];
    return random_string;
}

std::string g_rand_str() {
    return g_rand_str(g_rand_int(5, 42));
}
