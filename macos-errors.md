Run cmake --build build --config Release
  cmake --build build --config Release
  shell: /bin/bash -e {0}
[  4%] Building C object src/core/CMakeFiles/xxhash.dir/__/__/_deps/xxhash-src/xxhash.c.o
[  8%] Linking C static library libxxhash.a
[  8%] Built target xxhash
[ 12%] Building CXX object src/core/CMakeFiles/dupcleaner_core.dir/scanner.cpp.o
[ 16%] Building CXX object src/core/CMakeFiles/dupcleaner_core.dir/duplicate_finder.cpp.o
[ 20%] Building CXX object src/core/CMakeFiles/dupcleaner_core.dir/hasher.cpp.o
[ 24%] Linking CXX static library libdupcleaner_core.a
[ 24%] Built target dupcleaner_core
[ 24%] Built target CLI11
[ 28%] Building CXX object src/cli/CMakeFiles/dupcleaner_cli.dir/main.cpp.o
[ 32%] Linking CXX executable dupcleaner_cli
[ 32%] Built target dupcleaner_cli
[ 36%] Linking CXX static library ../../../lib/libgtest.a
[ 40%] Built target gtest
[ 44%] Linking CXX static library ../../../lib/libgtest_main.a
[ 48%] Built target gtest_main
[ 52%] Building CXX object tests/CMakeFiles/dupcleaner_tests.dir/test_placeholder.cpp.o
[ 56%] Building CXX object tests/CMakeFiles/dupcleaner_tests.dir/test_file_entry.cpp.o
[ 60%] Building CXX object tests/CMakeFiles/dupcleaner_tests.dir/test_scanner.cpp.o
[ 64%] Building CXX object tests/CMakeFiles/dupcleaner_tests.dir/test_duplicate_finder.cpp.o
[ 68%] Building CXX object tests/CMakeFiles/dupcleaner_tests.dir/test_hasher.cpp.o
[ 72%] Building CXX object tests/CMakeFiles/dupcleaner_tests.dir/test_cli_args.cpp.o
/Users/runner/work/DupCleaner/DupCleaner/tests/test_cli_args.cpp:2:10: fatal error: 'CLI/CLI.hpp' file not found
    2 | #include "CLI/CLI.hpp"
      |          ^~~~~~~~~~~~~
1 error generated.
make[2]: *** [tests/CMakeFiles/dupcleaner_tests.dir/test_cli_args.cpp.o] Error 1
make[1]: *** [tests/CMakeFiles/dupcleaner_tests.dir/all] Error 2
make: *** [all] Error 2
Error: Process completed with exit code 2.
