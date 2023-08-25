#include "cpp_thread_pool.h"
#include <cassert>

void test1() {
    zedis::threadpool pool;
    pool.init(3);
    std::vector<std::future<int>> int_futs;
    std::vector<std::future<bool>> bool_futs;
    for (int i = 0; i != 30; ++i) {
        int_futs.emplace_back(pool.async([](int i) -> int { return i; }, i));
    }
    for (int i = 0; i != 30; ++i) {
        bool_futs.emplace_back(
            pool.async([i]() -> bool { return i % 2 == 0; }));
    }
    // pool.terminate();

    // assert(pool.inited());
    // assert(pool.is_running());
    for (int i = 0; i != 30; ++i) {
        assert(int_futs[i].get() == i);
        assert(bool_futs[i].get() == (i % 2 == 0));
    }
}

int main() {
    test1();

    return 0;
}