#pragma once

// // C header files of syscall
// #include <unistd.h>     // POSIX syscall
// #include <fcntl.h>      // file control syscall
// #include <sys/socket.h> // socket syscall
// #include <poll.h>       // poll syscall

// // C header files
// #include <arpa/inet.h>  // net address transform
// #include <netinet/ip.h> // ip data strcut

// C++ header files
#include <cassert> // assert

// global constant
namespace zedis {
inline constexpr std::size_t k_max_msg = 4096 + 4;
}

// #define DEBUG

// custom header files
#include "panic.h"
#include "zstream.h"