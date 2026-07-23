#pragma once

#include <set>
#include <string>
#include <vector>
#include <cstdint>
#include "dupcleaner/file_entry.h"
#include "duplicate_finder.h"
#include "deleter.h"

namespace dupcleaner::gui {

/**
 * Calculates the initial set of file paths that should be checked for deletion,
 * delegating the decision to SafeDeleter::planDeletion.
 */
std::set<std::string> calculateInitialSelections(
    const std::vector<std::vector<FileEntry>>& exact,
    const DuplicateFinder::NearDuplicateResult& near_dup,
    KeepStrategy strategy
);

/**
 * Calculates the total size of all files that are currently selected for deletion.
 */
uintmax_t calculateSelectedWastedSpace(
    const std::set<std::string>& selected_paths,
    const std::vector<std::vector<FileEntry>>& exact,
    const DuplicateFinder::NearDuplicateResult& near_dup
);

} // namespace dupcleaner::gui
