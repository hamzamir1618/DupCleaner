#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>
#include "dupcleaner/file_entry.h"

namespace dupcleaner {

class DuplicateFinder {
public:
    // Buckets file entries by their exact size in bytes.
    static std::unordered_map<uintmax_t, std::vector<FileEntry>> groupBySize(const std::vector<FileEntry>& entries);

    // Filters out buckets that only contain a single entry (since they cannot be duplicates).
    static void filterUniqueSizes(std::unordered_map<uintmax_t, std::vector<FileEntry>>& buckets);

    // Compares two files byte-for-byte in chunks. Returns true if they are identical.
    static bool filesAreIdentical(const std::filesystem::path& a, const std::filesystem::path& b);

    // Finds and returns all groups of exact duplicates using size, hashing, and byte comparison.
    static std::vector<std::vector<FileEntry>> findExactDuplicates(const std::vector<FileEntry>& entries);
};

} // namespace dupcleaner
