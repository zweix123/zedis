#pragma once

#include "bytes.h"
#include "common.h"
#include <fcntl.h>  // file control syscall
#include <unistd.h> // posIX syscall

namespace zedis {

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
        if (m_fd != -1) { close(m_fd); }
    }
    File(File &&other) : m_fd(other.m_fd) { other.m_fd = -1; }
    File(const File &) = delete;
    File &operator=(const File &) = delete;

    int data() const { return m_fd; }
    void set_nb() const { fd_set_nb(m_fd); }

    bool check() const {
        errno = 0;
        int flags = fcntl(m_fd, F_GETFL, 0);
        if (errno) { err("file check fail"); }
        return errno != 0;
    }

    bool writeByte_b(Bytes &bytes, size_t count = 0) const {
        // 阻塞模式写入fd, 复写至完成, 所以要么完全写入, 要么寄
        // 对Bytes相当于读
        if (count == 0) { count = bytes.data.size() - bytes.pos; }
        ssize_t bytes_written = 0;
        while (bytes_written < count) {
            ssize_t rv = write(
                m_fd, bytes.data.data() + bytes.pos + bytes_written,
                count - bytes_written);
            assert((size_t)rv <= count - bytes_written);

            if (rv <= 0) { return false; }
            // 那这里可不可能出现write(.., .., 0)嘛? 不会, 有while条件卡着
            bytes_written += (size_t)rv;
        }
        bytes.pos += static_cast<std::size_t>(bytes_written);
        return true;
    }

    bool readByte_b(Bytes &bytes, size_t count) const {
        // 阻塞模式读出fd, 复读至完成, 所以要么完全读出, 要么寄
        // 对Bytes相当于写
        std::size_t end = bytes.data.size();
        bytes.data.resize(end + count);
        // 注意这里已经对Bytes进行了resize(不然没空间装), 在err后我会还原size
        ssize_t bytes_read = 0;
        while (bytes_read < count) {
            ssize_t rv = read(
                m_fd, bytes.data.data() + end + bytes_read, count - bytes_read);
            if (rv <= 0) {
                bytes.data.resize(end + bytes_read);
                return false;
                // 用户怎么判断是不是EOF呢? 结合errno
            }
            assert((size_t)rv <= count - bytes_read);
            bytes_read += rv;
        }
        return true;
    }

    int writeByte_nb(Bytes &bytes) const {
        // 非阻塞模式写入fd, 情况要复杂些, 可能寄, 可能阻塞
        // 因为可能阻塞, 所有必须返回, 不能在函数内处理
        // -1表示寄, 0表示写完, 1表示阻塞
        // 对Bytes相当于读
        ssize_t rv = 0;
        do {
            ssize_t count = bytes.data.size() - bytes.pos;
            rv = write(m_fd, bytes.data.data() + bytes.pos, count);
        } while (rv < 0 && errno == EINTR);
        if (rv < 0 && errno == EAGAIN) {
            return 1; // got EAGAIN, stop.
        }
        if (rv < 0) { return -1; }
        bytes.pos += rv;
        assert(bytes.pos <= bytes.size());
        if (bytes.pos == bytes.size()) { return 0; }
        return -1;
    }

    int readByte_nb(Bytes &bytes, size_t count) const {
        // 非阻塞模式读出fd, 情况要复杂些, 可能寄, 可能阻塞, 可能读完
        // 因为可能阻塞, 所有必须返回, 不能在函数内处理
        // -1表示寄, 0表示读完, 1表示阻塞, 2表示读完
        // 对Bytes相当于写
        std::size_t end = bytes.data.size();
        bytes.data.resize(end + count);
        ssize_t rv = 0;
        do {
            rv = read(m_fd, bytes.data.data() + end, count);
        } while (rv < 0 && errno == EINTR);
        if (rv < 0 && errno == EAGAIN) {
            bytes.data.resize(end);
            return 1;
        }
        if (rv < 0) {
            return -1;
            bytes.data.resize(end);
        }
        if (rv == 0) {
            bytes.data.resize(end);
            return 2;
        }
        return 0;
    }
};
} // namespace zedis