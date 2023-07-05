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
    // void init(AVLNode *node) {
    //     node->depth = 1;
    //     node->cnt = 1;
    //     node->left = node->right = node->parent = nullptr;
    // }
    uint32_t get_depth(AVLNode *node) {
        return node ? node->depth : 0;
    }
    uint32_t get_cnt(AVLNode *node) {
        return node ? node->cnt : 0;
    }
    // 为什么上面那些明显是Get的方法也要用函数? 因为可以自动处理node为空
    // 是的, 面向对象的话会导致要求实例必须有效, 但是在树的场景下很容易无效

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

} // namespace avl

} // namespace zedis
