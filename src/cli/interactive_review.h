#pragma once

#include <vector>
#include <iostream>
#include "duplicate_finder.h"
#include "dupcleaner/file_entry.h"

namespace dupcleaner::cli {

struct GroupDecision {
    bool skip{false};
    std::vector<FileEntry> keep_files;
    std::vector<FileEntry> delete_files;
};

// Given a group of duplicate files and the index of the file suggested to be kept,
// interacts with the user via in/out streams to determine which files to keep/delete.
GroupDecision resolveGroupInteractively(
    const std::vector<FileEntry>& group,
    size_t suggested_keep_index,
    std::istream& in,
    std::ostream& out
);

} // namespace dupcleaner::cli
