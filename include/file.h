#pragma once

#include "common.h"
#include "bytes.h"
#include <unistd.h> // POSIX syscall
#include <fcntl.h>  // file control syscall

namespace zedis {

std::vector<std::byte> str2data(const std::string &&str) {
    const auto *data = reinterpret_cast<const std::byte *>(str.data());
    return std::vector<std::byte>(data, data + str.size());
}
std::string data2str(const std::vector<std::byte> &data) {
    const char *rawData = reinterpret_cast<const char *>(data.data());
    size_t dataSize = data.size() * sizeof(std::byte);
    return std::string(rawData, dataSize);
}

void fd_set_nb(int fd) {
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (errno) {
        err("fcntl error");
        return;
    }

    flags |= O_NONBLOCK;

    errno = 0;
    (void)fcntl(fd, F_SETFL, flags);
    if (errno) { err("fcntl error"); }
}

// 截断写 open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644)
// 追加写 open("output.txt", O_WRONLY | O_CREAT | O_APPEND, 0644)
// 读    open("input.txt", O_RDONLY)
class File {
  private:
    int m_fd;

  public:
    File() : m_fd(-1) {}
    explicit File(int fd) : m_fd(fd) {}
    ~File() {
        if (m_fd != -1) close(m_fd);
    }
    File(File &&other) : m_fd(other.m_fd) { other.m_fd = -1; }
    File(const File &) = delete;
    File &operator=(const File &) = delete;

    int data() const { return m_fd; }
    void set_nb() { fd_set_nb(m_fd); }

    bool check() const {
        errno = 0;
        int flags = fcntl(m_fd, F_GETFL, 0);
        if (errno) { err("file check fail"); }
        return errno != 0;
    }

    int32_t writeByte(const Bytes &bytes) {
        // all
        ssize_t count = bytes.data.size();
        ssize_t bytes_written = 0;
        while (bytes_written < count) {
            ssize_t rv = write(
                m_fd, bytes.data.data() + bytes_written, count - bytes_written);
            if (rv < 0) {
                err("!!!");
                return -1; // error
            }
            bytes_written += rv;
        }
        return (int)bytes_written;
    }

    int32_t readByte(Bytes &bytes, size_t count) {
        bytes.pos = 0;
        // all
        bytes.data.resize(count);
        ssize_t bytes_read = 0;
        while (bytes_read < count) {
            ssize_t rv =
                read(m_fd, bytes.data.data() + bytes_read, count - bytes_read);
            if (rv < 0) {
                err("read byte error");
                return -1;         // error
            }
            if (rv == 0) return 0; // EOF
            bytes_read += rv;
        }
        bytes.data.resize(bytes_read);
        return (int)bytes_read;
    }
};
} // namespace zedis