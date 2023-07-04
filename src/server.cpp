#include "common.h"
#include "server.h"

int main() {
    zedis::Server server{};
    server.join();

    return 0;
}