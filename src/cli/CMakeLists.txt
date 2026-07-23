set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

include(FetchContent)

# Fetch CLI11
FetchContent_Declare(
  cli11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
  GIT_TAG        v2.4.2
)
FetchContent_MakeAvailable(cli11)



add_executable(dupcleaner_cli main.cpp)
target_include_directories(dupcleaner_cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dupcleaner_cli PRIVATE dupcleaner_core CLI11::CLI11)
