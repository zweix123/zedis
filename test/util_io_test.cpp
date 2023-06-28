#include "common.h"
#include <fstream>

int main() {
    std::string filename = "test.txt";
    std::string data = "Hello, world!";

    // Write data to file
    int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        std::cerr << "Error opening file: " << std::strerror(errno) << "\n";
        return -1;
    }
    const char *buf = data.c_str();
    size_t n = data.length();
    int32_t write_result = write_all(fd, buf, n);
    assert(write_result == 0);
    close(fd);

    // Read data from file
    fd = open(filename.c_str(), O_RDONLY);
    if (fd == -1) {
        std::cerr << "Error opening file: " << std::strerror(errno)
                  << std::endl;
        return -1;
    }
    char read_buf[1024];
    int32_t read_result = read_full(fd, read_buf, n);
    assert(read_result == 0);
    assert(std::strncmp(read_buf, buf, n) == 0);
    close(fd);

    std::cout << "Test passed." << std::endl;

    return 0;
}