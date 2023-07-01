#include "common.h"
#include "file.h"
#include <string>
#include <vector>

void test_write_string() {
    zedis::File fo(
        open("test_write_string.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644));
    zedis::Bytes bytes;
    bytes.appendString("hello world");
    fo.writeByte(bytes);
    std::cout << "test_write_string over\n";
}

void test_read_string() {
    zedis::File fi(open("test_write_string.txt", O_RDONLY));
    zedis::Bytes bytes;
    int n = 42;
    auto rv = fi.readByte(bytes, n);
    std::cout << "test_read_string(from test_write_string): 计划read " << n
              << ", 实际read " << rv << ", 内容为: " << bytes.getStringView(n)
              << "\n";
}

void test_write_interger() {
    zedis::File fo(
        open("test_write_interger", O_WRONLY | O_CREAT | O_TRUNC, 0644));
    zedis::Bytes bytes;
    int a = 42;
    bytes.appendNumber(a, sizeof(int));
    fo.writeByte(bytes);
    std::cout << "test_write_interger over\n";
}

void test_read_interger() {
    zedis::File fi(open("test_write_interger", O_RDONLY | O_CREAT));
    zedis::Bytes bytes;
    auto rv = fi.readByte(bytes, sizeof(int));
    auto b = bytes.getNumber<int>(sizeof(int));
    std::cout << "test_read_interger(from test_write_interger): rv = " << rv
              << ", "
              << "b = " << b << "\n";
}

void test_move() {
    zedis::File f1{open("test_move.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644)};
    zedis::Bytes bytes;
    bytes.appendString("hello world\n");
    f1.writeByte(bytes);
    // zedis::File f2 = f1;  // can't compile
    // zedis::File f2{f1};   // can't compile
    zedis::File f2{std::move(f1)};
    f2.writeByte(bytes);
    std::cout << "test_move over\n";
}

void test_pos() {
    std::cout << "test_pos: ";
    //
    zedis::File fo{open("test_pos.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644)};
    zedis::Bytes bytes;
    std::vector<std::string> cmds{"hello", " ", "world", "."};
    int all_len = 0;
    for (const auto &cmd : cmds) all_len += 4 + cmd.size();
    int32_t len = 4 + 4 + all_len;
    bytes.appendNumber(len, 4);
    bytes.appendNumber(cmds.size(), 4);
    for (const auto &cmd : cmds) {
        bytes.appendNumber(cmd.size(), 4);
        bytes.appendString(cmd);
    }
    fo.writeByte(bytes);
    //
    zedis::File fi(open("test_pos.txt", O_RDONLY));
    bytes.reset();
    fi.readByte(bytes, 4);
    fi.readByte(bytes, 4);
    all_len = bytes.getNumber<int32_t>(4);
    auto cmd_num = bytes.getNumber<int32_t>(4);
    while (cmd_num--) {
        fi.readByte(bytes, 4);
        auto cmd_len = bytes.getNumber<int32_t>(4);
        fi.readByte(bytes, cmd_len);
        auto cmd = bytes.getStringView(cmd_len);
        std::cout << cmd;
    }
    std::cout << "\n";
}

int main() {
    test_write_string();
    test_read_string();

    test_write_interger();
    test_read_interger();

    test_move();
    test_pos();

    return 0;
}