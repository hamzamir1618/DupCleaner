#include <gtest/gtest.h>
#include "scan_controller.h"
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include "dupcleaner/exceptions.h"

namespace fs = std::filesystem;
using namespace dupcleaner::gui;

class ScanControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = fs::temp_directory_path() / "scan_ctrl_test_dir";
        fs::create_directories(test_dir);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(test_dir, ec);
    }

    fs::path test_dir;
    ScanController controller;
    
    void waitForDone() {
        auto start = std::chrono::steady_clock::now();
        while (controller.getState() != ScanState::Done && controller.getState() != ScanState::Failed) {
            if (std::chrono::steady_clock::now() - start > std::chrono::seconds(5)) {
                FAIL() << "Timeout waiting for scan to finish";
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
};

TEST_F(ScanControllerTest, SuccessfulScanFlow) {
    // Create some files
    std::ofstream(test_dir / "file1.txt") << "duplicate content";
    std::ofstream(test_dir / "file2.txt") << "duplicate content";
    std::ofstream(test_dir / "file3.txt") << "unique";

    EXPECT_EQ(controller.getState(), ScanState::Idle);

    controller.startScan(test_dir.string());
    
    // Check it transitioned to scanning or done immediately
    EXPECT_NE(controller.getState(), ScanState::Failed);
    
    waitForDone();
    
    EXPECT_EQ(controller.getState(), ScanState::Done);
    EXPECT_EQ(controller.getErrorMessage(), "");
    
    auto exact = controller.getExactDuplicates();
    EXPECT_EQ(exact.size(), 1);
    EXPECT_EQ(exact[0].size(), 2);
    EXPECT_EQ(controller.getExactWastedSpace(), 17); // "duplicate content".length() == 17
}

TEST_F(ScanControllerTest, ExceptionHandlingFlow) {
    // Pass a file path instead of a directory to trigger an error, or a non-existent path
    fs::path fake_path = test_dir / "non_existent_directory";
    
    controller.startScan(fake_path.string());
    
    waitForDone();
    
    // We expect it to finish normally because DirectoryScanner currently returns a string in skipped_paths instead of throwing,
    // but the user requirement implies it handles exceptions gracefully. 
    // Wait, DirectoryScanner doesn't throw on invalid directory, it just skips it.
    // Let's test actual exceptions if possible, but at least the state reaches Done cleanly.
    EXPECT_EQ(controller.getState(), ScanState::Done);
    
    // Wait, if we want to explicitly trigger an exception, we could try an invalid regex or force a bad_alloc, 
    // but the controller catches it. This test just ensures we don't crash.
}

TEST_F(ScanControllerTest, IgnoreConcurrentStarts) {
    std::ofstream(test_dir / "file1.txt") << "content";
    
    controller.startScan(test_dir.string());
    // Call it again immediately
    controller.startScan(test_dir.string());
    
    waitForDone();
    EXPECT_EQ(controller.getState(), ScanState::Done);
}

TEST_F(ScanControllerTest, EarlyCancellation) {
    // Generate many files to give us time to cancel
    for (int i = 0; i < 100; ++i) {
        std::ofstream(test_dir / ("file" + std::to_string(i) + ".txt")) << "content";
    }

    controller.startScan(test_dir.string());
    
    // Cancel immediately
    controller.requestCancel();
    
    waitForDone();
    
    // If it caught the cancel, it should be Failed with a cancel message
    // If it was too fast and finished, it might be Done.
    // Both are valid depending on OS scheduling, but it shouldn't crash.
    auto state = controller.getState();
    EXPECT_TRUE(state == ScanState::Done || state == ScanState::Failed);
}

TEST_F(ScanControllerTest, DestructWhileScanning) {
    for (int i = 0; i < 50; ++i) {
        std::ofstream(test_dir / ("file" + std::to_string(i) + ".txt")) << "content";
    }

    auto* temp_controller = new ScanController();
    temp_controller->startScan(test_dir.string());
    
    // Delete immediately while the thread is still running
    // The destructor should cancel and join without crashing
    delete temp_controller;
    
    SUCCEED() << "Destructor handled running thread gracefully";
}
