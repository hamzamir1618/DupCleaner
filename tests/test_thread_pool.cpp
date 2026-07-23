#include <gtest/gtest.h>
#include "thread_pool.h"
#include <atomic>
#include <vector>

using namespace dupcleaner;

TEST(ThreadPoolTest, ExecuteTasks) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;

    for (int i = 0; i < 100; ++i) {
        futures.push_back(pool.enqueue([&counter] {
            counter++;
        }));
    }

    for (auto& f : futures) {
        f.wait();
    }

    EXPECT_EQ(counter.load(), 100);
}

TEST(ThreadPoolTest, ReturnValues) {
    ThreadPool pool(2);
    std::vector<std::future<int>> futures;

    for (int i = 0; i < 50; ++i) {
        futures.push_back(pool.enqueue([i] {
            return i * 2;
        }));
    }

    int sum = 0;
    for (int i = 0; i < 50; ++i) {
        sum += futures[i].get();
    }

    // Sum of i*2 for i in 0..49 = 2 * (49 * 50) / 2 = 2450
    EXPECT_EQ(sum, 2450);
}

TEST(ThreadPoolTest, StressTest) {
    ThreadPool pool; // uses hardware_concurrency
    std::atomic<int> count{0};
    std::vector<std::future<void>> futures;

    constexpr int num_tasks = 10000;

    for (int i = 0; i < num_tasks; ++i) {
        futures.push_back(pool.enqueue([&count] {
            count.fetch_add(1, std::memory_order_relaxed);
        }));
    }

    for (auto& f : futures) {
        f.get();
    }

    EXPECT_EQ(count.load(), num_tasks);
}
