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

namespace core {

    struct Entry {
        HNode node;
        std::string key, val;
        Entry() = delete;
        Entry(const std::string &k) : key{k}, val{}, node{string_hash(k)} {}
        Entry(std::string &&k, const std::string &v, uint64_t hcode)
            : key(std::move(k)), val{v}, node{hcode} {}
    };
    HMap m_map{};

    Cmp entry_eq = [](HNode *lhs, HNode *rhs) {
        Entry *le = container_of(lhs, Entry, node);
        Entry *re = container_of(rhs, Entry, node);
        return lhs->hcode == rhs->hcode && le->key == re->key;
    };

    std::optional<std::string> get(const std::string &k) {
        Entry key{k};
        HNode *node = m_map.lookup(&key.node, entry_eq);
        if (!node) return std::optional<std::string>();
        return std::make_optional(container_of(node, Entry, node)->val);
    }
    void set(const std::string &k, const std::string &v) {
        Entry key{k};
        HNode *node = m_map.lookup(&key.node, entry_eq);
        if (node) container_of(node, Entry, node)->val = v;
        else {
            Entry *ent = new Entry{std::move(key.key), v, key.node.hcode};
            m_map.insert(&ent->node);
        }
    }
    bool del(const std::string &k) {
        Entry key{k};
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
            out_str(buf, container_of(node, Entry, node)->key);
        };
        m_map.scan(node_scan, &buf);
    }
    void dispose() {
        Nodedispose node_dispose = [](HNode *node) {
            delete container_of(node, Entry, node);
        };
        m_map.dispose(node_dispose);
    }
}; // namespace core

bool cmd_is(const std::string_view word, const char *cmd) {
    return word == std::string_view(cmd);
}

void do_get(const std::vector<std::string> &cmd, Bytes &out) {
    auto res = core::get(cmd[1]);
    if (res.has_value()) {
        out_str(out, res.value());
    } else {
        out_nil(out);
    }
}
void do_set(const std::vector<std::string> &cmd, Bytes &out) {
    core::set(cmd[1], cmd[2]);
    out_nil(out);
}
void do_del(const std::vector<std::string> &cmd, Bytes &out) {
    out_int(out, core::del(cmd[1]));
}

void do_keys(const std::vector<std::string> &cmd, Bytes &out) {
    out_arr(out, core::m_map.size());
    core::scan(out);
}

bool parse_req(Bytes &data, std::vector<std::string> &cmd) {
    if (data.is_read_end()) return false;
    auto cmd_num = data.getNumber<uint32_t>(4);
    while (cmd_num--) {
        auto cmd_len = data.getNumber<uint32_t>(4);
        cmd.emplace_back(data.getStringView(cmd_len));
    }
    return true;
}

void interpret(std::vector<std::string> &cmds, Bytes &out) {
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