#include "duplicate_finder.h"
#include <fstream>
#include <cstring>
#include "hasher.h"
#include "dupcleaner/exceptions.h"

namespace dupcleaner {

std::unordered_map<uintmax_t, std::vector<FileEntry>> DuplicateFinder::groupBySize(const std::vector<FileEntry>& entries) {
    std::unordered_map<uintmax_t, std::vector<FileEntry>> buckets;
    for (const auto& entry : entries) {
        buckets[entry.size].push_back(entry);
    }
    return buckets;
}

void DuplicateFinder::filterUniqueSizes(std::unordered_map<uintmax_t, std::vector<FileEntry>>& buckets) {
    for (auto it = buckets.begin(); it != buckets.end(); ) {
        if (it->second.size() <= 1) {
            it = buckets.erase(it);
        } else {
            ++it;
        }
    }
}

bool DuplicateFinder::filesAreIdentical(const std::filesystem::path& a, const std::filesystem::path& b) {
    // Trivial self-comparison
    if (a == b) {
        return true;
    }

    std::error_code ec_a, ec_b;
    auto size_a = std::filesystem::file_size(a, ec_a);
    auto size_b = std::filesystem::file_size(b, ec_b);

    if (ec_a || ec_b || size_a != size_b) {
        return false; // Fast short-circuit if sizes differ or we can't stat them
    }

    if (size_a == 0) {
        return true; // Both zero-byte files
    }

    std::ifstream file_a(a, std::ios::binary);
    std::ifstream file_b(b, std::ios::binary);

    if (!file_a || !file_b) {
        return false;
    }

    constexpr size_t BUFFER_SIZE = 64 * 1024;
    std::vector<char> buf_a(BUFFER_SIZE);
    std::vector<char> buf_b(BUFFER_SIZE);

    while (file_a && file_b) {
        file_a.read(buf_a.data(), BUFFER_SIZE);
        file_b.read(buf_b.data(), BUFFER_SIZE);

        if (file_a.gcount() != file_b.gcount()) {
            return false;
        }

        if (file_a.gcount() == 0) {
            break;
        }

        if (std::memcmp(buf_a.data(), buf_b.data(), file_a.gcount()) != 0) {
            return false;
        }
    }

    return true;
}

std::vector<std::vector<FileEntry>> DuplicateFinder::findExactDuplicates(const std::vector<FileEntry>& entries) {
    std::vector<std::vector<FileEntry>> results;
    
    // 1. Group by size
    auto size_buckets = groupBySize(entries);
    filterUniqueSizes(size_buckets);

    // 2. Iterate through size buckets
    for (const auto& [size, size_group] : size_buckets) {
        
        // 3. Sub-group by fingerprint
        std::unordered_map<uint64_t, std::vector<FileEntry>> hash_buckets;
        for (const auto& entry : size_group) {
            try {
                uint64_t hash = FileHasher::fingerprint(entry.path);
                hash_buckets[hash].push_back(entry);
            } catch (const DupCleanerIOException&) {
                // If we can't hash it, silently skip it for duplicate consideration
                continue; 
            }
        }
        
        // 4. Verify identical bytes
        for (const auto& [hash, hash_group] : hash_buckets) {
            if (hash_group.size() <= 1) continue;

            // Resolve potential hash collisions
            std::vector<std::vector<FileEntry>> confirmed_groups;

            for (const auto& entry : hash_group) {
                bool added = false;
                for (auto& confirmed_group : confirmed_groups) {
                    if (filesAreIdentical(entry.path, confirmed_group[0].path)) {
                        confirmed_group.push_back(entry);
                        added = true;
                        break;
                    }
                }
                
                if (!added) {
                    confirmed_groups.push_back({entry});
                }
            }

            // 5. Output confirmed groups
            for (auto& confirmed_group : confirmed_groups) {
                if (confirmed_group.size() > 1) {
                    results.push_back(std::move(confirmed_group));
                }
            }
        }
    }

    return results;
}

} // namespace dupcleaner
