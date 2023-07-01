#include "common.h"
#include "server.h"

int main() {
    zedis::Server server{};
    auto one = std::move(server.acceptConn());
    one->receive();
    return 0;
}