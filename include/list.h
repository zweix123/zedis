#pragma once

#include "common.h"

namespace zedis {
struct DList {
    /*
     * 其实大部分代码都是常规的链表,
     * 但是初始化其实有点说法, 其自封为环
     * 这就导致在insert(无论前后)还是delete
     * 之后这个链表仍然是一个环
     * // 所以在用的时候每次往头结点的前插, 用的时候用头结点的next, 就自然保序了
     */
    DList *prev{nullptr};
    DList *next{nullptr};
    DList() : prev{this}, next{this} {}
    [[nodiscard]] bool empty() const { return next == this; }
    void detach() const {
        // 对结点本身使用，用来将自己从所在的链表中剔除
        auto *p = prev;
        auto *n = next;
        p->next = n;
        n->prev = p;
    }
    void insert_before(DList *rookie) {
        DList *t = prev;
        t->next = rookie;
        rookie->prev = prev;
        rookie->next = this;
        this->prev = rookie;
    }
};

} // namespace zedis