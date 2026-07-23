#pragma once

#include <vector>
#include <filesystem>
#include <string>
#include "dupcleaner/file_entry.h"

namespace dupcleaner {

enum class KeepStrategy {
    KeepOldest,
    KeepNewest,
    KeepFirstAlphabetically
};

struct DeletionPlan {
    std::vector<std::filesystem::path> keep_files;
    std::vector<std::filesystem::path> delete_files;
};

class SafeDeleter {
public:
    explicit SafeDeleter(const std::filesystem::path& trashRoot);

    // Determines which files to keep and which to delete based on the strategy.
    DeletionPlan planDeletion(const std::vector<std::vector<FileEntry>>& duplicateGroups, KeepStrategy strategy) const;

    // Executes the deletion plan. If moveToTrash is true, moves files to .dupcleaner_trash.
    // Returns true on success.
    bool execute(const DeletionPlan& plan, bool moveToTrash);

    // Restores the most recent batch of moved files.
    bool undoLastDeletion();

private:
    std::filesystem::path trash_root_;
    
    // Helper to generate a unique batch ID
    std::string generateBatchId() const;
};

} // namespace dupcleaner
