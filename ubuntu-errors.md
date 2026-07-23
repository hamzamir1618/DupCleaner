Run cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DDUPCLEANER_BUILD_TESTS=ON -DDUPCLEANER_BUILD_GUI=OFF
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DDUPCLEANER_BUILD_TESTS=ON -DDUPCLEANER_BUILD_GUI=OFF
  shell: /usr/bin/bash -e {0}
-- The CXX compiler identification is GNU 13.3.0
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /usr/bin/c++ - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
CMake Deprecation Warning at build/_deps/cli11-src/CMakeLists.txt:1 (cmake_minimum_required):
  Compatibility with CMake < 3.10 will be removed from a future version of
  CMake.

  Update the VERSION argument <min> value.  Or, use the <min>...<max> syntax
  to tell CMake that the project requires at least <min> but has been updated
  to work with policies introduced by <max> or earlier.


-- The C compiler identification is GNU 13.3.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /usr/bin/cc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Found Python3: /usr/bin/python3.12 (found version "3.12.3") found components: Interpreter
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD - Success
-- Found Threads: TRUE
-- Configuring done (7.8s)
CMake Error: Error required internal CMake variable not set, cmake may not be built correctly.
Missing variable is:
CMAKE_C_COMPILE_OBJECT
CMake Error: Error required internal CMake variable not set, cmake may not be built correctly.
Missing variable is:
CMAKE_C_CREATE_STATIC_LIBRARY
CMake Generate step failed.  Build files cannot be regenerated correctly.
-- Generating done (0.0s)
Error: Process completed with exit code 1.
