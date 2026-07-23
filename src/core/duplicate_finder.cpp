#include "duplicate_finder.h"
#include <fstream>
#include <cstring>
#include "hasher.h"
#include "image_loader.h"
#include "perceptual_hash.h"
#include "dupcleaner/exceptions.h"
#include <numeric>
#include <algorithm>
#include <cctype>
#include <map>
#include "thread_pool.h"

namespace dupcleaner {

std::map<uintmax_t, std::vector<FileEntry>> DuplicateFinder::groupBySize(const std::vector<FileEntry>& entries) {
    std::map<uintmax_t, std::vector<FileEntry>> buckets;
    for (const auto& entry : entries) {
        buckets[entry.size].push_back(entry);
    }
    return buckets;
}

void DuplicateFinder::filterUniqueSizes(std::map<uintmax_t, std::vector<FileEntry>>& buckets) {
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

    constexpr size_t BUFFER_SIZE = 64ULL * 1024ULL;
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
    
    // 1. Group by size (uses std::map to ensure deterministic ordering of tasks)
    auto size_buckets = groupBySize(entries);
    filterUniqueSizes(size_buckets);

    auto& pool = getGlobalThreadPool();
    std::vector<std::future<std::vector<std::vector<FileEntry>>>> futures;
    futures.reserve(size_buckets.size());

    // 2. Iterate through size buckets and enqueue tasks
    for (const auto& [size, size_group] : size_buckets) {
        futures.push_back(pool.enqueue([size_group]() {
            std::vector<std::vector<FileEntry>> local_results;
            
            // 3. Sub-group by fingerprint
            std::unordered_map<uint64_t, std::vector<FileEntry>> hash_buckets;
            for (const auto& entry : size_group) {
                try {
                    uint64_t hash = FileHasher::fingerprint(entry.path);
                    hash_buckets[hash].push_back(entry);
                } catch (const DupCleanerIOException&) {
                    continue; 
                }
            }
            
            // 4. Verify identical bytes
            for (const auto& [hash, hash_group] : hash_buckets) {
                if (hash_group.size() <= 1) continue;

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
                        // Sort files within the group to ensure stable order
                        std::sort(confirmed_group.begin(), confirmed_group.end(), [](const FileEntry& a, const FileEntry& b) {
                            return a.path.string() < b.path.string();
                        });
                        local_results.push_back(std::move(confirmed_group));
                    }
                }
            }
            return local_results;
        }));
    }

    // Collect all results
    for (auto& f : futures) {
        auto bucket_results = f.get();
        for (auto& group : bucket_results) {
            results.push_back(std::move(group));
        }
    }

    // Sort final results deterministically
    std::sort(results.begin(), results.end(), [](const std::vector<FileEntry>& a, const std::vector<FileEntry>& b) {
        if (a.empty() || b.empty()) return false;
        return a[0].path.string() < b[0].path.string();
    });

    return results;
}

DuplicateFinder::NearDuplicateResult DuplicateFinder::findNearDuplicateImages(const std::vector<FileEntry>& entries, int hammingThreshold) {
    NearDuplicateResult result;
    std::vector<std::pair<FileEntry, uint64_t>> image_hashes;
    
    auto& pool = getGlobalThreadPool();
    
    struct HashResult {
        bool success;
        FileEntry entry;
        uint64_t hash;
    };
    std::vector<std::future<HashResult>> futures;
    futures.reserve(entries.size());

    for (const auto& entry : entries) {
        auto ext = entry.path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

        if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp") {
            futures.push_back(pool.enqueue([entry]() -> HashResult {
                auto img = ImageLoader::load(entry.path);
                if (!img) {
                    return {false, entry, 0};
                }
                uint64_t hash = PerceptualHash::computeDHash(*img);
                return {true, entry, hash};
            }));
        }
    }
    
    for (size_t i = 0; i < futures.size(); ++i) {
        auto res = futures[i].get();
        if (res.success) {
            image_hashes.push_back({res.entry, res.hash});
        } else {
            result.skipped_paths.push_back(res.entry.path);
        }
    }

    if (image_hashes.empty()) {
        return result;
    }

    // Disjoint Set (Union-Find)
    std::vector<size_t> parent(image_hashes.size());
    std::iota(parent.begin(), parent.end(), 0);

    auto find_parent = [&parent](auto& self, size_t i) -> size_t {
        if (parent[i] == i) return i;
        return parent[i] = self(self, parent[i]);
    };

    auto union_sets = [&](size_t i, size_t j) {
        size_t root_i = find_parent(find_parent, i);
        size_t root_j = find_parent(find_parent, j);
        if (root_i != root_j) {
            parent[root_i] = root_j;
        }
    };

    for (size_t i = 0; i < image_hashes.size(); ++i) {
        for (size_t j = i + 1; j < image_hashes.size(); ++j) {
            if (PerceptualHash::hammingDistance(image_hashes[i].second, image_hashes[j].second) <= hammingThreshold) {
                union_sets(i, j);
            }
        }
    }

    std::unordered_map<size_t, std::vector<std::pair<FileEntry, uint64_t>>> clusters;
    for (size_t i = 0; i < image_hashes.size(); ++i) {
        size_t root = find_parent(find_parent, i);
        clusters[root].push_back(image_hashes[i]);
    }

    for (auto& [root, group] : clusters) {
        if (group.size() > 1) {
            // Sort files within the group to ensure stable order
            std::sort(group.begin(), group.end(), [](const auto& a, const auto& b) {
                return a.first.path.string() < b.first.path.string();
            });
            
            NearDuplicateGroup g;
            g.members = std::move(group);
            result.groups.push_back(std::move(g));
        }
    }
    
    // Sort final results deterministically
    std::sort(result.groups.begin(), result.groups.end(), [](const NearDuplicateGroup& a, const NearDuplicateGroup& b) {
        if (a.members.empty() || b.members.empty()) return false;
        return a.members[0].first.path.string() < b.members[0].first.path.string();
    });

    return result;
}

} // namespace dupcleaner
