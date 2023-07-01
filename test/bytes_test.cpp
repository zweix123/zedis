#include "common.h"
#include "bytes.h"

int main() {
    zedis::Bytes data;
    data.appendString("123");
    data.appendNumber<int>(-1, sizeof(int));
    data.appendNumber<char>('1', sizeof(char));

    auto s1 = data.getStringView(3);
    std::cout << "s1 = " << s1 << "\n";

    auto n1 = data.getNumber<int>(sizeof(int));
    std::cout << "n1 = " << n1 << "\n";

    auto n2 = data.getNumber<char>(sizeof(char));
    std::cout << "n2 = " << n2 << "\n";

    std::cout << data << "\n";
    return 0;
}