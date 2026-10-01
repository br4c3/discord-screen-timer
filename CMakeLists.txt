cmake_minimum_required(VERSION 3.16)

project(discord-screen-timer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(SQLite3 REQUIRED)

add_executable(discord-screen-timer
    src/main.cpp
    src/gateway.cpp
    src/activity.cpp
    src/database.cpp
    src/command.cpp
    src/rest.cpp
)

target_include_directories(discord-screen-timer PRIVATE
    include
)

target_link_libraries(discord-screen-timer PRIVATE
    CURL::libcurl
    nlohmann_json::nlohmann_json
    SQLite::SQLite3
)