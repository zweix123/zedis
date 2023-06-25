#pragma one

#include <cstdint>  // type
#include <cstdio>   // std io
#include <cstdlib>  // std func
#include <unistd.h> // system call
#include <cerrno>   // global variable errno

void err(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort(); // C std function
}

void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

#include "print.h"