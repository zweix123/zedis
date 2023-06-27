#pragma once

// C header files
#include <errno.h> // global errno var

// C header files of syscall
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

// C header files of net syscall
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

// C++ header files
#include <cassert>
#include <memory>
#include <iostream>
#include <cstring>

#include <vector>
#include <string>
#include <map>

// custom header files
#include "print.h"

void err(const char *msg) {
    std::perror(msg);
    std::abort();
}

void msg(const char *msg) {
    std::perror(msg);
}

//
const size_t k_max_msg = 4096;

#define DEBUG
// #ifdef DEBUG
// #endif