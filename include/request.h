#pragma once

#include "common.h"

enum class CmdRes {
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2,
};

static bool cmd_is(const std::string &word, const char *cmd) {
    return 0 == strcasecmp(word.c_str(), cmd);
}

std::map<std::string, std::string> g_map;

CmdRes
do_get(const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen) {
#ifdef DEBUG
    std::cout << "get命令"
              << "\n";
#endif

    if (!g_map.count(cmd[1])) return CmdRes::RES_NX;
    std::string &val = g_map[cmd[1]];
    assert(val.size() <= k_max_msg);
    memcpy(res, val.data(), val.size());
    *reslen = (uint32_t)val.size();
    return CmdRes::RES_OK;
}
CmdRes
do_set(const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen) {
#ifdef DEBUG
    std::cout << "set命令"
              << "\n";
#endif
    (void)res;
    (void)reslen;
    g_map[cmd[1]] = cmd[2];
    return CmdRes::RES_OK;
}
CmdRes
do_del(const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen) {
#ifdef DEBUG
    std::cout << "del命令"
              << "\n";
#endif
    (void)res;
    (void)reslen;
    g_map.erase(cmd[1]);
    return CmdRes::RES_OK;
}

int32_t
parse_req(const uint8_t *data, size_t len, std::vector<std::string> &out) {
#ifdef DEBUG
    std::cout << block2string(data) << "\n";
#endif

    if (len < 4) return -1;
    uint32_t n = 0;
    memcpy(&n, &data[0], 4);
    if (n > k_max_msg) return -1;
    size_t pos = 4;
    while (n--) {
        if (pos + 4 > len) return -1;
        uint32_t sz = 0;
        memcpy(&sz, &data[pos], 4);
        if (pos + 4 + sz > len) return -1;
        out.push_back(std::string((char *)&data[pos + 4], sz));
        pos += 4 + sz;
    }
#ifdef DEBUG
    for (const auto sam : out) { std::cout << "命令 \"" << sam << "\"\n"; }
#endif

    if (pos != len) return -1;
    return 0;
}

int32_t do_request(
    const uint8_t *req,
    uint32_t reqlen,
    uint32_t *rescode,
    uint8_t *res,
    uint32_t *reslen) {
#ifdef DEBUG
    std::cout << "进入函数do_request\n";
#endif
#ifdef DEBUG
    std::cout << block2string(req) << "\n";
#endif
    std::vector<std::string> cmd;
    if (0 != parse_req(req, reqlen, cmd)) {
        msg("bad req");
        return -1;
    }
    if (cmd.size() == 2 && cmd_is(cmd[0], "get")) {
#ifdef DEBUG
        std::cout << "执行命令get\n";
#endif
        *rescode = static_cast<uint32_t>(do_get(cmd, res, reslen));
    } else if (cmd.size() == 3 && cmd_is(cmd[0], "set")) {
#ifdef DEBUG
        std::cout << "执行命令set\n";
#endif
        *rescode = static_cast<uint32_t>(do_set(cmd, res, reslen));
    } else if (cmd.size() == 2 && cmd_is(cmd[0], "del")) {
#ifdef DEBUG
        std::cout << "执行命令del\n";
#endif
        *rescode = static_cast<uint32_t>(do_del(cmd, res, reslen));
    } else {
#ifdef DEBUG
        std::cout << "无符合命令\n";
#endif
        *rescode = static_cast<uint32_t>(CmdRes::RES_ERR);
        const char *msg = "Unknown cmd";
        strcpy((char *)res, msg);
        *reslen = strlen(msg);
    }
#ifdef DEBUG
    std::cout << "rescode = " << *rescode << "\n";
    std::cout << "reslen = " << *reslen << "\n";
#endif
    return 0;
}
