Run cmake --build build --config Release
  cmake --build build --config Release
  shell: C:\Program Files\PowerShell\7\pwsh.EXE -command ". '{0}'"
MSBuild version 18.7.8+1ac568fee for .NET Framework

  1>Checking Build System
  Building Custom Rule D:/a/DupCleaner/DupCleaner/src/core/CMakeLists.txt
  xxhash.c
  xxhash.vcxproj -> D:\a\DupCleaner\DupCleaner\build\src\core\Release\xxhash.lib
  Building Custom Rule D:/a/DupCleaner/DupCleaner/src/core/CMakeLists.txt
  scanner.cpp
  duplicate_finder.cpp
  hasher.cpp
  Generating Code...
  dupcleaner_core.vcxproj -> D:\a\DupCleaner\DupCleaner\build\src\core\Release\dupcleaner_core.lib
  Building Custom Rule D:/a/DupCleaner/DupCleaner/src/cli/CMakeLists.txt
  main.cpp
  dupcleaner_cli.vcxproj -> D:\a\DupCleaner\DupCleaner\build\src\cli\Release\dupcleaner_cli.exe
  gtest.vcxproj -> D:\a\DupCleaner\DupCleaner\build\lib\Release\gtest.lib
  gtest_main.vcxproj -> D:\a\DupCleaner\DupCleaner\build\lib\Release\gtest_main.lib
  Building Custom Rule D:/a/DupCleaner/DupCleaner/tests/CMakeLists.txt
  test_cli_integration.cpp
D:\a\DupCleaner\DupCleaner\tests\test_cli_integration.cpp(67,48): warning C4267: 'argument': conversion from 'size_t' to 'int', possible loss of data [D:\a\DupCleaner\DupCleaner\build\tests\dupcleaner_cli_integration.vcxproj]
D:\a\DupCleaner\DupCleaner\tests\test_cli_integration.cpp(41,36): warning C4996: 'getenv': This function or variable may be unsafe. Consider using _dupenv_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details. [D:\a\DupCleaner\DupCleaner\build\tests\dupcleaner_cli_integration.vcxproj]
  dupcleaner_cli_integration.vcxproj -> D:\a\DupCleaner\DupCleaner\build\tests\Release\dupcleaner_cli_integration.exe
  Building Custom Rule D:/a/DupCleaner/DupCleaner/tests/CMakeLists.txt
  test_placeholder.cpp
  test_file_entry.cpp
  test_scanner.cpp
  test_duplicate_finder.cpp
D:\a\DupCleaner\DupCleaner\tests\test_duplicate_finder.cpp(139,41): warning C4267: 'argument': conversion from 'size_t' to 'int', possible loss of data [D:\a\DupCleaner\DupCleaner\build\tests\dupcleaner_tests.vcxproj]
  test_hasher.cpp
  test_cli_args.cpp
D:\a\DupCleaner\DupCleaner\tests\test_cli_args.cpp(2,10): error C1083: Cannot open include file: 'CLI/CLI.hpp': No such file or directory [D:\a\DupCleaner\DupCleaner\build\tests\dupcleaner_tests.vcxproj]
  Generating Code...
  gmock.vcxproj -> D:\a\DupCleaner\DupCleaner\build\lib\Release\gmock.lib
  gmock_main.vcxproj -> D:\a\DupCleaner\DupCleaner\build\lib\Release\gmock_main.lib
Error: Process completed with exit code 1.
