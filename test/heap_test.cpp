#include "common.h"
#include "heap.h"
#include "zand.h"
#include <iostream>

const int N = 10000;
size_t a[N];

zedis::Heap heap;

int main() {
    for (unsigned long &i : a) { heap.push(g_rand_int(0, N << 2), &i); }
    heap.check();
    std::cout << "heap struct \033[92mpass!\033[0m\n";

    // TODO: 和heap关联的变量是正确的嘛?

    return 0;
}