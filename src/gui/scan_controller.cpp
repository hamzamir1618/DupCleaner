#include "scan_controller.h"
#include "selection_logic.h"
#include "formatting.h"
#include <exception>

namespace dupcleaner::gui {

ScanController::ScanController() {}

ScanController::~ScanController() {
    requestCancel();
    if (worker.joinable()) {
        worker.join();
    }
}

void ScanController::startScan(const std::string& path, const ScannerOptions& options) {
    ScanState expected = ScanState::Idle;
    // Allow restarting a scan if it's already Done or Failed
    if (current_state.load() == ScanState::Scanning) {
        return; // Ignore concurrent request
    }
    
    // Join the previous worker if it exists
    if (worker.joinable()) {
        worker.join();
    }

    current_state.store(ScanState::Scanning);
    cancel_flag.store(false);
    current_files_scanned.store(0);
    current_bytes_scanned.store(0);

    {
        std::lock_guard<std::mutex> lock(results_mutex);
        error_message.clear();
        exact_duplicates.clear();
        near_duplicates.groups.clear();
        near_duplicates.skipped_paths.clear();
        exact_wasted_space = 0;
    }

    worker = std::thread(&ScanController::runScan, this, path, options);
}

void ScanController::requestCancel() {
    cancel_flag.store(true, std::memory_order_relaxed);
}

ScanState ScanController::getState() const {
    return current_state.load();
}

std::string ScanController::getErrorMessage() const {
    std::lock_guard<std::mutex> lock(results_mutex);
    return error_message;
}

std::vector<std::vector<FileEntry>> ScanController::getExactDuplicates() const {
    std::lock_guard<std::mutex> lock(results_mutex);
    return exact_duplicates;
}

DuplicateFinder::NearDuplicateResult ScanController::getNearDuplicates() const {
    std::lock_guard<std::mutex> lock(results_mutex);
    return near_duplicates;
}

uintmax_t ScanController::getExactWastedSpace() const {
    std::lock_guard<std::mutex> lock(results_mutex);
    return exact_wasted_space;
}

uintmax_t ScanController::getFilesScanned() const {
    return current_files_scanned.load(std::memory_order_relaxed);
}

uintmax_t ScanController::getBytesScanned() const {
    return current_bytes_scanned.load(std::memory_order_relaxed);
}

void ScanController::runScan(std::string path, ScannerOptions options) {
    try {
        DirectoryScanner scanner;
        DirectoryScanner::ProgressTracker progress;
        progress.files_visited = &current_files_scanned;
        progress.bytes_visited = &current_bytes_scanned;

        auto result = scanner.scan(path, options, &cancel_flag, progress);
        
        if (cancel_flag.load(std::memory_order_relaxed)) {
            // Cancelled
            std::lock_guard<std::mutex> lock(results_mutex);
            error_message = "Scan cancelled by user.";
            current_state.store(ScanState::Failed);
            return;
        }

        auto exact = DuplicateFinder::findExactDuplicates(result.entries);
        
        if (cancel_flag.load(std::memory_order_relaxed)) {
            std::lock_guard<std::mutex> lock(results_mutex);
            error_message = "Scan cancelled by user.";
            current_state.store(ScanState::Failed);
            return;
        }
        
        auto near_dup = DuplicateFinder::findNearDuplicateImages(result.entries, 10);
        
        if (cancel_flag.load(std::memory_order_relaxed)) {
            std::lock_guard<std::mutex> lock(results_mutex);
            error_message = "Scan cancelled by user.";
            current_state.store(ScanState::Failed);
            return;
        }

        sortGroupsByWastedSpaceDescending(exact);
        
        uintmax_t total_wasted = 0;
        for (const auto& group : exact) {
            total_wasted += calculateWastedSpace(group);
        }

        std::lock_guard<std::mutex> lock(results_mutex);
        exact_duplicates = std::move(exact);
        near_duplicates = std::move(near_dup);
        exact_wasted_space = total_wasted;
        
        current_state.store(ScanState::Done);
        
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(results_mutex);
        error_message = e.what();
        current_state.store(ScanState::Failed);
    } catch (...) {
        std::lock_guard<std::mutex> lock(results_mutex);
        error_message = "Unknown exception during scan.";
        current_state.store(ScanState::Failed);
    }
}

} // namespace dupcleaner::gui
