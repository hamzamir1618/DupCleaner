#pragma once

#include <filesystem>
#include <optional>
#include <cstdint>

namespace dupcleaner {

struct FileEntry {
    std::filesystem::path path;
    std::uintmax_t size{0};
    std::filesystem::file_time_type last_modified;
    std::optional<uint64_t> fast_hash_fingerprint;

    bool operator==(const FileEntry& other) const {
        return path == other.path &&
               size == other.size &&
               last_modified == other.last_modified &&
               fast_hash_fingerprint == other.fast_hash_fingerprint;
    }

    bool operator!=(const FileEntry& other) const {
        return !(*this == other);
    }
};

} // namespace dupcleaner
