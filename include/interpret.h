#pragma once

#include "common.h"
#include "hash.h"

#include <string>
#include <vector>
#include <map>
#include <tuple>

namespace zedis {

#define container_of(ptr, type, member)                    \
    ({                                                     \
        const typeof(((type *)0)->member) *__mptr = (ptr); \
        (type *)((char *)__mptr - offsetof(type, member)); \
    })

struct {
    HMap db;
} g_data;

std::map<std::string, std::string> g_map;

bool cmd_is(const std::string_view &word, const char *cmd) {
    return word == std::string_view(cmd);
}

std::tuple<CmdRes, std::string>
do_get(const std::vector<std::string_view> &cmd) {
#ifdef DEBUG
    std::cout << "do_get\n";
#endif
    Entry key{cmd[1]};
    auto cmp = [](std::weak_ptr<HNode> lhs, std::weak_ptr<HNode> rhs) {
        if (lhs.expired() || rhs.expired()) return false;
        auto plhs = lhs.lock();
        auto prhs = rhs.lock();

        auto le = container_of(plhs.get(), Entry, node);
        auto re = container_of(prhs.get(), Entry, node);
        return le->node.hcode == re->node.hcode && le->key == re->key;
    };
    std::shared_ptr<HNode> tmp{&key.node};
    auto node = g_data.db.lookup(std::weak_ptr<HNode>(tmp), cmp);

    if (node.expired()) return std::make_tuple(CmdRes::RES_NX, "");

    auto sp_node = node.lock();
    const std::string &val = container_of(sp_node.get(), Entry, node)->value;
    // if (!g_map.count(std::string(cmd[1])))
    //     return std::make_tuple(CmdRes::RES_NX, "");
    // std::string &val = g_map[std::string(cmd[1])];
    return std::make_tuple(CmdRes::RES_OK, val);
}
std::tuple<CmdRes, std::string>
do_set(const std::vector<std::string_view> &cmd) {
#ifdef DEBUG
    std::cout << "do_set\n";
#endif
    Entry key{cmd[1]};
    auto cmp = [](std::weak_ptr<HNode> lhs, std::weak_ptr<HNode> rhs) {
        if (lhs.expired() || rhs.expired()) return false;
        auto plhs = lhs.lock();
        auto prhs = rhs.lock();

        auto le = container_of(plhs.get(), Entry, node);
        auto re = container_of(prhs.get(), Entry, node);
        return le->node.hcode == re->node.hcode && le->key == re->key;
    };
    std::shared_ptr<HNode> tmp{&key.node};
    auto node = g_data.db.lookup(std::weak_ptr<HNode>(tmp), cmp);
    if (node.expired()) {
        std::shared_ptr<Entry> entry_ptr = std::make_shared<Entry>(key);
        entry_ptr->value = cmd[2];
        g_data.db.insert(std::shared_ptr<HNode>(&(entry_ptr->node)));
    }
    //
    // g_map[std::string(cmd[1])] = std::string(cmd[2]);
    return std::make_tuple(CmdRes::RES_OK, std::string());
}
std::tuple<CmdRes, std::string>
do_del(const std::vector<std::string_view> &cmd) {
#ifdef DEBUG
    std::cout << "do_del\n";
#endif
    Entry key{cmd[1]};
    auto cmp = [](std::weak_ptr<HNode> lhs, std::weak_ptr<HNode> rhs) {
        if (lhs.expired() || rhs.expired()) return false;
        auto plhs = lhs.lock();
        auto prhs = rhs.lock();

        auto le = container_of(plhs.get(), Entry, node);
        auto re = container_of(prhs.get(), Entry, node);
        return le->node.hcode == re->node.hcode && le->key == re->key;
    };
    std::shared_ptr<HNode> tmp{&key.node};
    auto node = g_data.db.lookup(std::weak_ptr<HNode>(tmp), cmp);
    if (!node.expired()) g_data.db.pop(std::weak_ptr<HNode>(tmp), cmp);
    // g_map.erase(std::string(cmd[1]));
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