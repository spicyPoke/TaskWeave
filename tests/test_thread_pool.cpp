// Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "Executor/ThreadPool.h"

#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

namespace tw::test {

// Test ThreadPool construction
TEST(ThreadPoolTest, Construction)
{
    ThreadPool pool(4);

    EXPECT_EQ(pool.worker_count(), 4);
    EXPECT_TRUE(pool.empty());
    EXPECT_EQ(pool.size(), 0);
}

// Test ThreadPool with default hardware concurrency
TEST(ThreadPoolTest, DefaultConstruction)
{
    ThreadPool pool(std::thread::hardware_concurrency());

    EXPECT_GT(pool.worker_count(), 0);
}

// Test ThreadPool add_task
TEST(ThreadPoolTest, AddTask)
{
    ThreadPool pool(2);

    std::atomic<int> counter{0};

    for (int i = 0; i < 10; i++) {
        bool result = pool.add_task([&counter]() {
            counter.fetch_add(1, std::memory_order_acq_rel);
        });

        EXPECT_TRUE(result);
    }

    pool.run();
    pool.wait();

    EXPECT_EQ(counter.load(std::memory_order_acquire), 10);
    EXPECT_TRUE(pool.empty());
}

// Test ThreadPool add_task with null function
TEST(ThreadPoolTest, AddTaskNullFunction)
{
    ThreadPool pool(2);

    // Test with nullptr function pointer
    void (*null_func_ptr)() = nullptr;
    EXPECT_FALSE(pool.add_task(null_func_ptr));

    // Test with empty std::function
    std::function<void()> null_std_func = nullptr;
    EXPECT_FALSE(pool.add_task(null_std_func));

    // Verify pool is still empty after rejected tasks
    EXPECT_TRUE(pool.empty());
    EXPECT_EQ(pool.size(), 0);
}

// Test ThreadPool add_task with arguments
TEST(ThreadPoolTest, AddTaskWithArguments)
{
    ThreadPool pool(2);

    std::atomic<int> result{0};

    pool.add_task(
        [&result](int a, int b) {
            result.fetch_add(a + b, std::memory_order_acq_rel);
        },
        5, 3);

    pool.run();
    pool.wait();

    EXPECT_EQ(result.load(std::memory_order_acquire), 8);
}

// Test ThreadPool multiple tasks
TEST(ThreadPoolTest, MultipleTasks)
{
    ThreadPool pool(4);

    std::atomic<int> counter{0};

    // Add tasks - they may start executing immediately
    for (int i = 0; i < 20; i++) {
        pool.add_task([&counter]() {
            counter.fetch_add(1, std::memory_order_acq_rel);
        });
    }

    pool.run();
    pool.wait();

    EXPECT_EQ(counter.load(std::memory_order_acquire), 20);
    EXPECT_TRUE(pool.empty());
}

// Test ThreadPool active_task_count
TEST(ThreadPoolTest, ActiveTaskCount)
{
    ThreadPool pool(4);

    pool.add_task([]() {
    });

    EXPECT_EQ(pool.active_task_count(), 1);

    pool.run();
    pool.wait();

    EXPECT_EQ(pool.active_task_count(), 0);
}

// Test ThreadPool clear_queued_tasks
TEST(ThreadPoolTest, ClearQueuedTasks)
{
    ThreadPool pool(2);

    std::atomic<int> counter{0};

    // Add tasks that take some time
    for (int i = 0; i < 10; i++) {
        pool.add_task([&counter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            counter.fetch_add(1, std::memory_order_acq_rel);
        });
    }

    // Clear before they all run
    pool.clear_queued_tasks();

    // Wait for any running tasks to complete
    pool.run();
    pool.wait();

    // Counter should be less than 10 since we cleared some tasks
    EXPECT_LT(counter.load(std::memory_order_acquire), 10);
}

// Test ThreadPool is_idle
TEST(ThreadPoolTest, IsIdle)
{
    ThreadPool pool(2);

    EXPECT_TRUE(pool.is_idle());
    pool.add_task([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    });

    EXPECT_FALSE(pool.is_idle());

    pool.run();
    pool.wait();

    EXPECT_TRUE(pool.is_idle());
}

// Test ThreadPool get_queued_tasks
TEST(ThreadPoolTest, GetQueuedTasks)
{
    ThreadPool pool(1); // Use single worker to control execution

    std::atomic<bool> started{false};
    std::atomic<bool> done{false};

    // Add a blocking task first
    pool.add_task([&started, &done]() {
        started.store(true, std::memory_order_release);
        while (!done.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    });

    // Start the worker threads
    pool.run();

    // Wait for the blocking task to start
    while (!started.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Now add more tasks - they should be queued
    for (int i = 0; i < 5; i++) {
        pool.add_task([i]() {
            // Do nothing
        });
    }

    auto queued = pool.get_queued_tasks();
    EXPECT_EQ(queued.size(), 5);

    done.store(true, std::memory_order_release);
    pool.wait();
}

// Test ThreadPool shutdown
TEST(ThreadPoolTest, Shutdown)
{
    std::atomic<int> counter{0};

    {
        ThreadPool pool(2);

        for (int i = 0; i < 10; i++) {
            pool.add_task([&counter]() {
                counter.fetch_add(1, std::memory_order_acq_rel);
            });
        }

        pool.run();
        pool.wait();
    }
    // Pool destructor should join all threads

    EXPECT_EQ(counter.load(std::memory_order_acquire), 10);
}

// Test ThreadPool concurrent access
TEST(ThreadPoolTest, ConcurrentAccess)
{
    ThreadPool pool(4);

    std::atomic<int> counter{0};

    // Add tasks from multiple threads
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; i++) {
        threads.emplace_back([&pool, &counter]() {
            for (int j = 0; j < 10; j++) {
                pool.add_task([&counter]() {
                    counter.fetch_add(1, std::memory_order_acq_rel);
                });
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    pool.run();
    pool.wait();

    EXPECT_EQ(counter.load(std::memory_order_acquire), 40);
}

// Test ThreadPool task execution order (not guaranteed, but should complete)
TEST(ThreadPoolTest, TaskCompletion)
{
    ThreadPool pool(2);

    std::vector<int> results;
    std::mutex mtx;

    for (int i = 0; i < 10; i++) {
        pool.add_task([&results, &mtx, i]() {
            std::lock_guard<std::mutex> lock(mtx);
            results.push_back(i);
        });
    }

    pool.run();
    pool.wait();

    EXPECT_EQ(results.size(), 10);

    // All values 0-9 should be present (order may vary)
    std::sort(results.begin(), results.end());
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(results[i], i);
    }
}

// Test ThreadPool on_complete callback is called only once
TEST(ThreadPoolTest, OnCompleteCallbackCalledOnce)
{
    std::atomic<int> callback_count{0};

    {
        ThreadPool pool(5, [&callback_count]() {
            callback_count.fetch_add(1, std::memory_order_acq_rel);
        });

        // Add multiple tasks
        for (int i = 0; i < 10; i++) {
            pool.add_task([i]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(10 - i));
            });
        }

        pool.run();
        pool.wait();
    }

    // Callback should have been called exactly once when pool became idle
    EXPECT_EQ(callback_count.load(std::memory_order_acquire), 1);
}

} // namespace tw::test
