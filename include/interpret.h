#pragma once

#include "common.h"
#include "bytes.h"
#include "hashtable.h"

#include <string>
#include <vector>
#include <map>
#include <tuple>

namespace zedis {

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
    void del(std::string k) {
        Entry key;
        key.key = k;
        key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
        HNode *node = m_map.pop(&key.node, entry_eq);
        if (node) delete container_of(node, Entry, node);
    }
    void scan(Bytes &buf) {
        NodeScan node_scan = [](HNode *node, void *arg) {
            Bytes &buf = *(Bytes *)arg;
            std::string_view s = container_of(node, Entry, node)->key;
            
            buf.appendNumber(s.size(), 4);
            buf.appendStringView(s);
        };
        m_map.scan(node_scan, &buf);
    }
}; // namespace interpreter

bool cmd_is(const std::string_view &word, const char *cmd) {
    return word == std::string_view(cmd);
}

std::tuple<CmdRes, std::string>
do_get(const std::vector<std::string_view> &cmd) {
    auto res = interpreter::get(std::string(cmd[1]));
    if (res.has_value()) return std::make_tuple(CmdRes::RES_OK, res.value());
    else
        return std::make_tuple(CmdRes::RES_NX, "");
}
std::tuple<CmdRes, std::string>
do_set(const std::vector<std::string_view> &cmd) {
    interpreter::set(std::string(cmd[1]), std::string(cmd[2]));
    return std::make_tuple(CmdRes::RES_OK, std::string());
}
std::tuple<CmdRes, std::string>
do_del(const std::vector<std::string_view> &cmd) {
    interpreter::del(std::string(cmd[1]));
    return std::make_tuple(CmdRes::RES_OK, std::string());
}

void parse_req(Bytes &data, std::vector<std::string_view> &cmds) {
    auto cmd_num = data.getNumber<uint32_t>(4);
    while (cmd_num--) {
        auto cmd_len = data.getNumber<uint32_t>(4);
        cmds.emplace_back(data.getStringView(cmd_len));
    }
}

std::tuple<CmdRes, std::string> interpret(std::vector<std::string_view> &cmds) {
    if (cmds.size() == 2 && cmd_is(cmds[0], "get")) {
        return do_get(cmds);
    } else if (cmds.size() == 3 && cmd_is(cmds[0], "set")) {
        return do_set(cmds);

    } else if (cmds.size() == 2 && cmd_is(cmds[0], "del")) {
        return do_del(cmds);
    } else {
        return std::make_tuple(CmdRes::RES_ERR, "Unknown Cmd");
    }
}

} // namespace zedis