#pragma once

#include "common.h"
#include "bytes.h"
#include "hashtable.h"
#include "zset.h"
#include "heap.h"
#include "c_thread_pool.h"

#include <string>
#include <vector>
#include <optional>
#include <exception>
#include <cmath>

static bool str2dbl(const std::string &s, double &out) {
    char *endp = NULL;
    out = strtod(s.c_str(), &endp);
    return endp == s.c_str() + s.size() && !std::isnan(out);
}

static bool str2int(const std::string &s, int64_t &out) {
    char *endp = NULL;
    out = strtoll(s.c_str(), &endp, 10);
    return endp == s.c_str() + s.size();
}

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

void out_err(Bytes &out, CmdErr code, const std::string &msg) {
    out.appendNumber(static_cast<uint8_t>(SerType::SER_ERR), 1);
    out.appendNumber(static_cast<uint32_t>(code), 4);
    out.appendNumber<uint32_t>(msg.size(), 4);
    out.appendString(msg);
}

static void out_dbl(Bytes &out, double val) {
    out.appendNumber(static_cast<uint8_t>(SerType::SER_DBL), 1);
    out.appendNumber(val, 8);
}

void out_arr(Bytes &out, uint32_t n) {
    out.appendNumber(static_cast<uint8_t>(SerType::SER_ARR), 1);
    out.appendNumber<uint32_t>(n, 4);
}

void out_update_arr(Bytes &out, uint32_t n) {
    auto type = static_cast<SerType>(out.getNumber<int>(1));
    assert(type == SerType::SER_ARR);
    out.insertNumber(n, 1, 4);
}

namespace core {

    class CoreException : public std::exception {
      public:
        CoreException(CmdErr code, const std::string &message)
            : m_code(code), m_message(message) {}

        virtual const char *what() const noexcept {
            return m_message.c_str(); // return the message as a C string
        }

        CmdErr getCode() const noexcept { return m_code; }

      private:
        CmdErr m_code;
        std::string m_message;
    };
    /* try {
     *     throw CoreException("An error occurred", CmdErr::...);
     * } catch (const CoreException &e) {
     *     std::string message = e.what();
     *     auto code = e.getCode();
     * }
     */

    HMap m_map{};
    Heap m_heap{};
    TheadPool tp;

    enum class EntryType {
        T_STR = 1,
        T_ZSET = 2,
    };

    struct Entry {
        HNode node;
        EntryType type;
        std::string key, val;
        std::shared_ptr<ZSet> zset;
        size_t heap_idx;
        Entry() = delete;
        Entry(const std::string &k)
            : type{EntryType::T_STR}
            , key{k}
            , val{}
            , node{string_hash(k)}
            , zset{nullptr}
            , heap_idx{0} {}
        Entry(std::string &&k, const std::string &v, uint64_t hcode)
            : type{EntryType::T_STR}
            , key(std::move(k))
            , val{v}
            , node{hcode}
            , zset{nullptr}
            , heap_idx{0} {}

        ~Entry() { set_ttl(-1); }

        void set_ttl(int64_t ttl_ms) {
            if (ttl_ms < 0 && heap_idx != 0) {
                // 如果设置的时间小于0就意味着这个结点超时了要去掉
                size_t pos = heap_idx;
                m_heap.del(pos);
                heap_idx = 0;
            } else if (ttl_ms >= 0) {
                size_t pos = heap_idx;

                uint64_t now_time =
                    get_monotonic_usec() + (uint64_t)ttl_ms * 1000;
                if (pos == 0) { // 这个entry可以之前不在堆中
                    m_heap.push(now_time, &heap_idx);
                } else {
                    m_heap.set(pos, now_time);
                }
            }
        }
    };

    Cmp entry_eq = [](HNode *lhs, HNode *rhs) {
        Entry *le = container_of(lhs, Entry, node);
        Entry *re = container_of(rhs, Entry, node);
        return lhs->hcode == rhs->hcode && le->key == re->key;
    };

