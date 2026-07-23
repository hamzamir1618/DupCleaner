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

add_executable(dupcleaner_tests test_placeholder.cpp)
target_link_libraries(dupcleaner_tests PRIVATE dupcleaner_core GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(dupcleaner_tests)
