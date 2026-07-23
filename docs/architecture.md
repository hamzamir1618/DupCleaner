# Architecture

## Dependency graph

- **cli** -> `libdupcleaner` -> `xxHash`
- **gui** -> `libdupcleaner`
- **tests** -> `libdupcleaner` + `GoogleTest`

## Filesystem Traversal
The core logic utilizes a robust `DirectoryScanner` class wrapping `std::filesystem::recursive_directory_iterator`. It explicitly skips non-regular files and symlinks to prevent infinite directory recursion, and silently traps permission/read errors on a per-file basis using `std::error_code`. Instead of aborting an entire scan of 100k+ files when encountering a single inaccessible file, it collects warnings in a skipped-paths ledger (`getSkippedPaths()`) ensuring large real-world scans finish successfully.
