#include "selection_logic.h"

namespace dupcleaner::gui {

std::set<std::string> calculateInitialSelections(
    const std::vector<std::vector<FileEntry>>& exact,
    const DuplicateFinder::NearDuplicateResult& near_dup,
    KeepStrategy strategy
) {
    std::set<std::string> selected;
    SafeDeleter temp_deleter("");

    // Exact duplicates
    if (!exact.empty()) {
        auto plan = temp_deleter.planDeletion(exact, strategy);
        for (const auto& p : plan.delete_files) {
            selected.insert(p.string());
        }
    }

    // Near duplicates - SafeDeleter expects vector<vector<FileEntry>>
    if (!near_dup.groups.empty()) {
        std::vector<std::vector<FileEntry>> near_groups;
        for (const auto& group : near_dup.groups) {
            std::vector<FileEntry> entries;
            for (const auto& m : group.members) {
                entries.push_back(m.first);
            }
            near_groups.push_back(entries);
        }
        auto plan = temp_deleter.planDeletion(near_groups, strategy);
        for (const auto& p : plan.delete_files) {
            selected.insert(p.string());
        }
    }

    return selected;
}

uintmax_t calculateSelectedWastedSpace(
    const std::set<std::string>& selected_paths,
    const std::vector<std::vector<FileEntry>>& exact,
    const DuplicateFinder::NearDuplicateResult& near_dup
) {
    uintmax_t total = 0;

    for (const auto& group : exact) {
        for (const auto& file : group) {
            if (selected_paths.count(file.path.string())) {
                total += file.size;
            }
        }
    }

    for (const auto& group : near_dup.groups) {
        for (const auto& member : group.members) {
            if (selected_paths.count(member.first.path.string())) {
                total += member.first.size;
            }
        }
    }

    return total;
}

} // namespace dupcleaner::gui
