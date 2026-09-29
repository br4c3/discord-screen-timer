cmake_minimum_required(VERSION 3.16)

project(discord-screen-timer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CURL_ROOT "/usr/local")
set(CURL_NO_CURL_CMAKE ON)

find_package(CURL REQUIRED)

add_executable(discord-screen-timer
    src/main.cpp
)

target_link_libraries(discord-screen-timer PRIVATE
    CURL::libcurl
)

set_target_properties(discord-screen-timer PROPERTIES
    BUILD_RPATH "/usr/local/lib"
)