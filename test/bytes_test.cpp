#include "common.h"
#include "bytes.h"

int main() {
    zedis::Bytes byte;
    double da = 123.456;
    byte.appendNumber(da, 8);
    auto daa = byte.getNumber<double>(8);
    // std::cout << "a = " << a << ", aa = " << aa << "\n";
    auto diff = std::abs(da - daa);
    assert(diff < 0.0001);

    byte.clear();
    int ia = 1, ib = 2;
    byte.appendNumber(ia, 4);
    byte.appendNumber(ib, 4);
    int ic = 3;
    byte.insertNumber(ic, 0, 4);
    int iaa = byte.getNumber<int>(4), ibb = byte.getNumber<int>(4);
    assert(iaa == ic);
    assert(ibb == ib);

    return 0;
}