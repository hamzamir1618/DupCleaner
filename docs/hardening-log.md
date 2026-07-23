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
