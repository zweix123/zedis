#include "common.h"
#include "execute.h"

#include <vector>
#include <string>
#include <initializer_list>

using std::vector;
using std::string;
using namespace zedis;

void parse(Bytes &buff, std::string pre = "") {
    auto type = static_cast<SerType>(buff.getNumber<int>(1));
    // std::cout << type << " ";

    uint32_t str_len, arr_len;
    std::string_view str, msg;
    int64_t flag;
    CmdErr res_code;
    double val;

    switch (type) {
        case SerType::SER_NIL:
            std::cout << pre << "[nil]\n";
            break;
        case SerType::SER_ERR:
            res_code = static_cast<CmdErr>(buff.getNumber<uint32_t>(4));
            str_len = buff.getNumber<uint32_t>(4);
            msg = buff.getStringView(str_len);
            std::cout << pre << "[err]: " << res_code << " " << msg << "\n";
            break;
        case SerType::SER_STR:
            str_len = buff.getNumber<uint32_t>(4);
            str = buff.getStringView(str_len);
            std::cout << pre << "[str]: " << str << "\n";
            break;
        case SerType::SER_INT:
            flag = buff.getNumber<int64_t>(8);
            std::cout << pre << "[int]: " << flag << "\n";
            break;
        case SerType::SER_DBL:
            val = buff.getNumber<double>(8);
            std::cout << pre << "[dbl]: " << val << "\n";
            break;
        case SerType::SER_ARR:
            arr_len = buff.getNumber<uint32_t>(4);
            std::cout << pre << "[arr]: len = " << arr_len << "\n";
            for (int i = 0; i < arr_len; ++i) { parse(buff, pre + "  "); }
            break;
        default:
            msg("bad response");
    }
}

void work(std::initializer_list<std::string> args) {
    std::cout << "\033[32m";
    for (auto arg : args) std::cout << arg << " ";
    std::cout << "\033[0m\n";
    vector<string> cmd = args;
    Bytes buf;
    interpret(cmd, buf);
    parse(buf);
}

int main() {
    work({"zscore", "asdf", "n1"});
    work({"zquery", "xxx", "1", "asdf", "1", "10"});
    work({"zadd", "zset", "1", "n1"});
    work({"zadd", "zset", "2", "n2"});
    work({"zadd", "zset", "1.1", "n1"});
    work({"zscore", "zset", "n1"});
    work({"zquery", "zset", "1", "", "0", "10"});
    work({"zquery", "zset", "1.1", "", "1", "10"});
    work({"zquery", "zset", "1.1", "", "2", "10"});
    work({"zrem", "zset", "adsf"});
    work({"zrem", "zset", "n1"});
    work({"zquery", "zset", "1", "", "0", "10"});
    work({"set", "k", "1.2"});
    work({"get", "k"});
    work({"keys"});
    return 0;
}
