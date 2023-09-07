#include "server.h"
#include "common.h"

int main() {
    zedis::Server server{};
    server.join();

    return 0;
}