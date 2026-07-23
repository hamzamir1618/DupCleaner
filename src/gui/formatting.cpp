#include "formatting.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace dupcleaner::gui {

std::string formatBytes(uintmax_t bytes) {
    if (bytes == 0) return "0 B";

    const char* suffixes[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int suffixIndex = 0;
    double count = static_cast<double>(bytes);

    while (count >= 1024.0 && suffixIndex < 5) {
        count /= 1024.0;
        suffixIndex++;
    }

    std::ostringstream out;
    if (suffixIndex == 0) {
        out << bytes << " " << suffixes[suffixIndex];
    } else {
        out << std::fixed << std::setprecision(2) << count << " " << suffixes[suffixIndex];
    }
    return out.str();
}

uintmax_t calculateWastedSpace(const std::vector<FileEntry>& group) {
    if (group.size() <= 1) return 0;
    return group[0].size * (group.size() - 1);
}

void sortGroupsByWastedSpaceDescending(std::vector<std::vector<FileEntry>>& groups) {
    std::sort(groups.begin(), groups.end(), [](const std::vector<FileEntry>& a, const std::vector<FileEntry>& b) {
        return calculateWastedSpace(a) > calculateWastedSpace(b);
    });
}

} // namespace dupcleaner::gui
