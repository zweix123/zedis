#pragma once

#include <errno.h> // global errno var
#include <cstdlib>
#include <cstdio>

void err(const char *msg) {
    std::perror(msg);
    std::abort();
}

void msg(const char *msg) {
    std::perror(msg);
}