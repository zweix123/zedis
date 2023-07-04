#pragma once

#include "common.h"
#include "bytes.h"
#include "hashtable.h"

#include <string>
#include <vector>
#include <map>
#include <tuple>

namespace zedis {

void out_nil(Bytes &out) {
    out.appendNumber(static_cast<uint8_t>(SerType::SER_NIL), 1);
}

void out_str(Bytes &out, const std::string &val) {
    out.appendNumber(static_cast<uint8_t>(SerType::SER_STR), 1);
    out.appendNumber<uint32_t>(val.size(), 4);
    out.appendString(val);
}

void out_int(Bytes &out, int64_t val) {
    out.appendNumber(static_cast<uint8_t>(SerType::SER_INT), 1);
    out.appendNumber<int64_t>(val, 8);
}

void out_err(Bytes &out, CmdRes code, const std::string &msg) {
    out.appendNumber(static_cast<uint8_t>(SerType::SER_ERR), 1);
    out.appendNumber(static_cast<uint32_t>(code), 4);
    out.appendNumber<uint32_t>(msg.size(), 4);
    out.appendString(msg);
}

void out_arr(Bytes &out, uint32_t n) {
    out.appendNumber(static_cast<uint8_t>(SerType::SER_ARR), 1);
    out.appendNumber<uint32_t>(n, 4);
}

namespace interpreter {

#define container_of(ptr, type, member)                    \
    ({                                                     \
        const typeof(((type *)0)->member) *__mptr = (ptr); \
        (type *)((char *)__mptr - offsetof(type, member)); \
    })

    struct Entry {
        HNode node;
        std::string key, val;
    };
    HMap m_map;
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
    bool del(std::string k) {
        Entry key;
        key.key = k;
        key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
        HNode *node = m_map.pop(&key.node, entry_eq);
        if (node) {
            delete container_of(node, Entry, node);
            return true;
        }
        return false;
    }
    void scan(Bytes &buf) {
        NodeScan node_scan = [](HNode *node, void *arg) {
            Bytes &buf = *(Bytes *)arg;
            std::string_view s = container_of(node, Entry, node)->key;
            auto t = std::string(s);
            out_str(buf, t);
        };
        m_map.scan(node_scan, &buf);
    }
}; // namespace interpreter

bool cmd_is(const std::string_view &word, const char *cmd) {
    return word == std::string_view(cmd);
}

void do_get(const std::vector<std::string_view> &cmd, Bytes &out) {
    auto res = interpreter::get(std::string(cmd[1]));
    if (res.has_value()) {
        auto &s = res.value();
        out.appendNumber(static_cast<uint8_t>(SerType::SER_STR), 1);
        out.appendNumber(s.size(), 4);
        out.appendString(s);
    } else {
        out.appendNumber(static_cast<uint8_t>(SerType::SER_NIL), 1);
    }
}
void do_set(const std::vector<std::string_view> &cmd, Bytes &out) {
    interpreter::set(std::string(cmd[1]), std::string(cmd[2]));
    out_nil(out);
}
void do_del(const std::vector<std::string_view> &cmd, Bytes &out) {
    out_int(out, interpreter::del(std::string(cmd[1])));
}

void do_keys(const std::vector<std::string_view> &cmd, Bytes &out) {
    out_arr(out, interpreter::m_map.size());
    interpreter::scan(out);
}

bool parse_req(Bytes &data, std::vector<std::string_view> &cmds) {
    if (data.read_end()) return false;
    auto cmd_num = data.getNumber<uint32_t>(4);
    while (cmd_num--) {
        auto cmd_len = data.getNumber<uint32_t>(4);
        cmds.emplace_back(data.getStringView(cmd_len));
    }
    return true;
}

void interpret(std::vector<std::string_view> &cmds, Bytes &out) {
    if (cmds.size() == 1 && cmd_is(cmds[0], "keys")) {
        do_keys(cmds, out);
    } else if (cmds.size() == 2 && cmd_is(cmds[0], "get")) {
        do_get(cmds, out);
    } else if (cmds.size() == 3 && cmd_is(cmds[0], "set")) {
        do_set(cmds, out);
    } else if (cmds.size() == 2 && cmd_is(cmds[0], "del")) {
        do_del(cmds, out);
    } else {
        out_err(out, CmdRes::RES_NX, "Unknown cmd");
    }
}

} // namespace zedis