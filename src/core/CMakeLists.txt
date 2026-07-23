include(FetchContent)

# Disable xxHash CLI tools and tests
set(XXHASH_BUILD_XXHSUM OFF CACHE BOOL "" FORCE)
set(XXHASH_BUILD_TESTS OFF CACHE BOOL "" FORCE)

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

# Placeholder empty library target
add_library(dupcleaner_core INTERFACE)
target_include_directories(dupcleaner_core INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(dupcleaner_core INTERFACE xxHash::xxhash)
