#pragma once

#include <vector>
#include <filesystem>
#include "dupcleaner/file_entry.h"

namespace dupcleaner::gui {

/**
 * Returns true if the group contains image files based on the extension of the first file.
 * We assume groups contain files of the same type.
 */
bool isImageGroup(const std::vector<FileEntry>& group);

/**
 * Returns the paths of the files in the group that need thumbnails rendered.
 * For exact duplicates, only the first file's thumbnail is needed.
 * For near-duplicates, all files' thumbnails are needed.
 */
std::vector<std::filesystem::path> getPathsForThumbnails(const std::vector<FileEntry>& group, bool isNearDuplicate);

} // namespace dupcleaner::gui
