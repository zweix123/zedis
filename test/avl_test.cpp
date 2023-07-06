#include "common.h"
#include "avl.h"
#include <set>

using namespace zedis;

struct Data {
    avl::AVLNode node;
    uint32_t val;
    Data() : node{}, val{0} {}
};

struct Container {
    avl::AVLNode *root{nullptr};

    void add(uint32_t val) {
        Data *data = new Data{};
        // init(&data->node);
        data->val = val;

        if (!root) {
            root = &data->node;
            return;
        }

        avl::AVLNode *cur = root;
        while (true) {
            avl::AVLNode **from = (val < container_of(cur, Data, node)->val)
                                      ? &cur->left
                                      : &cur->right;
            if (!*from) {
                *from = &data->node;
                data->node.parent = cur;
                root = fix(&data->node);
                break;
            }
            cur = *from;
        }
    }
    bool del(uint32_t val) {
        avl::AVLNode *cur = root;
        while (cur) {
            uint32_t node_val = container_of(cur, Data, node)->val;
            if (val == node_val) { break; }
            cur = val < node_val ? cur->left : cur->right;
        }
        if (!cur) { return false; }

        root = avl::del(cur);
        delete container_of(cur, Data, node);
        return true;
    }
};

void verify(avl::AVLNode *parent, avl::AVLNode *node) {
    if (!node) { return; }

    assert(node->parent == parent);
    verify(node, node->left);
    verify(node, node->right);

    assert(
        node->cnt == 1 + avl::get_cnt(node->left) + avl::get_cnt(node->right));

    uint32_t l = avl::get_depth(node->left);
    uint32_t r = avl::get_depth(node->right);
    assert(l == r || l + 1 == r || l == r + 1);
    assert(node->depth == 1 + std::max(l, r));

    uint32_t val = container_of(node, Data, node)->val;
    if (node->left) {
        assert(node->left->parent == node);
        assert(container_of(node->left, Data, node)->val <= val);
    }
    if (node->right) {
        assert(node->right->parent == node);
        assert(container_of(node->right, Data, node)->val >= val);
    }
}

void extract(avl::AVLNode *node, std::multiset<uint32_t> &extracted) {
    if (!node) { return; }
    extract(node->left, extracted);
    extracted.insert(container_of(node, Data, node)->val);
    extract(node->right, extracted);
}

void container_verify(Container &c, const std::multiset<uint32_t> &ref) {
    verify(NULL, c.root);
    assert(avl::get_cnt(c.root) == ref.size());
    std::multiset<uint32_t> extracted;
    extract(c.root, extracted);
    assert(extracted == ref);
}

void dispose(Container &c) {
    while (c.root) {
        avl::AVLNode *node = c.root;
        c.root = avl::del(c.root);
        delete container_of(node, Data, node);
    }
}

//

static void test_insert(uint32_t sz) {
    for (uint32_t val = 0; val < sz; ++val) {
        Container c;
        std::multiset<uint32_t> ref;
        for (uint32_t i = 0; i < sz; ++i) {
            if (i == val) { continue; }
            c.add(i);
            ref.insert(i);
        }
        container_verify(c, ref);

        c.add(val);
        ref.insert(val);
        container_verify(c, ref);
        dispose(c);
    }
}

static void test_insert_dup(uint32_t sz) {
    for (uint32_t val = 0; val < sz; ++val) {
        Container c;
        std::multiset<uint32_t> ref;
        for (uint32_t i = 0; i < sz; ++i) {
            c.add(i);
            ref.insert(i);
        }
        container_verify(c, ref);

        c.add(val);
        ref.insert(val);
        container_verify(c, ref);
        dispose(c);
    }
}

static void test_remove(uint32_t sz) {
    for (uint32_t val = 0; val < sz; ++val) {
        Container c;
        std::multiset<uint32_t> ref;
        for (uint32_t i = 0; i < sz; ++i) {
            c.add(i);
            ref.insert(i);
        }
        container_verify(c, ref);

        assert(c.del(val));
        ref.erase(val);
        container_verify(c, ref);
        dispose(c);
    }
}

int main() {
    Container c;

    // some quick tests
    container_verify(c, {});
    c.add(123);
    container_verify(c, {123});
    assert(!c.del(124));
    assert(c.del(123));
    container_verify(c, {});
    std::cout << "base test pass\n";

    // sequential insertion
    std::multiset<uint32_t> ref;
    for (uint32_t i = 0; i < 1000; i += 3) {
        c.add(i);
        ref.insert(i);
        container_verify(c, ref);
    }
    std::cout << "sequential insertion pass\n";

    // random insertion
    for (uint32_t i = 0; i < 100; i++) {
        uint32_t val = (uint32_t)rand() % 1000;
        c.add(val);
        ref.insert(val);
        container_verify(c, ref);
    }
    std::cout << "random insertion pass\n";

    // random deletion
    for (uint32_t i = 0; i < 200; i++) {
        uint32_t val = (uint32_t)rand() % 1000;
        auto it = ref.find(val);
        if (it == ref.end()) {
            assert(!c.del(val));
        } else {
            assert(c.del(val));
            ref.erase(it);
        }
        container_verify(c, ref);
    }
    std::cout << "random deletion pass\n";

    // insertion/deletion at various positions
    for (uint32_t i = 0; i < 200; ++i) {
        std::cout << "The " << i
                  << " insertion/deletion at various positions\n";
        test_insert(i);
        test_insert_dup(i);
        test_remove(i);
    }
    dispose(c);

    return 0;
}