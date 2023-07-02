#pragma once

#include "common.h"
#include <string>
#include <vector>
#include <map>
#include <tuple>

namespace zedis {

bool cmd_is(const std::string_view &word, const char *cmd) {
    return word == std::string_view(cmd);
}

std::map<std::string, std::string> g_map;

std::tuple<CmdRes, std::string>
do_get(const std::vector<std::string_view> &cmd) {
    if (!g_map.count(std::string(cmd[1])))
        return std::make_tuple(CmdRes::RES_NX, "");
    std::string &val = g_map[std::string(cmd[1])];
    return std::make_tuple(CmdRes::RES_OK, val);
}
std::tuple<CmdRes, std::string>
do_set(const std::vector<std::string_view> &cmd) {
    g_map[std::string(cmd[1])] = std::string(cmd[2]);
    return std::make_tuple(CmdRes::RES_OK, std::string());
}
std::tuple<CmdRes, std::string>
do_del(const std::vector<std::string_view> &cmd) {
    g_map.erase(std::string(cmd[1]));
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