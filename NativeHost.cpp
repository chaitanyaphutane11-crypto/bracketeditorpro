#include <iostream>
#include <vector>
#include <stack>
#include <nlohmann/json.hpp>
#include <boost/dynamic_bitset.hpp>
#include <zlib.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

using json = nlohmann::json;

// --- ProcessAudit implementation ---
json ProcessAudit(const std::string& input) {
    int n = static_cast<int>(input.length());
    if (n == 0) return {{"matrix", {}}, {"checksum", 0}};

    std::vector<boost::dynamic_bitset<uint32_t>> tc(n, boost::dynamic_bitset<uint32_t>(n));
    std::stack<int> st;
    for (int i = 0; i < n; ++i) {
        tc[i].set(i);
        if (std::string("({[").find(input[i]) != std::string::npos) {
            if (!st.empty()) tc[st.top()].set(i);
            st.push(i);
        } else if (!st.empty() && std::string(")}]").find(input[i]) != std::string::npos) {
            st.pop();
        }
    }
    for (int k = 0; k < n; ++k)
        for (int i = 0; i < n; ++i)
            if (tc[i].test(k)) tc[i] |= tc[k];

    unsigned long crc = crc32(0L, Z_NULL, 0);
    std::vector<std::string> rows;
    for (int i = 0; i < n; ++i) {
        std::string s;
        boost::to_string(tc[i], s);
        std::reverse(s.begin(), s.end());
        rows.push_back(s);
        std::vector<uint32_t> blocks(tc[i].num_blocks());
        boost::to_block_range(tc[i], blocks.begin());
        crc = crc32(crc, (const unsigned char*)blocks.data(), (unsigned int)blocks.size() * 4);
    }
    return {{"matrix", rows}, {"checksum", crc}};
}

int main() {
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    while (true) {
        uint32_t len = 0;
        if (!std::cin.read(reinterpret_cast<char*>(&len), 4)) break;

        std::vector<char> buf(len);
        if (!std::cin.read(buf.data(), len)) break;

        std::string payload(buf.begin(), buf.end());
        json parsed;
        try {
            parsed = json::parse(payload);
        } catch (...) {
            continue; // skip bad input
        }

        // Run ProcessAudit on the "text" field
        auto j_out = ProcessAudit(parsed.value("text", ""));
        std::string res = j_out.dump();
        uint32_t res_len = static_cast<uint32_t>(res.size());

        std::cout.write(reinterpret_cast<char*>(&res_len), 4);
        std::cout.write(res.data(), res.size());
        std::cout.flush();
    }
    return 0;
}
