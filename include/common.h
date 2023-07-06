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
enum class SerType {
    SER_NIL = 0, // Like `NULL`
    SER_ERR = 1, // An error code and message
    SER_STR = 2, // A string
    SER_INT = 3, // A int64
    SER_ARR = 4, // Array
};

#define container_of(ptr, type, member)                    \
    ({                                                     \
        const typeof(((type *)0)->member) *__mptr = (ptr); \
        (type *)((char *)__mptr - offsetof(type, member)); \
    })

} // namespace zedis

#define DEBUG

// custom header files
#include "panic.h"
#include "ztream.h"