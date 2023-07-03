#pragma once

#include "common.h"
#include "hashtable.h"

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
    Map db;
} g_data;

std::map<std::string, std::string> g_map;

bool cmd_is(const std::string_view &word, const char *cmd) {
    return word == std::string_view(cmd);
}

std::tuple<CmdRes, std::string>
do_get(const std::vector<std::string_view> &cmd) {
    auto res = g_data.db.get(std::string(cmd[1]));
    if (res.has_value()) return std::make_tuple(CmdRes::RES_OK, res.value());
    else
        return std::make_tuple(CmdRes::RES_NX, "");
}
std::tuple<CmdRes, std::string>
do_set(const std::vector<std::string_view> &cmd) {
    g_data.db.set(std::string(cmd[1]), std::string(cmd[2]));
    return std::make_tuple(CmdRes::RES_OK, std::string());
}
std::tuple<CmdRes, std::string>
do_del(const std::vector<std::string_view> &cmd) {
    g_data.db.del(std::string(cmd[1]));
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