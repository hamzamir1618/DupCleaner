#pragma once

#include <string>
#include <vector>
#include <atomic>
#include "dupcleaner/file_entry.h"
#include "duplicate_finder.h"
#include "thumbnail_cache.h"
#include "deleter.h"
#include "scan_controller.h"
#include "table_logic.h"
#include <set>

namespace dupcleaner::gui {

class DupCleanerApp {
public:
    DupCleanerApp();
    ~DupCleanerApp();

    void render();

private:
    void startScan(const std::string& path);
    void renderExactDuplicates();
    void renderNearDuplicates();

    // UI State
    char target_dir[2048] = "";

    ScanController scan_controller;

    // Results from ScanController (fetched when Done)
    std::vector<std::vector<FileEntry>> exact_duplicates;
    DuplicateFinder::NearDuplicateResult near_duplicates;
    uintmax_t exact_wasted_space = 0;
    
    // Deletion State
    std::set<std::string> selected_for_deletion;
    bool show_confirm_modal = false;
    
    ThumbnailCache thumbnail_cache;
    
    // Core Deleter
    std::unique_ptr<SafeDeleter> deleter;
};

} // namespace dupcleaner::gui
