#pragma once

#include <filesystem>
#include <cstdint>

namespace dupcleaner {

class FileHasher {
public:
    // Computes xxHash64 over the file contents.
    // Throws DupCleanerIOException if the file cannot be opened.
    static uint64_t fingerprint(const std::filesystem::path& path);

    // Computes xxHash64 over a raw byte buffer (useful for fuzzing or archive ingestion).
    static uint64_t fingerprint(const unsigned char* buffer, size_t length);
};

} // namespace dupcleaner
