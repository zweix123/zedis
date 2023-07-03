#include "common.h"
#include <vector>
#include <string>
#include <functional>
#include <optional>

namespace zedis {

struct HNode {
    HNode *next{nullptr};
    uint64_t hcode{0};
};

const size_t k_resizing_work = 128;
const size_t k_max_load_factor = 8;

using Cmp = std::function<bool(HNode *, HNode *)>;
using NodeScan = std::function<void(HNode *, void *)>;

struct HTab {
    HNode **tab{nullptr};
    size_t mask{0}, size{0};

    void insert(HNode *node) {
        size_t pos = node->hcode & mask;
        node->next = tab[pos];
        tab[pos] = node;
        size++;
    }
    HNode **lookup(HNode *key, Cmp cmp) {
        if (!tab) return nullptr;
        size_t pos = key->hcode & mask;
        HNode **from = &tab[pos];
        while (*from) {
            if (cmp(*from, key)) return from;
            from = &((*from)->next);
        }
        return nullptr;
    }
    HNode *detach(HNode **from) {
        HNode *node = *from;
        *from = (*from)->next;
        size--;
        return node;
    }
    void scan(NodeScan node_scan, void *extra) {
        if (size == 0) return;
        for (size_t i = 0; i <= mask; ++i) {
            HNode *node = tab[i];
            while (node) {
                node_scan(node, extra);
                node = node->next;
            }
        }
    }
};

HTab make_htab(size_t n) {
    assert(n > 0 && ((n - 1) & n) == 0);
    return HTab{new HNode *[n], n, 0};
}

// Progressive Resizing

struct HMap {
    HTab ht1{}, ht2{};
    size_t resizing_pos = 0;
    size_t size() const { return ht1.size + ht2.size; }
    HNode *lookup(HNode *key, Cmp cmp) {
        help_resizing();
        HNode **from = ht1.lookup(key, cmp);
        if (!from) from = ht2.lookup(key, cmp);
        return from ? *from : nullptr;
    }
    void help_resizing() {
        if (!ht2.tab) return;
        size_t nwork = 0;
        while (nwork < k_resizing_work && ht2.size > 0) {
            HNode **from = &ht2.tab[resizing_pos];
            if (!*from) {
                resizing_pos++;
                continue;
            }
            ht1.insert(ht2.detach(from));
            nwork++;
        }
        if (ht2.size == 0) {
            delete[] ht2.tab;
            ht2 = HTab{};
        }
    }
    void insert(HNode *node) {
        if (!ht1.tab) ht1 = make_htab(4);
        ht1.insert(node);
        if (!ht2.tab) {
            size_t load_factor = ht1.size / (ht1.mask + 1);
            if (load_factor >= k_max_load_factor) { start_resizing(); }
        }
        help_resizing();
    }
    void start_resizing() {
        assert(ht2.tab == nullptr);

        ht2 = std::move(ht1);
        ht1 = make_htab((ht2.mask + 1) << 1);
        resizing_pos = 0;
    }
    HNode *pop(HNode *key, Cmp cmp) {
        help_resizing();
        HNode **from = ht1.lookup(key, cmp);
        if (from) return ht1.detach(from);
        from = ht2.lookup(key, cmp);
        if (from) return ht2.detach(from);

        return nullptr;
    }
    void scan(NodeScan node_scan, void *extra) {
        ht1.scan(node_scan, extra);
        ht2.scan(node_scan, extra);
    }
};

} // namespace zedis