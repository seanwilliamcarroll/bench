cmake_minimum_required(VERSION 3.20)
project(bench CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(bench
    src/main.cpp
    src/record.cpp
    src/utils.cpp
    src/symbols.cpp
)
