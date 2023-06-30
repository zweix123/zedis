#include "common.h"
#include "file.h"

void test_write_string() {
    zedis::File fo(open("content.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644));
    zedis::Bytes bytes;
    bytes.appendString("hello world");
    fo.writeByte(bytes);
}

void test_read_string() {
    zedis::File fi(open("content.txt", O_RDONLY));
    zedis::Bytes bytes;
    int n = 42;
    auto rv = fi.readByte(bytes, n);
    std::cout << "计划read " << n << ", 实际read " << rv
              << ", 内容为: " << bytes.getStringView(n) << "\n";
}

void test_write_interger() {
    zedis::File fo(open("content.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644));
    zedis::Bytes bytes;
    int a = 42;
    bytes.appendNumber(a, sizeof(int));
    fo.writeByte(bytes);
}

void test_read_interger() {
    zedis::File fi(open("content.txt", O_RDONLY));
    zedis::Bytes bytes;
    auto rv = fi.readByte(bytes, sizeof(int));
    auto b = bytes.getNumber<int>(sizeof(int));
    std::cout << "rv = " << rv << ", "
              << "b = " << b << "\n";
}

int main() {
    test_write_string();
    test_read_string();

    test_write_interger();
    test_read_interger();

    return 0;
}