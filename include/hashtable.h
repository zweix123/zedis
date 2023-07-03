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

using Cmp = std::function<bool(HNode *, HNode *)>;

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
};

HTab make_htab(size_t n) {
    assert(n > 0 && ((n - 1) & n) == 0);
    return HTab{new HNode *[n], n, 0};
}

// Progressive Resizing

const size_t k_resizing_work = 128;
const size_t k_max_load_factor = 8;

struct HMap {
    HTab ht1{}, ht2{};
    size_t resizing_pos = 0;
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
};

#define container_of(ptr, type, member)                    \
    ({                                                     \
        const typeof(((type *)0)->member) *__mptr = (ptr); \
        (type *)((char *)__mptr - offsetof(type, member)); \
    })

struct Entry {
    HNode node;
    std::string key, val;
};

uint64_t str_hash(const uint8_t *data, size_t len) {
    uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i < len; i++) { h = (h + data[i]) * 0x01000193; }
    return h;
}

Cmp entry_eq = [](HNode *lhs, HNode *rhs) {
    Entry *le = container_of(lhs, Entry, node);
    Entry *re = container_of(rhs, Entry, node);
    return lhs->hcode == rhs->hcode && le->key == re->key;
};

class Map {
  private:
    HMap m_map;

  public:
    std::optional<std::string> get(std::string k) {
        Entry key;
        key.key = k;
        key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
        HNode *node = m_map.lookup(&key.node, entry_eq);
        if (!node) return std::optional<std::string>();

        const std::string &val = container_of(node, Entry, node)->val;
        return val;
    }
    void set(std::string k, std::string v) {
        Entry key;
        key.key = k;
        key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
        HNode *node = m_map.lookup(&key.node, entry_eq);
        if (node) container_of(node, Entry, node)->val = v;
        else {
            Entry *ent = new Entry();
            ent->key.swap(key.key);
            ent->node.hcode = key.node.hcode;
            ent->val = v;
            m_map.insert(&ent->node);
        }
    }
    void del(std::string k) {
        Entry key;
        key.key = k;
        key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
        HNode *node = m_map.pop(&key.node, entry_eq);
        if (node) delete container_of(node, Entry, node);
    }
};

} // namespace zedis