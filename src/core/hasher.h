#pragma once

#include <filesystem>
#include <cstdint>

namespace dupcleaner {

class FileHasher {
public:
    // Computes xxHash64 over the file contents.
    // Throws DupCleanerIOException if the file cannot be opened.
    static uint64_t fingerprint(const std::filesystem::path& path);
};

} // namespace dupcleaner
