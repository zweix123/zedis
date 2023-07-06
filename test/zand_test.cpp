#include "common.h"
#include "zand.h"

int main() {
    for (int i = 1; i <= 42; ++i)
        std::cout << g_rand_int(0, i) << " " << g_rand_str() << "\n";
    return 0;
}