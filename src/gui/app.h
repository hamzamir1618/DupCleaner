#pragma once

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include "dupcleaner/file_entry.h"
#include "duplicate_finder.h"
#include "thumbnail_cache.h"

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
    
    // Threading and State
    std::thread scan_thread;
    std::atomic<bool> is_scanning{false};
    std::atomic<bool> scan_finished_flag{false};
    std::mutex results_mutex;

    // Results
    std::vector<std::vector<FileEntry>> exact_duplicates;
    DuplicateFinder::NearDuplicateResult near_duplicates;
    uintmax_t exact_wasted_space = 0;
    
    ThumbnailCache thumbnail_cache;
};

} // namespace dupcleaner::gui
