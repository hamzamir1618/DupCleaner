set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

include(FetchContent)

# Fetch CLI11
FetchContent_Declare(
  cli11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
  GIT_TAG        v2.4.2
)
FetchContent_MakeAvailable(cli11)

# Fetch nlohmann_json
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

add_executable(dupcleaner_cli main.cpp)
target_include_directories(dupcleaner_cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dupcleaner_cli PRIVATE dupcleaner_core CLI11::CLI11 nlohmann_json::nlohmann_json)
