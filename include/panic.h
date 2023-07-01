#pragma once

#include <errno.h> // global errno var
#include <cstdlib>
#include <cstdio>

void ERR(const char *file, const char *func, int line, const char *msg) {
    std::fprintf(stderr, " %s:%d %s: %s\n", file, line, func, msg);
    std::abort();
}

void MSG(const char *file, const char *func, int line, const char *msg) {
    std::fprintf(stderr, " %s:%d %s: %s\n", file, line, func, msg);
}

#define err(msg) ERR(__FILE__, __func__, __LINE__, msg)
#define msg(msg) MSG(__FILE__, __func__, __LINE__, msg)