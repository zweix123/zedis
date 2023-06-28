#include "common.h"

int main() {
    std::vector<std::string> cmd{"ls"};
    uint32_t len = 4;
    for (const std::string &s : cmd) { len += 4 + s.size(); }
    if (len > k_max_msg) { return -1; }

    char wbuf[4 + k_max_msg];
    memcpy(&wbuf[0], &len, 4);
    uint32_t n = cmd.size();
    memcpy(&wbuf[4], &n, 4);
    size_t cur = 8;
    for (const std::string &s : cmd) {
        uint32_t p = (uint32_t)s.size();
        memcpy(&wbuf[cur], &p, 4);
        memcpy(&wbuf[cur + 4], s.data(), s.size());
        cur += 4 + s.size();
    }
    std::cout << block2string(wbuf) << "\n";
}