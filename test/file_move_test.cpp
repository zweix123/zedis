#include "common.h"
#include "file.h"

int main() {
    zedis::File f1{open("content.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644)};
    zedis::Bytes bytes;
    bytes.appendString("hello world");
    f1.writeByte(bytes);
    // zedis::File f2 = f1;
    // zedis::File f2{f1};
    zedis::File f2{std::move(f1)};
    f2.writeByte(bytes);

    return 0;
}