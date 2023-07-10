#pragma once

#include "common.h"
#include "ztream.h"
#include <iostream>
#include <vector>

namespace zedis {

struct HeapItem {
    uint64_t val{0};
    size_t *ref{nullptr};
    HeapItem() = default;
    HeapItem(uint64_t val_, size_t *ref_) : val{val_}, ref{ref_} {}

    bool operator<(const HeapItem &other) const { return val < other.val; }
    bool operator>(const HeapItem &other) const { return val > other.val; }
    bool operator<=(const HeapItem &other) const { return val <= other.val; }
    bool operator>=(const HeapItem &other) const { return val >= other.val; }
    void change_ref(size_t pos) { *ref = pos; }

    friend std::ostream &operator<<(std::ostream &os, const HeapItem &hi) {
        return os << class2str("HeapItem", "val", hi.val, "ref", hi.ref);
    }
};
// 这个堆要即在server中有, 又要在entry中有, 且能相互转换, 这里是这样设计的
// 整个堆以数组的形式存在于server中, 然后entry有个属性就是对应堆结点的索引
// 然后这个堆结点数组中每个元素也有个属性是一个整数的指针, 就指向entry的那个属性

// idx start at 1 !!!
// parent: i >> 1
// left: i << 1
// right: i << 1 | 1

class Heap {
  private:
    std::vector<HeapItem> data;
    void up(size_t now) {
        assert(1 <= now && now < data.size());
        std::size_t next;

        while (now > 1) {
            next = now >> 1;

            if (data[next] <= data[now]) break;
            std::swap(data[next], data[now]);
            now = next;
        }
        data[now].change_ref(now);
    }

    void down(size_t now) {
        assert(1 <= now && now < data.size());
        std::size_t next;
        while ((now << 1) < data.size()) {
            next = now << 1;
            if (next < data.size() - 1 && data[next + 1] < data[next]) ++next;
            if (data[now] < data[next]) break;
            std::swap(data[now], data[next]);
            now = next;
        }
        data[now].change_ref(now);
    }

    void update(size_t pos) {
        if (pos > 1 && data[pos >> 1] > data[pos]) {
            up(pos);
        } else {
            down(pos);
        }
    }

  public:
    Heap() : data{1} {}

    friend std::ostream &operator<<(std::ostream &os, const Heap &heap) {
        os << class2str("Heap");
        os << "(\n";
        for (auto sam : heap.data) { os << "    " << sam << "\n"; }
        os << ")";
        return os;
    }

    bool empty() const { return data.size() == 1; }
    uint64_t get_min() const {
        assert(!empty());
        return data[1].val;
    }
    uint64_t get(size_t pos) const {
        assert(!empty());
        assert(1 <= pos && pos < data.size());
        return data[pos].val;
    }

    size_t *get_min_ref() const {
        assert(!empty());
        return data[1].ref;
    }

    void del(size_t pos) {
        data[pos] = data.back();

        data.pop_back();
        update(pos);
    }

    void push(uint64_t val, size_t *entry_heap_idx) {
        data.emplace_back(HeapItem{val, entry_heap_idx});
        update(data.size() - 1);
    }

    void set(size_t pos, uint64_t val) {
        data[pos].val = val;
        update(pos);
    }

    bool check(size_t pos = 1) {
        bool flag = true;
        if ((pos << 1) < data.size()) {
            assert(data[pos << 1] >= data[pos]);
            flag &= check(pos << 1);
        }
        if ((pos << 1 | 1) < data.size()) {
            assert(data[pos << 1 | 1] >= data[pos]);
            flag &= check(pos << 1 | 1);
        }
        return flag;
    }
};

} // namespace zedis