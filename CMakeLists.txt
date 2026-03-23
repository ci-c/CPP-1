cmake_minimum_required(VERSION 3.20)
project(hello LANGUAGES CXX)

add_executable(iusearchbtw
    src/main.cpp)

include(FetchContent)

FetchContent_Declare(
    mathlib
    GIT_REPOSITORY https://github.com/USERNAME/mathlib.git
    GIT_TAG master
)
