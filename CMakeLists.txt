cmake_minimum_required(VERSION 3.20)
project(hello LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
    mathlib
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../CPP-2
)

FetchContent_MakeAvailable(mathlib)

add_executable(iusearchbtw
    src/main.cpp)

target_link_libraries(iusearchbtw
    PRIVATE
        mathlib::mathlib
)
