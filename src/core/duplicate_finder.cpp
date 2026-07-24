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

std::map<uintmax_t, std::unordered_map<uint64_t, std::vector<FileEntry>>> 
DuplicateFinder::fingerprintCandidates(const std::map<uintmax_t, std::vector<FileEntry>>& size_buckets, const PhaseProgressTracker& progress) {
    auto& pool = getGlobalThreadPool();
    
    struct HashResult {
        uintmax_t size;
        std::unordered_map<uint64_t, std::vector<FileEntry>> hash_buckets;
    };
    std::vector<std::future<HashResult>> futures;
    futures.reserve(size_buckets.size());

    size_t total_to_hash = 0;
    for (const auto& [size, group] : size_buckets) {
        total_to_hash += group.size();
    }
    if (progress.total_items) progress.total_items->store(total_to_hash, std::memory_order_relaxed);
    if (progress.current_items) progress.current_items->store(0, std::memory_order_relaxed);
    if (progress.current_bytes) progress.current_bytes->store(0, std::memory_order_relaxed);

    for (const auto& [size, size_group] : size_buckets) {
        futures.push_back(pool.enqueue([size, size_group, &progress]() {
            HashResult res;
            res.size = size;
            for (const auto& entry : size_group) {
                try {
                    uint64_t hash = FileHasher::fingerprint(entry.path);
                    res.hash_buckets[hash].push_back(entry);
                } catch (const DupCleanerIOException&) {
                    // skip
                }
                if (progress.current_items) progress.current_items->fetch_add(1, std::memory_order_relaxed);
                if (progress.current_bytes) progress.current_bytes->fetch_add(entry.size, std::memory_order_relaxed);
            }
            return res;
        }));
    }

    std::map<uintmax_t, std::unordered_map<uint64_t, std::vector<FileEntry>>> result;
    for (auto& f : futures) {
        auto res = f.get();
        result[res.size] = std::move(res.hash_buckets);
    }
    return result;
}

std::vector<std::vector<FileEntry>> 
DuplicateFinder::verifyExactDuplicates(const std::map<uintmax_t, std::unordered_map<uint64_t, std::vector<FileEntry>>>& hash_buckets, const PhaseProgressTracker& progress) {
    auto& pool = getGlobalThreadPool();
    std::vector<std::future<std::vector<std::vector<FileEntry>>>> futures;
    
    size_t total_to_verify = 0;
    for (const auto& [size, hb] : hash_buckets) {
        for (const auto& [hash, group] : hb) {
            if (group.size() > 1) {
                total_to_verify += group.size();
            }
        }
    }
    
    if (progress.total_items) progress.total_items->store(total_to_verify, std::memory_order_relaxed);
    if (progress.current_items) progress.current_items->store(0, std::memory_order_relaxed);
    if (progress.current_bytes) progress.current_bytes->store(0, std::memory_order_relaxed);

    for (const auto& [size, hb] : hash_buckets) {
        futures.push_back(pool.enqueue([hb, &progress]() {
            std::vector<std::vector<FileEntry>> local_results;
            for (const auto& [hash, hash_group] : hb) {
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
                    if (progress.current_items) progress.current_items->fetch_add(1, std::memory_order_relaxed);
                    if (progress.current_bytes) progress.current_bytes->fetch_add(entry.size, std::memory_order_relaxed);
                }

                for (auto& confirmed_group : confirmed_groups) {
                    if (confirmed_group.size() > 1) {
                        std::sort(confirmed_group.begin(), confirmed_group.end(), [](const FileEntry& a, const FileEntry& b) {
                            return a.path.u8string() < b.path.u8string();
                        });
                        local_results.push_back(std::move(confirmed_group));
                    }
                }
            }
            return local_results;
        }));
    }

    std::vector<std::vector<FileEntry>> results;
    for (auto& f : futures) {
        auto bucket_results = f.get();
        for (auto& group : bucket_results) {
            results.push_back(std::move(group));
        }
    }

    std::sort(results.begin(), results.end(), [](const std::vector<FileEntry>& a, const std::vector<FileEntry>& b) {
        if (a.empty() || b.empty()) return false;
        return a[0].path.u8string() < b[0].path.u8string();
    });

    return results;
}

std::vector<std::vector<FileEntry>> DuplicateFinder::findExactDuplicates(const std::vector<FileEntry>& entries, const PhaseProgressTracker& progress) {
    auto size_buckets = groupBySize(entries);
    filterUniqueSizes(size_buckets);
    
    // In the convenience method, we don't have separate progress trackers for each phase, so we just run them.
    auto hash_buckets = fingerprintCandidates(size_buckets);
    return verifyExactDuplicates(hash_buckets);
}

DuplicateFinder::NearDuplicateResult DuplicateFinder::findNearDuplicateImages(const std::vector<FileEntry>& entries, int hammingThreshold, const PhaseProgressTracker& progress) {
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
        auto ext = entry.path.extension().u8string();
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
    
    if (progress.total_items) progress.total_items->store(futures.size(), std::memory_order_relaxed);
    if (progress.current_items) progress.current_items->store(0, std::memory_order_relaxed);

    for (size_t i = 0; i < futures.size(); ++i) {
        auto res = futures[i].get();
        if (progress.current_items) progress.current_items->fetch_add(1, std::memory_order_relaxed);
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
                return a.first.path.u8string() < b.first.path.u8string();
            });
            
            NearDuplicateGroup g;
            g.members = std::move(group);
            result.groups.push_back(std::move(g));
        }
    }
    
    // Sort final results deterministically
    std::sort(result.groups.begin(), result.groups.end(), [](const NearDuplicateGroup& a, const NearDuplicateGroup& b) {
        if (a.members.empty() || b.members.empty()) return false;
        return a.members[0].first.path.u8string() < b.members[0].first.path.u8string();
    });

    return result;
}

} // namespace dupcleaner
