#include <iostream>
#include <vector>
#include <stack>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/dynamic_bitset.hpp>
#include <nlohmann/json.hpp>
#include <zlib.h>

#ifdef _WIN32
    #include <io.h>
    #include <fcntl.h>
    #include <boost/asio/windows/stream_handle.hpp>
#else
    #include <boost/asio/posix/stream_descriptor.hpp>
#endif

namespace asio = boost::asio;
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

asio::awaitable<void> run_host() {
    auto exec = co_await asio::this_coro::executor;

#ifdef _WIN32
    asio::windows::stream_handle in(exec, reinterpret_cast<HANDLE>(_get_osfhandle(0)));
    asio::windows::stream_handle out(exec, reinterpret_cast<HANDLE>(_get_osfhandle(1)));
#else
    asio::posix::stream_descriptor in(exec, ::dup(0)), out(exec, ::dup(1));
#endif

    for (;;) {
        uint32_t len = 0;
        co_await asio::async_read(in, asio::buffer(&len, 4), asio::use_awaitable);
        std::vector<char> buf(len);
        co_await asio::async_read(in, asio::buffer(buf), asio::use_awaitable);
        auto j_out = ProcessAudit(json::parse(buf.begin(), buf.end()).value("text", ""));
        std::string res = j_out.dump();
        uint32_t res_len = (uint32_t)res.length();
        co_await asio::async_write(out, asio::buffer(&res_len, 4), asio::use_awaitable);
        co_await asio::async_write(out, asio::buffer(res), asio::use_awaitable);
    }
}

int main() {
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    asio::io_context ctx;
    asio::co_spawn(ctx, run_host(), asio::detached);
    ctx.run();
    return 0;
}
