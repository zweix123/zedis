#include "client.h"
#include "common.h"

int main(int argc, char **argv) {
    zedis::Client client{};
    std::vector<std::string> cmd;
    for (int i = 1; i < argc; ++i) { cmd.push_back(argv[i]); }

    client.send(cmd);
    client.receive();

    return 0;
}