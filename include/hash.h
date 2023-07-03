#include "common.h"
#include <vector>
#include <functional>

namespace zedis {

struct HNode {
    std::shared_ptr<HNode> next;
    size_t hcode;
    HNode(size_t hcode_) : next{nullptr}, hcode{hcode_} {}
};

class HMap;

class HTab {
  private:
    std::vector<std::shared_ptr<HNode>> tab;
    size_t mask;
    size_t size;

    friend class HMap;

  public:
    HTab(size_t n) : tab{n}, mask{n}, size{0} {
        assert(n >= 0 && ((n - 1) & n) == 0);
    }
    void insert(std::weak_ptr<HNode> node) {
        auto pn = node.lock();
        assert(pn);
        size_t pos = pn->hcode & mask;
        auto next = tab[pos];
        pn->next = next;
        tab[pos] = pn;
        size++;
    }
    std::weak_ptr<HNode> lookup(
        std::weak_ptr<HNode> key,
        std::function<bool(std::weak_ptr<HNode>, std::weak_ptr<HNode>)> cmp) {
        if (tab.size() == 0) return std::weak_ptr<HNode>();

        auto pk = key.lock();
        size_t pos = pk->hcode & mask;
        std::weak_ptr<HNode> from = tab[pos];
        while (!from.expired()) {
            std::shared_ptr<HNode> sp_from = from.lock();
            if (cmp(from, key)) return from;
            from = sp_from->next;
        }
        return std::weak_ptr<HNode>();
    }

    std::shared_ptr<HNode> detach(std::shared_ptr<HNode> &from) {
        auto res = from;
        from = from->next;
        size--;
        return res;
    }
};

const size_t k_resizing_work = 128;
const size_t k_max_load_factor = 8;

class HMap {
  private:
    HTab ht1, ht2;
    size_t resizing_pos;

  public:
    HMap() : ht1{0}, ht2{0}, resizing_pos{0} {}
    std::weak_ptr<HNode> lookup(
        std::weak_ptr<HNode> key,
        std::function<bool(std::weak_ptr<HNode>, std::weak_ptr<HNode>)> cmp) {
        help_resizing();
        auto from = ht1.lookup(key, cmp);
        if (from.expired()) from = ht2.lookup(key, cmp);
        return from;
    }

    void help_resizing() {
        if (ht2.tab.size() == 0) return;
        size_t nwork = 0;
        while (nwork < k_resizing_work && ht2.size > 0) {
            std::weak_ptr<HNode> from = ht2.tab[resizing_pos];
            if (from.expired()) {
                resizing_pos++;
                continue;
            }
            auto sp_from = from.lock();
            ht1.insert(ht2.detach(sp_from));
        }
    }

    void insert(std::shared_ptr<HNode> node) {
        if (ht1.tab.size() == 0) ht1 = HTab{4};
        ht1.insert(node);
        if (ht2.tab.size() == 0) {
            size_t load_factor = ht1.size / (ht1.mask + 1);
            if (load_factor >= k_max_load_factor) { start_resizing(); }
        }
        help_resizing();
    }

    void start_resizing() {
        assert(ht2.tab.size() == 0);
        ht2 = std::move(ht1);
        ht1 = HTab{(ht2.mask + 1) << 1};
        resizing_pos = 0;
    }

    std::shared_ptr<HNode>
    pop(std::weak_ptr<HNode> key,
        std::function<bool(std::weak_ptr<HNode>, std::weak_ptr<HNode>)> cmp) {
        help_resizing();
        auto from = ht1.lookup(key, cmp);
        if (!from.expired()) {
            auto sp_from = from.lock();
            return ht1.detach(sp_from);
        }
        from = ht2.lookup(key, cmp);
        if (!from.expired()) {
            auto sp_from = from.lock();
            return ht2.detach(sp_from);
        }

        return std::shared_ptr<HNode>();
    }
};

uint64_t str_hash(const char *data, size_t len) {
    uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i < len; i++) { h = (h + data[i]) * 0x01000193; }
    return h;
}

struct Entry {
    HNode node;
    std::string key, value;
    Entry(std::string_view k)
        : key{k}, value{k}, node{str_hash(k.data(), k.size())} {}
};

} // namespace zedis