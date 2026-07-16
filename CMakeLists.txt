cmake_minimum_required(VERSION 3.10)
project(sysmon CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Curses REQUIRED)

add_executable(sysmon
    src/main.cpp
    src/cpu.cpp
    src/memory.cpp
    src/disk.cpp
    src/network.cpp
    src/process.cpp
    src/system.cpp
    src/utils.cpp
    src/graphs.cpp
    src/renderer.cpp
)

target_include_directories(sysmon PRIVATE
    include
    ${CURSES_INCLUDE_DIR}
)

target_link_libraries(sysmon PRIVATE ${CURSES_LIBRARIES})
