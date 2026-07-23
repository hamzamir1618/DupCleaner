# Architecture

## Dependency graph

- **cli** -> `libdupcleaner` -> `xxHash`
- **gui** -> `libdupcleaner`
- **tests** -> `libdupcleaner` + `GoogleTest`

## Filesystem Traversal
The core logic utilizes a robust `DirectoryScanner` class wrapping `std::filesystem::recursive_directory_iterator`. It explicitly skips non-regular files and symlinks to prevent infinite directory recursion, and silently traps permission/read errors on a per-file basis using `std::error_code`. Instead of aborting an entire scan of 100k+ files when encountering a single inaccessible file, it collects warnings in a skipped-paths ledger (`getSkippedPaths()`) ensuring large real-world scans finish successfully.

## Fast Hashing (Fingerprinting)
To quickly sort potential duplicates before performing expensive byte-for-byte comparisons, `dupcleaner` uses `xxHash64` (`FileHasher`). xxHash was chosen specifically for its extreme speed on modern CPUs (often bottlenecking on disk I/O rather than CPU). It is a non-cryptographic hash, which is perfectly fine for this use case since we use it merely as a fast fingerprinting heuristic, and any collisions will be securely verified with a full byte-for-byte comparison before any deletion operations occur.

## Safety Model
When executing deletion operations, `dupcleaner` defaults to moving duplicate files into a localized, hidden `.dupcleaner_trash/` directory situated within the scanned target folder. Along with the files, it generates a JSON manifest (`manifest.json`) that maps the randomized trash filenames back to their exact original absolute paths. This default "trash" behavior guarantees that users can completely reverse accidental deletions via the `undo` command. Permanent deletion (`std::filesystem::remove`) is strictly an opt-in behavior (`--permanent`) and avoids touching any user system-level trash configurations, keeping operations strictly sandboxed to the project directory being analyzed.
