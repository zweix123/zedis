#include "common.h"
#include "zset.h"
#include <queue>

struct Node {
    std::string name;
    double score;
    bool operator<(const Node &node) const {
        if (score != node.score) { return score < node.score; }
        return name < node.name;
    }
};

void zset_verify(zedis::ZSet &zset, std::priority_queue<Node> &ref) {
}

int main() {
    bool ok;

    zedis::ZSet zset;
    ok = zset.add("123", 123);
    assert(ok);
    auto score = zset.find("123");
    assert(score == 123);
    ok = zset.pop("321");
    assert(!ok);
    ok = zset.pop("123");
    assert(ok);

    zset.add("1", 1);
    zset.add("2", 2);
    zset.add("3", 3);
    for (const auto &node : zset.query(1, "", 0, 3)) {
        std::cout << node << '\n';
    }

    return 0;
}