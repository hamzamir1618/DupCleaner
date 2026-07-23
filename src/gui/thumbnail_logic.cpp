#include "thumbnail_logic.h"
#include <string>
#include <algorithm>

namespace dupcleaner::gui {

bool isImageGroup(const std::vector<FileEntry>& group) {
    if (group.empty()) return false;
    
    std::string ext = group[0].path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    return ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp" || ext == ".gif";
}

std::vector<std::filesystem::path> getPathsForThumbnails(const std::vector<FileEntry>& group, bool isNearDuplicate) {
    std::vector<std::filesystem::path> paths;
    if (group.empty()) return paths;

    if (isNearDuplicate) {
        for (const auto& entry : group) {
            paths.push_back(entry.path);
        }
    } else {
        paths.push_back(group[0].path);
    }
    return paths;
}

} // namespace dupcleaner::gui
