# Application Hardening Log

This document tracks fixes for memory safety, undefined behavior, and potential vulnerabilities identified through code reviews and Sanitizer runs (ASan/UBSan).

## Sanitizer Integrations
We have integrated a dedicated GitHub Actions workflow (`sanitizer-test`) that builds the test suite using Clang with `-fsanitize=address,undefined` enabled. This ensures that every commit is automatically vetted for buffer overruns, use-after-free, memory leaks, and undefined integer arithmetic.

## Code Review Findings & Fixes

### 1. `ImageLoader::load` Buffer Handling
**Analysis**: `stbi_load` allocates raw C-arrays on the heap. We correctly copy these raw bytes into a modern `std::vector<unsigned char>` and call `stbi_image_free` immediately after to prevent memory leaks and dangling pointers. 
**Fix**: None needed; the implementation was already safe. However, we explicitly cast `width`, `height`, and `channels` to `size_t` prior to multiplying them to prevent 32-bit signed integer overflow (`UBSan`) if an attacker provided a massive image.

### 2. `FileHasher::fingerprint` Chunked Reads
**Analysis**: Chunked binary reading from `std::ifstream` using `.read()` and checking `.gcount()` can be a common source of buffer over-reads if EOF state isn't handled accurately.
**Fix**: No buffer overflow issues were found. We successfully check `file.gcount() > 0` after the `while` loop finishes to catch any trailing bytes that were read when `failbit/eofbit` triggered. The buffer is statically sized to 64KB, preventing out-of-bounds array access.

### 3. `PerceptualHash::computeDHash` Array Indexing
**Analysis**: The pixel looping logic `int idx = (sy * img.width + sx) * img.channels;` was susceptible to signed integer overflow (`UBSan`) when encountering exceptionally large images, as the multiplication was happening within 32-bit signed bounds before array indexing. 
**Fix**: Updated the pixel index calculation to explicitly cast the coordinates to `size_t` (`size_t idx = (static_cast<size_t>(sy) * static_cast<size_t>(img.width) + static_cast<size_t>(sx)) * static_cast<size_t>(img.channels);`), guaranteeing that we never overflow the index on large multi-gigapixel images. Array bounds for RGB channel access were analytically verified to never exceed `img.data.size()`.

## Static Analysis (Clang-Tidy & Cppcheck)

We introduced a `static-analysis` CI job running both `clang-tidy` and `cppcheck` to programmatically catch logic errors.

### Baseline & Suppression Strategy
To prevent stylistic nits from failing CI retroactively on existing code, we established a targeted baseline:
- **Clang-Tidy**: Configured `.clang-tidy` to strictly enable `bugprone-*`, `performance-*`, and explicit `modernize-*/cppcoreguidelines-*` rules (like bounds-decay and uninitialized variables), while globally treating all active warnings as errors (`WarningsAsErrors: '*'`).
- **Cppcheck**: Configured to run with `--enable=warning,performance,portability` acting as a second-pass safety net for deeper static control-flow analysis not caught by clang. We suppressed noisy missing system include warnings (`--suppress=missingIncludeSystem`).

### 4. Uninitialized Primitive Variables
**Analysis**: Static analysis flagged that raw scalar primitives passed by pointer into `stb_image` (e.g., `int w, h, c;`) were declared without initialization, which breaches `cppcoreguidelines-init-variables` and poses a risk of undefined behavior if the loader failed without modifying them. Similarly, `gray_9x8` was an uninitialized C-style 2D array.
**Fix**: explicitly initialized `int w = 0, h = 0, c = 0;` in `ImageLoader::load`. Replaced the C-style array with `std::array<std::array<int, 9>, 8> gray_9x8 = {};` in `PerceptualHash::computeDHash` to securely zero-initialize memory and strictly prevent array-to-pointer decay bounds issues.

### 5. Clang-Tidy Deep Dive Pass
**Analysis**: After strictly parsing `src/` and `include/`, clang-tidy identified several edge-case performance and bug-prone anti-patterns.
**Fixes Applied**:
- **`bugprone-exception-escape`**: Both `main()` functions (`debug_cli.cpp` and `main.cpp`) failed to wrap their CLI library execution in a top-level `try-catch` block, which could lead to an ungraceful crash. Wrapped them correctly to return `1`.
- **`performance-inefficient-vector-operation`**: Multi-threaded result aggregations in `duplicate_finder.cpp` and `main.cpp` used `.push_back` inside loops dynamically allocating thousands of vector entries. Added `.reserve(size)` before loops to eliminate intermediate memory reallocation thrashing.
- **`performance-avoid-endl`**: Replaced standard library `std::endl` flushes with fast `'\n'` character injections where appropriate.
- **`bugprone-implicit-widening-of-multiplication-result`**: Identified chunk size definitions (`64 * 1024`) implicitly converting to 64-bit `size_t`. Replaced with explicit literal width modifiers (`64ULL * 1024ULL`).
- **`performance-unnecessary-copy-initialization`**: Filesystem loop assignments (`fs::path p = entry.path()`) performed deep copies. Upgraded to `const fs::path& p`.
- **`bugprone-empty-catch`**: Intentionally ignored malformed JSON files when walking the `.dupcleaner_trash` directory. Exonerated explicitly with a `// NOLINT` pragma.
- **`HeaderFilterRegex`**: Clang-tidy was originally bleeding its strict ruleset into `third_party` dependencies (like `nlohmann_json`). Fixed the YAML regex boundary to strictly anchor `^src/.*|^include/.*`.
