cmake_minimum_required(VERSION 3.16)

project(discord-screen-timer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)

add_executable(discord-screen-timer
    src/main.cpp
)

target_link_libraries(discord-screen-timer PRIVATE
    CURL::libcurl
)