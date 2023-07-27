#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <utility>

namespace zedis {
template<typename T>
class threadsafe_queue {
  public:
    using wlock = std::unique_lock<std::shared_mutex>;
    using rlock = std::shared_lock<std::shared_mutex>;

  private:
    struct node {
        T data;
        std::unique_ptr<node> next;
        node() = default;
        explicit node(T data_) : data(std::move(data_)) {}
    };

    mutable std::shared_mutex head_mutex_;
    std::unique_ptr<node> head_{nullptr};
    mutable std::shared_mutex tail_mutex_;
    node *tail_{nullptr};

  public:
    threadsafe_queue() : head_(std::make_unique<node>()), tail_(head_.get()) {}
    ~threadsafe_queue() { clear(); }
    threadsafe_queue(const threadsafe_queue &other) = delete;
    threadsafe_queue &operator=(const threadsafe_queue &other) = delete;

  private:
    node *tail() const {
        rlock lock(tail_mutex_);
        return tail_;
    }

  public:
    bool empty() const {
        rlock lock(head_mutex_);
        return tail() == head_.get();
    }

    size_t size() const {
        // 线性的size?
        rlock h_lock(head_mutex_);
        rlock t_lock(tail_mutex_);
        node *work = head_.get();
        size_t res = 0;
        while (work != tail_) {
            work = work->next.get();
            ++res;
        }
        return res;
    }

  public:
    void clear() {
        wlock h_lock(head_mutex_);
        wlock t_lock(tail_mutex_);
        std::make_unique<node>().swap(head_);
        tail_ = head_.get();
    }

    bool pop(T &holder) {
        wlock lock(head_mutex_);
        if (head_.get() == tail()) { return false; }
        holder = std::move(head_->data);
        const std::unique_ptr<node> old_head = std::move(head_);
        head_ = std::move(old_head->next);
        return true;
    }

    void push(T new_value) {
        std::unique_ptr<node> p = std::make_unique<node>();
        node *const new_tail = p.get();
        wlock lock(tail_mutex_);
        tail_->data = std::move(new_value);
        tail_->next = std::move(p);
        tail_ = new_tail;
    }

    template<typename... Args>
    void emplace(Args... args) {
        std::unique_ptr<node> p = std::make_unique<node>();
        node *const new_tail = p.get();
        wlock lock(tail_mutex_);
        tail_->data = T(std::forward<Args>(args)...);
        tail_->next = std::move(p);
        tail_ = new_tail;
    }
};
} // namespace zedis