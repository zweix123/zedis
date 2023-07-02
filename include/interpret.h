#pragma once

#include "common.h"
#include <string>
#include <vector>
#include <map>
#include <tuple>

namespace zedis {

enum class CmdRes {
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2,
};

bool cmd_is(const std::string &word, const char *cmd) {
    return 0 == strcasecmp(word.c_str(), cmd);
}

std::map<std::string, std::string> g_map;

std::tuple<CmdRes, std::string> do_get(const std::vector<std::string> &cmd) {
    if (!g_map.count(cmd[1])) return std::make_tuple(CmdRes::RES_NX, "");
    std::string &val = g_map[cmd[1]];
    return std::make_tuple(CmdRes::RES_OK, val);
}
std::tuple<CmdRes, std::string> do_set(const std::vector<std::string> &cmd) {
    g_map[cmd[1]] = cmd[2];
    return std::make_tuple(CmdRes::RES_OK, std::string());
}
std::tuple<CmdRes, std::string> do_del(const std::vector<std::string> &cmd) {
    g_map.erase(cmd[1]);
    return std::make_tuple(CmdRes::RES_OK, std::string());
}

int32_t parse_req(Bytes &data, std::vector<std::string> &cmds) {
    auto cmd_num = data.getNumber<uint32_t>(4);
    while (cmd_num--) {
        auto cmd_len = data.getNumber<uint32_t>(4);
        cmds.emplace_back(data.getStringView(cmd_len));
    }
    return 0;
}

std::tuple<CmdRes, std::string> interpret(std::vector<std::string> &cmds) {
    if (cmds.size() == 2 && cmd_is(cmds[0], "get")) {
        return do_get(cmds);
    } else if (cmds.size() == 3 && cmd_is(cmds[0], "set")) {
        return do_set(cmds);

    } else if (cmds.size() == 2 && cmd_is(cmds[0], "del")) {
        return do_del(cmds);
    } else {
        return std::make_tuple(CmdRes::RES_ERR, "Unknown cmds");
    }
}

} // namespace zedis