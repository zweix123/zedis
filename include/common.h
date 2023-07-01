#pragma once

// C++ header files
#include <cassert> // assert
#include <memory>  // smart pointer

// global constant
namespace zedis {
// 容器长度相关      -> std::size_t
// 系统调用返回值相关 -> ssize_t
// 传输协议相关      -> int32_t
// 其他             -> ints
}

#define DEBUG

// custom header files
#include "panic.h"
#include "zstream.h"