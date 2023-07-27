#pragma once

#include <queue>
#include <mutex>        // unique_lock
#include <shared_mutex> // shared_lock

namespace zedis {

template<typename T>
class blocking_queue : protected std::queue<T> {
  public:
    // shared_mutex接口两种lock实现读写锁
    using wlock = std::unique_lock<std::shared_mutex>;
    using rlock = std::shared_lock<std::shared_mutex>;

  public:
    blocking_queue() = default;
    ~blocking_queue() { clear(); }
    // 拷贝和移动的构造函数和赋值运算符用不到
    blocking_queue(const blocking_queue &) = delete;
    blocking_queue(blocking_queue &&) = delete;
    blocking_queue &operator=(const blocking_queue &) = delete;
    blocking_queue &operator=(blocking_queue &&) = delete;

    bool empty() const {
        rlock lock(mtx_);
        return std::queue<T>::empty();
    }

    size_t size() const {
        rlock lock(mtx_);
        return std::queue<T>::size();
    }

  public:
    void clear() {
        wlock lock(mtx_);
        while (!std::queue<T>::empty()) { std::queue<T>::pop(); }
    }

    void push(const T &obj) {
        wlock lock(mtx_);
        std::queue<T>::push(obj);
    }

    template<typename... Args>
    void emplace(Args &&...args) {
        wlock lock(mtx_);
        std::queue<T>::emplace(std::forward<Args>(args)...); // 完美转发
    }

    bool pop(T &holder) {
        wlock lock(mtx_);
        if (std::queue<T>::empty()) {
            return false;
        } else {
            holder = std::move(std::queue<T>::front());
            std::queue<T>::pop();
            return true;
        }
    }

  private:
    mutable std::shared_mutex mtx_;
};

}; // namespace zedis