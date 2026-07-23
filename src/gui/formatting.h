#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "dupcleaner/file_entry.h"

namespace dupcleaner::gui {

/**
 * Formats a byte count into a human-readable string.
 * Example: 1536 -> "1.50 KB"
 */
std::string formatBytes(uintmax_t bytes);

/**
 * Calculates the total wasted space for a group of exact duplicates.
 * Wasted space = file_size * (group_size - 1)
 */
uintmax_t calculateWastedSpace(const std::vector<FileEntry>& group);

/**
 * Sorts groups of exact duplicates by the amount of space wasted, descending.
 */
void sortGroupsByWastedSpaceDescending(std::vector<std::vector<FileEntry>>& groups);

} // namespace dupcleaner::gui
