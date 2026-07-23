#include "duplicate_finder.h"
#include <fstream>
#include <cstring>

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

} // namespace dupcleaner
