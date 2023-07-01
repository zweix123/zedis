#include "common.h"
#include "client.h"

int main() {
    zedis::Client client{};
    std::vector<std::string> cmds = {"set", "k", "v"};
    client.send(cmds);
    return 0;
}