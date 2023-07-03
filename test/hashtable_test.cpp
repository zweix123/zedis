#include "common.h"
#include "interpret.h"

int main() {
    zedis::interpreter::set("key1", "val1");
    zedis::interpreter::set("key2", "val2");

    auto get1 = zedis::interpreter::get("key1");
    if (get1.has_value()) std::cout << "get 1 -> " << get1.value() << "\n";
    else
        std::cout << "get 1 get none\n";

    auto get3 = zedis::interpreter::get("key3");
    if (get3.has_value()) std::cout << "get 3 -> " << get3.value() << "\n";
    else
        std::cout << "get 3 get none\n";

    zedis::interpreter::del("key2");
    auto get2 = zedis::interpreter::get("key2");
    if (get2.has_value()) std::cout << "get 2 -> " << get2.value() << "\n";
    else
        std::cout << "get 2 get none\n";

    zedis::interpreter::set("key1", "val2");
    get1 = zedis::interpreter::get("key1");
    if (get1.has_value())
        std::cout << "修改后 get 1 -> " << get1.value() << "\n";
    else
        std::cout << "修改后 get 1 get none\n";

    zedis::interpreter::set("zweix_k", "zweix_v");

    zedis::Bytes buf;
    buf.appendNumber(zedis::interpreter::m_map.size(), 4);
    zedis::interpreter::scan(buf);

    auto num = buf.getNumber<size_t>(4);
    for (int i = 0; i < num; ++i) {
        auto len = buf.getNumber<std::size_t>(4);
        auto s = buf.getStringView(len);
        std::cout << "key: \"" << s << "\"\n";
    }

    return 0;
}