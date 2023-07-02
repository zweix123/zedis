#include "common.h"
#include "client.h"

int main() {
    zedis::Client client{};
    std::vector<std::string> cmds1 = {"set", "k", "v"};
    auto rv = client.send(cmds1);
    std::cout << "rv = " << rv << "\n";
    client.receive();
    // std::vector<std::string> cmds2 = {"get", "k"};
    // client.send(cmds2);
    // client.receive();
    return 0;
}