#pragma once

// C header files
#include <errno.h> // global errno var

// C header files of syscall
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <poll.h>

// C++ header files
#include <cassert>
#include <memory>
#include <iostream>
#include <cstring>

#include <vector>
#include <string>
#include <map>
#include <unordered_map>

// panic func
void err(const char *msg) {
    std::perror(msg);
    std::abort();
}

void msg(const char *msg) {
    std::perror(msg);
}

// global constant
const size_t k_max_msg = 4096;

// #define DEBUG

// custom header files
#include "util.h"
#include "debug.h"