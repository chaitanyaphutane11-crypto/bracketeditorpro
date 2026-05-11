cmake_minimum_required(VERSION 3.20)
project(NativeHost LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")

add_executable(NativeHost NativeHost.cpp)

find_package(Boost REQUIRED COMPONENTS asio dynamic_bitset)
find_package(nlohmann_json REQUIRED)
find_package(ZLIB REQUIRED)

target_link_libraries(NativeHost
    PRIVATE
        Boost::asio
        Boost::dynamic_bitset
        Boost::date_time
        Boost::container
        Boost::context
        nlohmann_json::nlohmann_json
        ZLIB::ZLIB
        ws2_32   
)
