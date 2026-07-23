#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include "deleter.h"

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
    auto res = deleter.execute(plan, false); // moveToTrash = false

    EXPECT_TRUE(res.success);
    EXPECT_TRUE(res.failed_files.empty());
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
    auto res = deleter.execute(plan, true);
    
    EXPECT_TRUE(res.success);
    EXPECT_TRUE(res.failed_files.empty());
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

TEST_F(DeleterTest, ReadOnlyFilesReportedPerFileInsteadOfAborting) {
    auto p1 = create_file("f1.txt", "content");
    
    fs::path sub_dir = temp_dir / "sub";
    fs::create_directory(sub_dir);
    auto p2 = sub_dir / "f2.txt";
    std::ofstream ofs(p2);
    ofs << "content";
    ofs.close();

    auto p3 = create_file("f3.txt", "content");

    // Make sub_dir read-only so p2 cannot be deleted on POSIX
    fs::permissions(sub_dir, fs::perms::owner_read | fs::perms::owner_exec | fs::perms::group_read | fs::perms::group_exec | fs::perms::others_read | fs::perms::others_exec, fs::perm_options::replace);

    DeletionPlan plan;
    plan.delete_files.push_back(p1);
    plan.delete_files.push_back(p2);
    plan.delete_files.push_back(p3);

    SafeDeleter deleter(temp_dir);
    auto res = deleter.execute(plan, false); // Permanent deletion

    // Restore permissions immediately so TearDown can clean it up
    fs::permissions(sub_dir, fs::perms::owner_all, fs::perm_options::add);

    EXPECT_TRUE(res.success); // The plan didn't fundamentally abort
    ASSERT_EQ(res.failed_files.size(), 1);
    EXPECT_EQ(res.failed_files[0], p2);

    // p1 and p3 should be deleted, p2 should remain
    EXPECT_FALSE(fs::exists(p1));
    EXPECT_TRUE(fs::exists(p2));
    EXPECT_FALSE(fs::exists(p3));
}
