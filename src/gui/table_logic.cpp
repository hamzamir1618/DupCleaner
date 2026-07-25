#include "table_logic.h"
#include <algorithm>

namespace dupcleaner::gui {

void SortFileEntries(std::vector<FileEntry>& entries, TableColumn column, bool descending) {
    auto comparator = [column, descending](const FileEntry& a, const FileEntry& b) {
        bool result = false;
        switch (column) {
            case TableColumn::Path:
                result = a.path.u8string() < b.path.u8string();
                break;
            case TableColumn::Size:
                if (a.size != b.size) {
                    result = a.size < b.size;
                } else {
                    result = a.path.u8string() < b.path.u8string();
                }
                break;
            case TableColumn::LastModified:
                if (a.last_modified != b.last_modified) {
                    result = a.last_modified < b.last_modified;
                } else {
                    result = a.path.u8string() < b.path.u8string();
                }
                break;
        }
        return descending ? !result : result;
    };
    
    std::sort(entries.begin(), entries.end(), comparator);
}

} // namespace dupcleaner::gui
