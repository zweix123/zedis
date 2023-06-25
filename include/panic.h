#pragma once

#include "common.h"

void err(const char *msg) {
    std::perror(msg);
    std::abort();
}

void msg(const char *msg) {
    std::perror(msg);
}
