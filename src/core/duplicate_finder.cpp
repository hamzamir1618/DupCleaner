#include "duplicate_finder.h"

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

} // namespace dupcleaner
