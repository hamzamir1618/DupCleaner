cmake_minimum_required(VERSION 3.10)
project(dupcleaner VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DUPCLEANER_BUILD_TESTS "Build tests" ON)
option(DUPCLEANER_BUILD_GUI "Build GUI frontend" ON)
option(DUPCLEANER_BUILD_FUZZERS "Build libFuzzer targets (requires Clang)" OFF)

if(DUPCLEANER_BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(src/core)

if(DUPCLEANER_BUILD_GUI)
    add_subdirectory(src/gui)
endif()

add_subdirectory(src/cli)

if(DUPCLEANER_BUILD_TESTS)
    add_subdirectory(tests)
endif()

add_subdirectory(tools)
