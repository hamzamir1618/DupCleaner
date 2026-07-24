#include "thread_pool.h"

namespace dupcleaner {

ThreadPool::ThreadPool(size_t threads)
    : stop(false)
{
    // Ensure at least 1 thread
    if (threads == 0) threads = 1;

    for (size_t i = 0; i < threads; ++i) {
        workers.emplace_back([this] {
            for (;;) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait(lock, [this] {
                        return this->stop || !this->tasks.empty();
                    });

                    if (this->stop && this->tasks.empty()) {
                        return;
                    }

                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }

                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread &worker : workers) {
        worker.join();
    }
}

static size_t g_pool_size = std::max<size_t>(1, std::min<size_t>(std::thread::hardware_concurrency(), 4));
static std::unique_ptr<ThreadPool> g_pool;
static std::once_flag g_pool_once;

void initGlobalThreadPool(size_t threads) {
    if (threads > 0) g_pool_size = threads;
}

ThreadPool& getGlobalThreadPool() {
    std::call_once(g_pool_once, []() {
        g_pool = std::make_unique<ThreadPool>(g_pool_size);
    });
    return *g_pool;
}

} // namespace dupcleaner
