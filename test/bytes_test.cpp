#include "common.h"
#include "bytes.h"

int main() {
    // zedis::Bytes data;
    // data.appendString("123");
    // data.appendNumber<int>(-1, sizeof(int));
    // data.appendNumber<char>('1', sizeof(char));

    // auto s1 = data.getStringView(3);
    // std::cout << "s1 = " << s1 << "\n";

    // auto n1 = data.getNumber<int>(sizeof(int));
    // std::cout << "n1 = " << n1 << "\n";

    // auto n2 = data.getNumber<char>(sizeof(char));
    // std::cout << "n2 = " << n2 << "\n";

    // std::cout << data << "\n";

    // std::cout << "test move\n";
    // zedis::Bytes data_;
    // data_.appendBytes_move(std::move(data));
    // std::cout << "data.size() -> " << data.size() << "\n";
    // std::cout << "data_.size() -> " << data_.size() << "\n";
    zedis::Bytes buff;
    std::vector<std::string> cmds = {"set", "k", "v"};
    uint32_t len = 4;
    for (const std::string &cmd : cmds) { len += 4 + cmd.size(); }

    buff.appendNumber(len, 4);

    uint32_t n = cmds.size();
    buff.appendNumber(n, 4);

    for (const std::string &s : cmds) {
        uint32_t p = (uint32_t)s.size();
        buff.appendNumber(p, 4);
        buff.appendString(s);
    }

    std::cout << "client send buff : " << buff << "\n";

    return 0;
}