include(FetchContent)

# Disable xxHash CLI tools and tests
set(XXHASH_BUILD_XXHSUM OFF CACHE BOOL "" FORCE)
set(XXHASH_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
    xxHash
    GIT_REPOSITORY https://github.com/Cyan4973/xxHash.git
    GIT_TAG        v0.8.2
    SOURCE_SUBDIR  ""
)
FetchContent_MakeAvailable(xxHash)

if(NOT TARGET xxhash)
    add_library(xxhash STATIC ${xxhash_SOURCE_DIR}/xxhash.c)
    target_include_directories(xxhash PUBLIC ${xxhash_SOURCE_DIR})
    add_library(xxHash::xxhash ALIAS xxhash)
endif()

# Core library target
add_library(dupcleaner_core STATIC scanner.cpp duplicate_finder.cpp hasher.cpp deleter.cpp)
target_include_directories(dupcleaner_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(dupcleaner_core PUBLIC xxHash::xxhash nlohmann_json::nlohmann_json)
