#include "common.h"
#include "interpret.h"
#include "zand.h"
#include <vector>
#include <string>
#include <set>

int base = 420;

void scan() {
    zedis::Bytes buf;
    buf.appendNumber(zedis::interpreter::m_map.size(), 4);
    zedis::interpreter::scan(buf);

    auto num = buf.getNumber<size_t>(4);
    for (int i = 0; i < num; ++i) {
        (void)buf.getNumber<char>(1);
        auto len = buf.getNumber<std::size_t>(4);
        auto s = buf.getStringView(len);
        std::cout << "key: \"" << s << "\"\n";
    }
}

std::set<std::string> get_all() {
    std::set<std::string> res;

    zedis::Bytes buf;
    buf.appendNumber(zedis::interpreter::m_map.size(), 4);
    zedis::interpreter::scan(buf);

    auto num = buf.getNumber<size_t>(4);
    for (int i = 0; i < num; ++i) {
        (void)buf.getNumber<char>(1);
        auto len = buf.getNumber<std::size_t>(4);
        res.insert(std::string(buf.getStringView(len)));
    }

    return res;
}

void hashtable_verify(const std::set<std::string> &ref) {
    // auto a = get_all();
    // std::cout << a.size() << "\n";
    // for (auto t : a) std::cout << "  " << t << "\n";
    assert(get_all() == ref);
}

int main() {
    // zedis::interpreter::scan();
    // exit(0);
    // some quick tests
    hashtable_verify({});
    zedis::interpreter::set("123", "123");
    hashtable_verify({"123"});
    assert(!zedis::interpreter::del("124"));
    assert(zedis::interpreter::del("123"));
    hashtable_verify({});
    std::cout << "base test \033[92mpass!\033[0m\n";

    // sequential insertion
    std::set<std::string> ref;
    for (int i = 0; i < base; i++) {
        auto k = g_rand_str();
        auto v = g_rand_str();
        zedis::interpreter::set(k, v);
        ref.insert(k);
    }
    hashtable_verify(ref);
    std::cout << "sequential insertion \033[92mpass!\033[0m\n";

    // random deletion
    int num = 0;
    for (int i = 0; i < base / 2; ++i) {
        auto s = g_rand_str();
        auto it = ref.find(s);
        if (it == ref.end()) {
            assert(!zedis::interpreter::del(s));
        } else {
            ++num;
            assert(zedis::interpreter::del(s));
            ref.erase(s);
        }
        hashtable_verify(ref);
    }
    std::cout << "random deletion \033[92mpass!\033[0m\n";

    num = base / 2 - num;
    while (num--) {
        auto it = ref.begin();
        std::advance(it, g_rand_int(0, ref.size() - 1));
        auto s = *it;
        assert(zedis::interpreter::del(s));
        ref.erase(*it);
    }
    // std::cout << num << ", " << base / 2 << "\n";
    std::cout << "deletion \033[92mpass!\033[0m\n";

    zedis::interpreter::dispose();

    std::cout << "\033[94mtest all pass\033[0m\n";

    return 0;
}