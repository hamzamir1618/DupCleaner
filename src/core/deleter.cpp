#include "deleter.h"
#include <algorithm>
#include <fstream>
#include <chrono>
#include <random>
#include "nlohmann/json.hpp"

namespace dupcleaner {

SafeDeleter::SafeDeleter(const std::filesystem::path& trashRoot)
    : trash_root_(trashRoot) {}

DeletionPlan SafeDeleter::planDeletion(const std::vector<std::vector<FileEntry>>& duplicateGroups, KeepStrategy strategy) const {
    DeletionPlan plan;

    for (const auto& group : duplicateGroups) {
        if (group.empty()) continue;
        if (group.size() == 1) {
            plan.keep_files.push_back(group[0].path);
            continue;
        }

        auto sortedGroup = group;
        if (strategy == KeepStrategy::KeepOldest) {
            std::sort(sortedGroup.begin(), sortedGroup.end(), [](const FileEntry& a, const FileEntry& b) {
                if (a.last_modified == b.last_modified) return a.path < b.path;
                return a.last_modified < b.last_modified;
            });
        } else if (strategy == KeepStrategy::KeepNewest) {
            std::sort(sortedGroup.begin(), sortedGroup.end(), [](const FileEntry& a, const FileEntry& b) {
                if (a.last_modified == b.last_modified) return a.path < b.path;
                return a.last_modified > b.last_modified;
            });
        } else if (strategy == KeepStrategy::KeepFirstAlphabetically) {
            std::sort(sortedGroup.begin(), sortedGroup.end(), [](const FileEntry& a, const FileEntry& b) {
                return a.path < b.path;
            });
        }

        // The first element is the one we keep
        plan.keep_files.push_back(sortedGroup[0].path);
        
        // The rest are marked for deletion
        for (size_t i = 1; i < sortedGroup.size(); ++i) {
            plan.delete_files.push_back(sortedGroup[i].path);
        }
    }

    return plan;
}

std::string SafeDeleter::generateBatchId() const {
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(1000, 9999);
    return "batch_" + std::to_string(now) + "_" + std::to_string(dis(gen));
}

DeletionResult SafeDeleter::execute(const DeletionPlan& plan, bool moveToTrash) {
    DeletionResult result;
    result.success = true;

    if (plan.delete_files.empty()) return result;

    if (!moveToTrash) {
        // Permanent deletion
        for (const auto& file : plan.delete_files) {
            std::error_code ec;
            if (std::filesystem::exists(file, ec)) {
                std::filesystem::remove(file, ec);
                if (ec) {
                    result.failed_files.push_back(file);
                }
            } else if (ec) {
                result.failed_files.push_back(file);
            }
        }
        return result;
    }

    // Move to trash
    std::filesystem::path trashDir = trash_root_ / ".dupcleaner_trash";
    std::error_code ec;
    std::filesystem::create_directories(trashDir, ec);
    if (ec) {
        result.success = false;
        result.failed_files = plan.delete_files;
        return result;
    }

    std::string batchId = generateBatchId();
    nlohmann::json manifest;
    manifest["batch_id"] = batchId;
    manifest["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
    manifest["files"] = nlohmann::json::object();

    for (const auto& originalPath : plan.delete_files) {
        if (!std::filesystem::exists(originalPath, ec) || ec) {
            if (ec) result.failed_files.push_back(originalPath);
            continue;
        }

        // Generate a random unique filename in the trash
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint64_t> dis;
        std::string trashFilename = "trashed_" + std::to_string(dis(gen));
        std::filesystem::path trashFilePath = trashDir / trashFilename;

        // Move the file
        std::filesystem::rename(originalPath, trashFilePath, ec);
        if (!ec) {
            // Success, record in manifest
            manifest["files"][trashFilename] = originalPath.string(); // Using string() to correctly store unicode paths in JSON
        } else {
            result.failed_files.push_back(originalPath);
        }
    }

    // Save manifest
    std::filesystem::path manifestPath = trashDir / (batchId + ".json");
    std::ofstream out(manifestPath);
    if (out) {
        out << manifest.dump(4);
    } else {
        result.success = false;
        return result;
    }

    return result;
}

bool SafeDeleter::undoLastDeletion() {
    std::filesystem::path trashDir = trash_root_ / ".dupcleaner_trash";
    if (!std::filesystem::exists(trashDir)) return false;

    std::filesystem::path latestManifest;
    uintmax_t latestTime = 0;

    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(trashDir, ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            std::ifstream in(entry.path());
            if (in) {
                nlohmann::json manifest;
                try {
                    in >> manifest;
                    if (manifest.contains("timestamp")) {
                        uintmax_t ts = manifest["timestamp"].get<uintmax_t>();
                        if (ts >= latestTime) { // >= ensures we pick the latest even if timestamps tie
                            latestTime = ts;
                            latestManifest = entry.path();
                        }
                    }
                } catch (...) { // NOLINT(bugprone-empty-catch)
                    // Ignore malformed
                }
            }
        }
    }

    if (latestManifest.empty()) return false;

    std::ifstream in(latestManifest);
    nlohmann::json manifest;
    try {
        in >> manifest;
    } catch (...) {
        return false;
    }

    if (!manifest.contains("files")) return false;

    // Restore files
    auto files = manifest["files"];
    for (auto it = files.begin(); it != files.end(); ++it) {
        std::filesystem::path trashFilePath = trashDir / it.key();
        
        // Handle utf-8 string correctly
        std::string path_str = it.value().get<std::string>();
        std::filesystem::path originalPath(path_str);

        if (std::filesystem::exists(trashFilePath)) {
            // Ensure original directory exists
            std::filesystem::create_directories(originalPath.parent_path(), ec);
            std::filesystem::rename(trashFilePath, originalPath, ec);
        }
    }

    // Delete the manifest
    std::filesystem::remove(latestManifest, ec);

    return true;
}

} // namespace dupcleaner
