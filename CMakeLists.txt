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

# Packaging with CPack
set(CPACK_PACKAGE_NAME "DupCleaner")
set(CPACK_PACKAGE_VENDOR "DupCleaner Authors")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A fast, cross-platform duplicate file and image cleaner.")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "TGZ")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)
