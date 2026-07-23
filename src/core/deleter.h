#pragma once

#include <vector>
#include <filesystem>
#include <string>
#include "dupcleaner/file_entry.h"

namespace dupcleaner {

enum class KeepStrategy : std::uint8_t {
    KeepOldest,
    KeepNewest,
    KeepFirstAlphabetically
};

struct DeletionPlan {
    std::vector<std::filesystem::path> keep_files;
    std::vector<std::filesystem::path> delete_files;
};

struct DeletionResult {
    bool success;
    std::vector<std::filesystem::path> failed_files;
};

class SafeDeleter {
public:
    explicit SafeDeleter(const std::filesystem::path& trashRoot);

    // Determines which files to keep and which to delete based on the strategy.
    DeletionPlan planDeletion(const std::vector<std::vector<FileEntry>>& duplicateGroups, KeepStrategy strategy) const;

    // Executes the deletion plan. If moveToTrash is true, moves files to .dupcleaner_trash.
    // Returns DeletionResult with overall success and a list of files that failed to delete.
    DeletionResult execute(const DeletionPlan& plan, bool moveToTrash);

    // Restores the most recent batch of moved files.
    bool undoLastDeletion();

private:
    std::filesystem::path trash_root_;
    
    // Helper to generate a unique batch ID
    std::string generateBatchId() const;
};

} // namespace dupcleaner
