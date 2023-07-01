#pragma once

// C++ header files
#include <cassert> // assert
#include <memory>  // smart pointer

// global constant
namespace zedis {
inline constexpr std::size_t k_max_msg = 4096 + 4;
}

#define DEBUG

// custom header files
#include "panic.h"
#include "zstream.h"