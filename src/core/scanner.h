#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include "dupcleaner/file_entry.h"

namespace dupcleaner {

class DirectoryScanner {
public:
    std::vector<FileEntry> scan(const std::filesystem::path& root);
    const std::vector<std::string>& getSkippedPaths() const;

private:
    std::vector<std::string> skipped_paths_;
};

} // namespace dupcleaner
