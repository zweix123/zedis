#include "common.h"
#include "hashtable.h"

int main() {
    zedis::Map map;

    map.set("key1", "val1");
    map.set("key2", "val2");

    auto get1 = map.get("key1");
    if (get1.has_value()) std::cout << "get 1 -> " << get1.value() << "\n";
    else
        std::cout << "get 1 get none\n";

    auto get3 = map.get("key3");
    if (get3.has_value()) std::cout << "get 3 -> " << get3.value() << "\n";
    else
        std::cout << "get 3 get none\n";

    map.del("key2");
    auto get2 = map.get("key2");
    if (get2.has_value()) std::cout << "get 2 -> " << get2.value() << "\n";
    else
        std::cout << "get 2 get none\n";

    return 0;
}