include(FetchContent)

# Enable CTest testing
enable_testing()

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(dupcleaner_tests test_placeholder.cpp test_file_entry.cpp test_scanner.cpp test_duplicate_finder.cpp test_hasher.cpp test_cli_args.cpp test_deleter.cpp test_image_loader.cpp test_perceptual_hash.cpp test_interactive_review.cpp ${CMAKE_SOURCE_DIR}/src/cli/interactive_review.cpp test_gui_formatting.cpp ${CMAKE_SOURCE_DIR}/src/gui/formatting.cpp test_gui_thumbnail_logic.cpp ${CMAKE_SOURCE_DIR}/src/gui/thumbnail_logic.cpp test_gui_selection_logic.cpp ${CMAKE_SOURCE_DIR}/src/gui/selection_logic.cpp)
target_include_directories(dupcleaner_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/cli ${CMAKE_SOURCE_DIR}/src/core ${CMAKE_SOURCE_DIR}/src/gui ${CMAKE_SOURCE_DIR}/third_party)
target_link_libraries(dupcleaner_tests PRIVATE dupcleaner_core GTest::gtest_main CLI11::CLI11)

include(GoogleTest)
gtest_discover_tests(dupcleaner_tests)

# CLI Integration Tests
add_executable(dupcleaner_cli_integration test_cli_integration.cpp)
target_include_directories(dupcleaner_cli_integration PRIVATE ${CMAKE_SOURCE_DIR}/src/cli ${CMAKE_SOURCE_DIR}/third_party)
target_link_libraries(dupcleaner_cli_integration PRIVATE GTest::gtest_main nlohmann_json::nlohmann_json)

add_dependencies(dupcleaner_cli_integration dupcleaner_cli)

gtest_discover_tests(dupcleaner_cli_integration
    PROPERTIES ENVIRONMENT "DUPCLEANER_CLI_EXE=$<TARGET_FILE:dupcleaner_cli>"
)
