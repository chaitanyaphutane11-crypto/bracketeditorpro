#include <iostream>
#include <vector>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

using json = nlohmann::json;

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

        // Echo back the same JSON
        std::string res = parsed.dump();
        uint32_t res_len = static_cast<uint32_t>(res.size());

        std::cout.write(reinterpret_cast<char*>(&res_len), 4);
        std::cout.write(res.data(), res.size());
        std::cout.flush();
    }
    return 0;
}
