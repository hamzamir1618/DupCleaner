#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include "dupcleaner/deleter.h"

namespace fs = std::filesystem;
using namespace dupcleaner;

class DeleterTest : public ::testing::Test {
protected:
    fs::path temp_dir;
    fs::path trash_dir;

    void SetUp() override {
        auto now = std::chrono::system_clock::now().time_since_epoch().count();
        temp_dir = fs::temp_directory_path() / ("dupcleaner_deleter_test_" + std::to_string(now));
        trash_dir = temp_dir / ".dupcleaner_trash";
        fs::create_directories(temp_dir);
    }

    void TearDown() override {
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
    }

    fs::path create_file(const std::string& name, const std::string& content, int time_offset_seconds = 0) {
        fs::path p = temp_dir / name;
        std::ofstream ofs(p);
        ofs << content;
        ofs.close();

        if (time_offset_seconds != 0) {
            auto new_time = fs::file_time_type::clock::now() + std::chrono::seconds(time_offset_seconds);
            fs::last_write_time(p, new_time);
        }
        return p;
    }

    FileEntry make_entry(const fs::path& p) {
        FileEntry e;
        e.path = p;
        e.size = fs::file_size(p);
        e.last_modified = fs::last_write_time(p);
        return e;
    }
};

TEST_F(DeleterTest, PlanDeletionKeepOldest) {
    auto p1 = create_file("f1.txt", "content", 10); // Newer
    auto p2 = create_file("f2.txt", "content", -10); // Oldest
    auto p3 = create_file("f3.txt", "content", 0); // Middle

    std::vector<std::vector<FileEntry>> groups = {
        {make_entry(p1), make_entry(p2), make_entry(p3)}
    };

    SafeDeleter deleter(temp_dir);
    auto plan = deleter.planDeletion(groups, KeepStrategy::KeepOldest);

    ASSERT_EQ(plan.keep_files.size(), 1);
    ASSERT_EQ(plan.delete_files.size(), 2);
    EXPECT_EQ(plan.keep_files[0], p2); // f2 is oldest
}

TEST_F(DeleterTest, PlanDeletionKeepNewest) {
    auto p1 = create_file("f1.txt", "content", -10); // Oldest
    auto p2 = create_file("f2.txt", "content", 10); // Newest

    std::vector<std::vector<FileEntry>> groups = {
        {make_entry(p1), make_entry(p2)}
    };

    SafeDeleter deleter(temp_dir);
    auto plan = deleter.planDeletion(groups, KeepStrategy::KeepNewest);

    ASSERT_EQ(plan.keep_files.size(), 1);
    ASSERT_EQ(plan.delete_files.size(), 1);
    EXPECT_EQ(plan.keep_files[0], p2); // f2 is newest
}

TEST_F(DeleterTest, PlanDeletionAlwaysKeepsOne) {
    auto p1 = create_file("f1.txt", "content", 0); 
    auto p2 = create_file("f2.txt", "content", 0); 

    std::vector<std::vector<FileEntry>> groups = {
        {make_entry(p1), make_entry(p2)}
    };

    SafeDeleter deleter(temp_dir);
    auto plan = deleter.planDeletion(groups, KeepStrategy::KeepFirstAlphabetically);

    ASSERT_EQ(plan.keep_files.size(), 1);
    ASSERT_EQ(plan.delete_files.size(), 1);
    EXPECT_EQ(plan.keep_files[0], p1); // f1 comes first alphabetically
}

TEST_F(DeleterTest, ExecutePermanentDeletion) {
    auto p1 = create_file("f1.txt", "content");
    auto p2 = create_file("f2.txt", "content");

    DeletionPlan plan;
    plan.delete_files.push_back(p1);
    plan.delete_files.push_back(p2);

    SafeDeleter deleter(temp_dir);
    bool ok = deleter.execute(plan, false); // moveToTrash = false

    EXPECT_TRUE(ok);
    EXPECT_FALSE(fs::exists(p1));
    EXPECT_FALSE(fs::exists(p2));
    EXPECT_FALSE(fs::exists(trash_dir));
}

TEST_F(DeleterTest, ExecuteMoveToTrashAndUndo) {
    auto p1 = create_file("f1.txt", "content1");
    auto p2 = create_file("f2.txt", "content2");

    DeletionPlan plan;
    plan.delete_files.push_back(p1);
    plan.delete_files.push_back(p2);

    SafeDeleter deleter(temp_dir);
    
    // Execute moveToTrash
    bool ok = deleter.execute(plan, true);
    
    EXPECT_TRUE(ok);
    EXPECT_FALSE(fs::exists(p1));
    EXPECT_FALSE(fs::exists(p2));
    EXPECT_TRUE(fs::exists(trash_dir));

    // Verify manifest exists
    bool manifest_found = false;
    for (const auto& entry : fs::directory_iterator(trash_dir)) {
        if (entry.path().extension() == ".json") {
            manifest_found = true;
        }
    }
    EXPECT_TRUE(manifest_found);

    // Undo
    bool undo_ok = deleter.undoLastDeletion();
    EXPECT_TRUE(undo_ok);

    EXPECT_TRUE(fs::exists(p1));
    EXPECT_TRUE(fs::exists(p2));

    // Verify content restored
    std::ifstream in1(p1);
    std::string c1((std::istreambuf_iterator<char>(in1)), std::istreambuf_iterator<char>());
    EXPECT_EQ(c1, "content1");
}
