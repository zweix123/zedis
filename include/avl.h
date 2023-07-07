#pragma once

#include "common.h"

namespace zedis {

namespace avl {
    struct AVLNode {
        uint32_t depth{0};
        uint32_t cnt{0};
        AVLNode *left{nullptr};
        AVLNode *right{nullptr};
        AVLNode *parent{nullptr};
        AVLNode()
            : depth{1}
            , cnt{1}
            , left{nullptr}
            , right{nullptr}
            , parent{nullptr} {}
    };

    uint32_t get_depth(AVLNode *node) {
        return node ? node->depth : 0;
    }
    uint32_t get_cnt(AVLNode *node) {
        return node ? node->cnt : 0;
    }
    // 为什么上面这些很容易放在类中作为Get方法也要写成函数?
    // 因为这样可以自动处理node为nullptr, 是的, 在面向对象的思想中有一个前提,
    // 就是对象默认有效, 但是在树的场景下很容易遇到空节点

    void update(AVLNode *node) {
        node->depth =
            1 + std::max(get_depth(node->left), get_depth(node->right));
        node->cnt = 1 + get_cnt(node->left) + get_cnt(node->right);
    }

    AVLNode *rot_left(AVLNode *node) {
        AVLNode *new_node = node->right;
        if (new_node->left) { new_node->left->parent = node; }
        node->right = new_node->left;
        new_node->left = node;
        new_node->parent = node->parent;
        node->parent = new_node;
        update(node);
        update(new_node);
        return new_node;
    }

    AVLNode *rot_right(AVLNode *node) {
        AVLNode *new_node = node->left;
        if (new_node->right) { new_node->right->parent = node; }
        node->left = new_node->right;
        new_node->right = node;
        new_node->parent = node->parent;
        node->parent = new_node;
        update(node);
        update(new_node);
        return new_node;
    }

    AVLNode *fix_left(AVLNode *root) {
        if (get_depth(root->left->left) < get_depth(root->left->right)) {
            root->left = rot_left(root->left);
        }
        return rot_right(root);
    }

    AVLNode *fix_right(AVLNode *root) {
        if (get_depth(root->right->right) < get_depth(root->right->left)) {
            root->right = rot_right(root->right);
        }
        return rot_left(root);
    }

    AVLNode *fix(AVLNode *node) {
        while (true) {
            update(node);
            uint32_t l = get_depth(node->left);
            uint32_t r = get_depth(node->right);
            AVLNode **from = NULL;
            if (node->parent) {
                from = (node->parent->left == node) ? &node->parent->left
                                                    : &node->parent->right;
            }
            if (l == r + 2) {
                node = fix_left(node);
            } else if (l + 2 == r) {
                node = fix_right(node);
            }
            if (!from) { return node; }
            *from = node;
            node = node->parent;
        }
    }

    AVLNode *del(AVLNode *node) {
        if (node->right == NULL) {
            // no right subtree, replace the node with the left subtree
            // link the left subtree to the parent
            AVLNode *parent = node->parent;
            if (node->left) { node->left->parent = parent; }
            if (parent) {
                // attach the left subtree to the parent
                (parent->left == node ? parent->left : parent->right) =
                    node->left;
                return fix(parent);
            } else {
                // removing root?
                return node->left;
            }
        } else {
            // swap the node with its next sibling
            AVLNode *victim = node->right;
            while (victim->left) { victim = victim->left; }
            AVLNode *root = del(victim);

            *victim = *node;
            if (victim->left) { victim->left->parent = victim; }
            if (victim->right) { victim->right->parent = victim; }
            AVLNode *parent = node->parent;
            if (parent) {
                (parent->left == node ? parent->left : parent->right) = victim;
                return root;
            } else {
                // removing root?
                return victim;
            }
        }
    }

    AVLNode *offset(AVLNode *node, int64_t offset) {
        int64_t pos = 0; // relative to the starting node
        while (offset != pos) {
            if (pos < offset && pos + get_cnt(node->right) >= offset) {
                // the target is inside the right subtree
                node = node->right;
                pos += get_cnt(node->left) + 1;
            } else if (pos > offset && pos - get_cnt(node->left) <= offset) {
                // the target is inside the left subtree
                node = node->left;
                pos -= get_cnt(node->right) + 1;
            } else {
                // go to the parent
                AVLNode *parent = node->parent;
                if (!parent) { return NULL; }
                if (parent->right == node) {
                    pos -= get_cnt(node->left) + 1;
                } else {
                    pos += get_cnt(node->right) + 1;
                }
                node = parent;
            }
        }
        return node;
    }

} // namespace avl

} // namespace zedis
