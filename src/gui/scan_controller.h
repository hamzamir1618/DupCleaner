#pragma once

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <stdexcept>
#include "dupcleaner/file_entry.h"
#include "duplicate_finder.h"
#include "scanner.h"

namespace dupcleaner::gui {

enum class ScanState {
    Idle,
    Scanning,
    Done,
    Failed
};

class ScanController {
public:
    ScanController();
    ~ScanController();

    // Prevent copying and moving to ensure thread safety
    ScanController(const ScanController&) = delete;
    ScanController& operator=(const ScanController&) = delete;

    void startScan(const std::string& path, const ScannerOptions& options = {}, bool enable_near_duplicates = false, int similarity_threshold = 10);
    void requestCancel();

    // Thread-safe accessors
    ScanState getState() const;
    std::string getErrorMessage() const;

    // Returns a copy of the exact duplicates to ensure thread-safety for the UI
    std::vector<std::vector<FileEntry>> getExactDuplicates() const;
    
    // Returns a copy of the near duplicates to ensure thread-safety for the UI
    DuplicateFinder::NearDuplicateResult getNearDuplicates() const;

    uintmax_t getExactWastedSpace() const;

    uintmax_t getFilesScanned() const;
    uintmax_t getBytesScanned() const;

    PipelinePhase getCurrentPhase() const;
    uintmax_t getPhaseProgress() const;
    uintmax_t getPhaseTotal() const;

private:
    // Core scanning method run in the background thread
    void runScan(std::string path, ScannerOptions options);

    std::thread worker;
    std::atomic<ScanState> current_state{ScanState::Idle};
    std::atomic<bool> cancel_flag{false};
    std::atomic<bool> enable_near_duplicates{false};
    std::atomic<int> similarity_threshold{10};
    
    std::atomic<uintmax_t> current_files_scanned{0};
    std::atomic<uintmax_t> current_bytes_scanned{0};
    
    std::atomic<PipelinePhase> current_phase{PipelinePhase::Scanning};
    std::atomic<uintmax_t> current_phase_progress{0};
    std::atomic<uintmax_t> current_phase_total{0};

    mutable std::mutex results_mutex;
    std::string error_message;
    std::vector<std::vector<FileEntry>> exact_duplicates;
    DuplicateFinder::NearDuplicateResult near_duplicates;
    uintmax_t exact_wasted_space{0};
};

} // namespace dupcleaner::gui
