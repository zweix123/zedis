#pragma once

// C header files
#include <errno.h> // global errno var

// C header files of syscall
#include <unistd.h>

// C header files of net syscall
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

// C++ header files
#include <iostream>
#include <cstring>

// custom header files
#include "print.h"
#include "panic.h"