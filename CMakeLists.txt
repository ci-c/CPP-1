cmake_minimum_required(VERSION 3.20)
project(calculator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)

include(FetchContent)

set(MATHLIB_GIT_REPOSITORY
    "https://github.com/ci-c/CPP-2.git"
    CACHE STRING "Git repository for the math library"
)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../CPP-2/CMakeLists.txt")
    set(
        FETCHCONTENT_SOURCE_DIR_MATHLIB
        "${CMAKE_CURRENT_SOURCE_DIR}/../CPP-2"
        CACHE PATH "Local override for mathlib"
        FORCE
    )
endif()

FetchContent_Declare(
    mathlib
    GIT_REPOSITORY ${MATHLIB_GIT_REPOSITORY}
    GIT_TAG master
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(mathlib)

find_program(CLANG_TIDY_EXE NAMES clang-tidy)
find_program(CLANG_FORMAT_EXE NAMES clang-format)

if (CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
else()
    message(WARNING "clang-tidy not found")
endif()

set(CALCULATOR_SOURCES
    src/main.cpp
)

add_executable(iusearchbtw
    ${CALCULATOR_SOURCES}
)

target_link_libraries(iusearchbtw
    PRIVATE
        mathlib::mathlib
)

if (CLANG_FORMAT_EXE)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i ${CALCULATOR_SOURCES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Formatting sources with clang-format"
    )
else()
    message(WARNING "clang-format not found")
endif()

if (CLANG_TIDY_EXE)
    add_custom_target(tidy
        COMMAND ${CLANG_TIDY_EXE}
                -p=${CMAKE_BINARY_DIR}
                ${CALCULATOR_SOURCES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running clang-tidy"
    )
endif()

install(TARGETS iusearchbtw
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
