#pragma once

#include "common.h"
#include "avl.h"
#include "hashtable.h"

#include <optional>

namespace zedis {

struct ZNode {
    avl::AVLNode avl_node;
    HNode h_node;
    std::string name;
    double score;
    ZNode() = delete;
    ZNode(const std::string &name_, double score_)
        : avl_node{}, h_node{string_hash(name_)}, name{name_}, score{score_} {}
    friend std::ostream &operator<<(std::ostream &os, const ZNode &node) {
        return os << class2str("ZNode", "name", node.name, "score", node.score);
    }
};

bool zless(avl::AVLNode *lhs, double score, const std::string &name) {
    ZNode *zl = container_of(lhs, ZNode, avl_node);
    if (zl->score != score) { return zl->score < score; }
    return zl->name < name;
}

bool zless(avl::AVLNode *lhs, avl::AVLNode *rhs) {
    ZNode *zr = container_of(rhs, ZNode, avl_node);
    return zless(lhs, zr->score, zr->name);
}

struct HKey { // 辅助用, 因为想进入hash table必须有有一个包含HNode的结构体
    HNode node;
    std::string name;
    HKey() = delete;
    HKey(const std::string &name_) : node{string_hash(name_)}, name{name_} {}
};

Cmp hcmp = [](HNode *node, HNode *key) {
    if (node->hcode != key->hcode) { return false; }
    ZNode *znode = container_of(node, ZNode, h_node);
    HKey *hkey = container_of(key, HKey, node);
    return znode->name == hkey->name;
};

class ZNodeCollection {
  private:
    ZNode *data_;
    int64_t length_;

  public:
    class iterator {
      private:
        ZNode *ptr_;
        int64_t step_;

      public:
        iterator(ZNode *ptr, int64_t step) : ptr_(ptr), step_{step} {}
        iterator &operator++() {
            ptr_ =
                container_of(avl::offset(&ptr_->avl_node, +1), ZNode, avl_node);
            step_++;
            return *this;
        }
        bool operator!=(const iterator &other) const {
            if (!ptr_) return false;
            return step_ < other.step_;
        }
        ZNode &operator*() {
            assert(ptr_);
            return *ptr_;
        }
    };

    ZNodeCollection(ZNode *data, int64_t length)
        : data_{data}, length_{length} {}

    iterator begin() { return iterator(data_, 0); }
    iterator end() { return iterator(data_, length_); }
    iterator cbegin() const { return iterator(data_, 0); }
    iterator cend() const { return iterator(data_, length_); }

    int64_t size() const { return length_; }

    friend std::ostream &
    operator<<(std::ostream &os, const ZNodeCollection &znode_colloction) {
        return os << class2str(
                   "ZNodeCollection", "data_", znode_colloction.data_,
                   "length_", znode_colloction.length_);
    }
};

class ZSet {
  private:
    avl::AVLNode *avl_root{nullptr};
    HMap hmap{};

    void tree_add(ZNode *node) {
        if (!avl_root) {
            avl_root = &node->avl_node;
            return;
        }

        avl::AVLNode *cur = avl_root;
        while (true) {
            avl::AVLNode **from =
                zless(&node->avl_node, cur) ? &cur->left : &cur->right;
            if (!*from) {
                *from = &node->avl_node;
                node->avl_node.parent = cur;
                avl_root = avl::fix(&node->avl_node);
                break;
            }
            cur = *from;
        }
    }

    ZNode *lookup(const std::string &name) {
        if (!avl_root) return nullptr;
        HKey key{name};
        HNode *found = hmap.lookup(&key.node, hcmp);
        if (!found) return nullptr;
        return container_of(found, ZNode, h_node);
    }

  public:
    // ZSet() = default;
    ~ZSet() {
        std::function<void(avl::AVLNode *)> tree_dispose;
        tree_dispose = [&](avl::AVLNode *node) {
            if (!node) return;
            tree_dispose(node->left);
            tree_dispose(node->right);
            delete container_of(node, ZNode, avl_node);
        };
        tree_dispose(avl_root);
    }

    bool add(const std::string &name, double score) {
        ZNode *node = lookup(name);

        if (node) {
            update(node, score);
            return false;
        } else {
            node = new ZNode{name, score};

            hmap.insert(&node->h_node);
            tree_add(node);
            return true;
        }
    }

    void update(ZNode *node, double score) {
        if (node->score == score) { return; }
        avl_root = avl::del(&node->avl_node);
        node->score = score;
        node->avl_node = avl::AVLNode{};
        tree_add(node);
    }

    std::optional<double> find(const std::string &name) {
        if (!avl_root) return std::optional<double>();
        HKey key{name};
        HNode *found = hmap.lookup(&key.node, hcmp);
        if (!found) return std::optional<double>();
        return container_of(found, ZNode, h_node)->score;
    }

    // ZNode *pop(const std::string &name) {
    //     if (avl_root) return nullptr;
    //     HKey key{name};
    //     HNode *found = hmap.pop(&key.node, hcmp);
    //     if (!found) return nullptr;
    //     ZNode *node = container_of(found, ZNode, h_node);
    //     avl_root = avl::del(&node->avl_node);
    //     return node;
    // }
    bool pop(const std::string &name) {
        if (!avl_root) return false;
        HKey key{name};
        HNode *found = hmap.pop(&key.node, hcmp);
        if (!found) return false;
        ZNode *node = container_of(found, ZNode, h_node);
        avl_root = avl::del(&node->avl_node);
        delete node; //
        return true;
    }

    ZNode *query(double score, const std::string &name, int64_t offset) {
        avl::AVLNode *found = nullptr;
        avl::AVLNode *cur = avl_root;
        while (cur) {
            if (zless(cur, score, name)) {
                cur = cur->right;
            } else {
                found = cur; // candidate
                cur = cur->left;
            }
        }

        if (found) { found = avl::offset(found, offset); }
        return found ? container_of(found, ZNode, avl_node) : nullptr;
    }
    ZNodeCollection query(
        double score, const std::string &name, int64_t offset, int64_t limit) {
        avl::AVLNode *found = nullptr;
        avl::AVLNode *cur = avl_root;
        while (cur) {
            if (zless(cur, score, name)) {
                cur = cur->right;
            } else {
                found = cur; // candidate
                cur = cur->left;
            }
        }

        if (found) { found = avl::offset(found, offset); }

        return found ? ZNodeCollection(
                   container_of(found, ZNode, avl_node), limit)
                     : ZNodeCollection(nullptr, 0);
    }
};

} // namespace zedis