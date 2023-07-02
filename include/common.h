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
enum class CmdRes {
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2, // not exist
};
enum class ConnState {
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2,
};

}

#define DEBUG

// custom header files
#include "panic.h"
#include "zstream.h"