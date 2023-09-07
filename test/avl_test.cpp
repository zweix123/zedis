#include "avl.h"
#include "common.h"
#include <set>

using namespace zedis;

struct Data {
    avl::AVLNode node{};
    uint32_t val{0};
    Data() = default;
};

struct Container {
    avl::AVLNode *root{nullptr};

    void add(uint32_t val) {
        Data *data = new Data{};

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

void tree_dispose(avl::AVLNode *node) {
    if (node) {
        tree_dispose(node->left);
        tree_dispose(node->right);
        delete container_of(node, Data, node);
    }
}

void test_case(uint32_t sz) {
    Container c;
    for (uint32_t i = 0; i < sz; ++i) { c.add(i); }

    avl::AVLNode *min = c.root;
    while (min->left) { min = min->left; }
    for (uint32_t i = 0; i < sz; ++i) {
        avl::AVLNode *node = avl::offset(min, (int64_t)i);
        assert(container_of(node, Data, node)->val == i);

        for (uint32_t j = 0; j < sz; ++j) {
            int64_t offset = (int64_t)j - (int64_t)i;
            avl::AVLNode *n2 = avl::offset(node, offset);
            assert(container_of(n2, Data, node)->val == j);
        }
        assert(!avl::offset(node, -(int64_t)i - 1));
        assert(!avl::offset(node, sz - i));
    }

    tree_dispose(c.root);
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
    std::cout << "base test \033[92mpass!\033[0m\n";

    // sequential insertion
    std::multiset<uint32_t> ref;
    for (uint32_t i = 0; i < 1000; i += 3) {
        c.add(i);
        ref.insert(i);
        container_verify(c, ref);
    }
    std::cout << "sequential insertion \033[92mpass!\033[0m\n";

    // random insertion
    for (uint32_t i = 0; i < 100; i++) {
        uint32_t val = (uint32_t)rand() % 1000;
        c.add(val);
        ref.insert(val);
        container_verify(c, ref);
    }
    std::cout << "random insertion \033[92mpass!\033[0m\n";

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
    std::cout << "random deletion \033[92mpass!\033[0m\n";

    // insertion/deletion at various positions
    for (uint32_t i = 0; i < 200; ++i) {
        // std::cout << "insertion/deletion at various positions with i = " <<
        // i;
        test_insert(i);
        test_insert_dup(i);
        test_remove(i);
        // std::cout << " \033[92mpass!\033[0m\n";
    }

    dispose(c);

    for (uint32_t i = 1; i < 500; ++i) {
        // std::cout << "offset with i = " << i;
        test_case(i);
        // std::cout << " \033[92mpass!\033[0m\n";
    }

    std::cout << "\033[94mtest all pass\033[0m\n";

    return 0;
}