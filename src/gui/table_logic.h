#pragma once

#include <vector>
#include "dupcleaner/file_entry.h"

namespace dupcleaner::gui {

enum class TableColumn {
    Path,
    Size,
    LastModified
};

void SortFileEntries(std::vector<FileEntry>& entries, TableColumn column, bool descending);

} // namespace dupcleaner::gui
