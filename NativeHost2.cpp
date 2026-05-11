#include <iostream>
#include <fstream>
#include <vector>
#include <stack>
#include <boost/dynamic_bitset.hpp>
#include <nlohmann/json.hpp>
#include <zlib.h>

using json = nlohmann::json;

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
    // Open input.bin
    std::ifstream in("input.bin", std::ios::binary);
    if (!in) {
        std::cerr << "Cannot open input.bin\n";
        return 1;
    }

    // Read length prefix
    uint32_t len = 0;
    in.read(reinterpret_cast<char*>(&len), 4);
    if (in.gcount() != 4) {
        std::cerr << "Failed to read length prefix\n";
        return 1;
    }

    // Read payload
    std::vector<char> buf(len);
    in.read(buf.data(), len);
    if (in.gcount() != len) {
        std::cerr << "Failed to read full payload\n";
        return 1;
    }
    std::string payload(buf.begin(), buf.end());

    // Parse JSON
    json parsed;
    try {
        parsed = json::parse(payload);
    } catch (const std::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << "\n";
        return 1;
    }

    // Process
    auto j_out = ProcessAudit(parsed.value("text", ""));
    std::string res = j_out.dump();
    uint32_t res_len = static_cast<uint32_t>(res.size());

    // Write output.bin
    std::ofstream out("output.bin", std::ios::binary);
    out.write(reinterpret_cast<const char*>(&res_len), 4);
    out.write(res.data(), res.size());

    std::cerr << "Wrote response of length " << res_len << " to output.bin\n";
    return 0;
}
