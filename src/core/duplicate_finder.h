#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>
#include <filesystem>
#include "dupcleaner/file_entry.h"

#include <map>

#include <atomic>

#include <functional>

namespace dupcleaner {

enum class PipelinePhase {
    Scanning,
    GroupingBySize,
    Fingerprinting,
    Verifying,
    ComputingPerceptualHashes,
    ClusteringNearDuplicates,
    Done
};

using ProgressCallback = std::function<void(PipelinePhase phase, size_t current, size_t total)>;

class DuplicateFinder {
public:
    struct PhaseProgressTracker {
        std::atomic<uintmax_t>* current_items;
        std::atomic<uintmax_t>* total_items;
        std::atomic<uintmax_t>* current_bytes;
        PhaseProgressTracker() : current_items(nullptr), total_items(nullptr), current_bytes(nullptr) {}
    };
    // Buckets file entries by their exact size in bytes.
    static std::map<uintmax_t, std::vector<FileEntry>> groupBySize(const std::vector<FileEntry>& entries);

    // Filters out buckets that only contain a single entry (since they cannot be duplicates).
    static void filterUniqueSizes(std::map<uintmax_t, std::vector<FileEntry>>& buckets);

    // Compares two files byte-for-byte in chunks. Returns true if they are identical.
    static bool filesAreIdentical(const std::filesystem::path& a, const std::filesystem::path& b);

    // Phase 2: Compute fingerprints for candidate files
    static std::map<uintmax_t, std::unordered_map<uint64_t, std::vector<FileEntry>>> fingerprintCandidates(const std::map<uintmax_t, std::vector<FileEntry>>& size_buckets, const PhaseProgressTracker& progress = PhaseProgressTracker(), ProgressCallback onProgress = nullptr);

    // Phase 3: Byte verification for hashed candidates
    static std::vector<std::vector<FileEntry>> verifyExactDuplicates(const std::map<uintmax_t, std::unordered_map<uint64_t, std::vector<FileEntry>>>& hash_buckets, const PhaseProgressTracker& progress = PhaseProgressTracker(), ProgressCallback onProgress = nullptr);

    // Finds and returns all groups of exact duplicates using size, hashing, and byte comparison (Convenience method).
    static std::vector<std::vector<FileEntry>> findExactDuplicates(const std::vector<FileEntry>& entries, const PhaseProgressTracker& progress = PhaseProgressTracker(), ProgressCallback onProgress = nullptr);

    struct NearDuplicateGroup {
        std::vector<std::pair<FileEntry, uint64_t>> members;
    };

    struct NearDuplicateResult {
        std::vector<NearDuplicateGroup> groups;
        std::vector<std::filesystem::path> skipped_paths;
    };

    // Finds near-duplicate images within the specified hamming threshold.
    static NearDuplicateResult findNearDuplicateImages(const std::vector<FileEntry>& entries, int hammingThreshold = 5, const PhaseProgressTracker& progress = PhaseProgressTracker(), ProgressCallback onProgress = nullptr);
};

} // namespace dupcleaner
