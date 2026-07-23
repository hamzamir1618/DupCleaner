include(FetchContent)

FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG        v2.4.2
)
FetchContent_MakeAvailable(CLI11)

# CLI executable
add_executable(dupcleaner_cli main.cpp)
target_link_libraries(dupcleaner_cli PRIVATE dupcleaner_core CLI11::CLI11)