    std::optional<std::string> get(const std::string &k) {
        Entry key{k};
        HNode *node = m_map.lookup(&key.node, entry_eq);
        if (!node) std::optional<std::string>();
        Entry *ent = container_of(node, Entry, node);
        if (ent->type != EntryType::T_STR) {
            throw CoreException(CmdErr::ERR_TYPE, "expect string type");
        }
        return std::make_optional<std::string>(ent->val);
    }
    void set(const std::string &k, const std::string &v) {
        Entry key{k};
        HNode *node = m_map.lookup(&key.node, entry_eq);
        if (node) {
            Entry *ent = container_of(node, Entry, node);
            if (ent->type != EntryType::T_STR) {
                throw CoreException(CmdErr::ERR_TYPE, "expect string type");
            }
            ent->val = v;
        } else {
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
    Entry *get_zset_entry(const std::string &k) {
        Entry key{k};
        HNode *hnode = m_map.lookup(&key.node, entry_eq);
        if (!hnode) return nullptr; // 没有

        Entry *ent = container_of(hnode, Entry, node);
        if (ent->type == EntryType::T_STR) return nullptr;
        return ent;
    }

}; // namespace core

bool cmd_is(const std::string_view word, const char *cmd) {
    return word == std::string_view(cmd);
}

void do_get(const std::vector<std::string> &cmd, Bytes &out) {
    try {
        auto res = core::get(cmd[1]);
        if (res.has_value()) {
            out_str(out, res.value());
        } else {
            out_nil(out);
        }
    } catch (const core::CoreException &e) {
        auto code = e.getCode();
        std::string message = e.what();
        out_err(out, code, message);
    }
}
void do_set(const std::vector<std::string> &cmd, Bytes &out) {
    try {
        core::set(cmd[1], cmd[2]);
        out_nil(out);
    } catch (const core::CoreException &e) {
        auto code = e.getCode();
        std::string message = e.what();
        out_err(out, code, message);
    }
}
void do_del(const std::vector<std::string> &cmd, Bytes &out) {
    out_int(out, core::del(cmd[1]));
}

void do_keys(const std::vector<std::string> &cmd, Bytes &out) {
    out_arr(out, core::m_map.size());
    core::scan(out);
}

// ==

void do_zadd(std::vector<std::string> &cmd, Bytes &out) {
    double score = 0;
    if (!str2dbl(cmd[2], score)) {
        return out_err(out, CmdErr::ERR_ARG, "expect fp number");
    }

    // look up or create the zset
    core::Entry key{cmd[1]};
    HNode *hnode = core::m_map.lookup(&key.node, core::entry_eq);
    core::Entry *ent = nullptr;
    if (!hnode) {
        ent = new core::Entry{std::move(key.key), "", key.node.hcode};

        ent->type = core::EntryType::T_ZSET;
        ent->zset = std::make_shared<ZSet>();
        core::m_map.insert(&ent->node);
    } else {
        ent = container_of(hnode, core::Entry, node);
        if (ent->type != core::EntryType::T_ZSET) {
            return out_err(out, CmdErr::ERR_TYPE, "expect zset");
        }
    }

    // add or update the tuple
    const std::string &name = cmd[3];
    auto ok = ent->zset->add(name, score);

    out_int(out, (int64_t)ok);
}

static void do_zrem(std::vector<std::string> &cmd, Bytes &out) {
    core::Entry *ent = core::get_zset_entry(cmd[1]);
    if (!ent) {
        out_nil(out);
        return;
    }

    const std::string &name = cmd[2];
    auto ok = ent->zset->pop(name);
    out_int(out, (int64_t)ok);
}

void do_zscore(std::vector<std::string> &cmd, Bytes &out) {
    core::Entry *ent = core::get_zset_entry(cmd[1]);
    if (!ent) {
        out_nil(out);
        return;
    }

    const std::string &name = cmd[2];
    auto res = ent->zset->find(name);
    if (res.has_value()) out_dbl(out, res.value());
    else
        out_nil(out);
}

void do_zquery(const std::vector<std::string> &cmd, Bytes &out) {
    // parse args
    double score = 0;
    if (!str2dbl(cmd[2], score)) {
        return out_err(out, CmdErr::ERR_ARG, "expect fp number");
    }
    const std::string &name = cmd[3];
    int64_t offset = 0;
    int64_t limit = 0;
    if (!str2int(cmd[4], offset)) {
        return out_err(out, CmdErr::ERR_ARG, "expect int");
    }
    if (!str2int(cmd[5], limit)) {
        return out_err(out, CmdErr::ERR_ARG, "expect int");
    }

    // get the zset
    core::Entry *ent = core::get_zset_entry(cmd[1]);
    if (!ent) {
        out_err(out, CmdErr::ERR_TYPE, "expect zset");
        return;
    }

    if (limit & 1) limit++;
    limit >>= 1;

    auto ite = ent->zset->query(score, name, offset, limit);

    uint32_t n = 0;
    Bytes buff;
    for (auto sam : ite) {
        out_str(buff, sam.name);
        out_dbl(buff, sam.score);
        n += 2;
    }

    out_arr(out, n);
    out.appendBytes_move(std::move(buff));
}

void do_expire(const std::vector<std::string> &cmd, Bytes &out) {
    int64_t ttl_ms = 0;
    if (!str2int(cmd[2], ttl_ms)) {
        out_err(out, CmdErr::ERR_ARG, "expect int64");
        return;
    }

    core::Entry key{cmd[1]};
    HNode *node = core::m_map.lookup(&key.node, core::entry_eq);

    if (node) {
        core::Entry *ent = container_of(node, core::Entry, node);
        ent->set_ttl(ttl_ms);
    }
    out_int(out, node ? 1 : 0);
    return;
}

void do_ttl(const std::vector<std::string> &cmd, Bytes &out) {
    core::Entry key{cmd[1]};
    HNode *node = core::m_map.lookup(&key.node, core::entry_eq);

    if (!node) {
        out_int(out, -2);
        return;
    }

    core::Entry *ent = container_of(node, core::Entry, node);
    if (ent->heap_idx == 0) { return out_int(out, -1); }

    uint64_t expire_at = core::m_heap.get(ent->heap_idx);
    uint64_t now_us = get_monotonic_usec();
    return out_int(out, expire_at > now_us ? (expire_at - now_us) / 1000 : 0);
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

void interpret(std::vector<std::string> &cmd, Bytes &out) {
    if (cmd.size() == 1 && cmd_is(cmd[0], "keys")) {
        do_keys(cmd, out);
    } else if (cmd.size() == 2 && cmd_is(cmd[0], "get")) {
        do_get(cmd, out);
    } else if (cmd.size() == 3 && cmd_is(cmd[0], "set")) {
        do_set(cmd, out);
    } else if (cmd.size() == 2 && cmd_is(cmd[0], "del")) {
        do_del(cmd, out);
    } else if (cmd.size() == 4 && cmd_is(cmd[0], "zadd")) {
        do_zadd(cmd, out);
    } else if (cmd.size() == 3 && cmd_is(cmd[0], "zrem")) {
        do_zrem(cmd, out);
    } else if (cmd.size() == 3 && cmd_is(cmd[0], "zscore")) {
        do_zscore(cmd, out);
    } else if (cmd.size() == 6 && cmd_is(cmd[0], "zquery")) {
        do_zquery(cmd, out);
    } else if (cmd.size() == 3 && cmd_is(cmd[0], "pexpire")) {
        do_expire(cmd, out);
    } else if (cmd.size() == 2 && cmd_is(cmd[0], "pttl")) {
        do_ttl(cmd, out);
    } else {
        // cmd is not recognized
        out_err(out, CmdErr::ERR_UNKNOWN, "Unknown cmd");
    }
}

} // namespace zedis