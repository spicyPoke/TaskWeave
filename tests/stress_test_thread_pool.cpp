// Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "Executor/ThreadPool.h"
#include "stress_test_utils.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <gtest/gtest.h>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

namespace tw::stress {

// ============================================================================
// Basic ThreadPool Stress Tests
// ============================================================================

/**
 * @brief Stress test: ThreadPool with 10,000 simple tasks
 *
 * Basic stress test for direct ThreadPool usage.
 */
TEST(StressThreadPool, Pool_10K_Tasks)
{
    constexpr size_t kTaskCount = kTaskCount_Heavy; // 10000
    std::atomic<size_t> counter{0};

    // Setup
    ThreadPool pool(std::thread::hardware_concurrency());

    for (size_t i = 0; i < kTaskCount; ++i) {
        pool.add_task([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    // Execute
    auto start = Clock::now();
    pool.run();
    pool.wait();
    auto end = Clock::now();

    // Validate
    EXPECT_EQ(counter.load(), kTaskCount)
        << "Not all tasks executed. Expected: " << kTaskCount << ", Got: " << counter.load();

    // Report
    auto duration = std::chrono::duration_cast<Duration>(end - start);
    TimingResult result{
        .total = duration,
        .min_task = DurationMicro{0},
        .max_task = DurationMicro{0},
        .avg_task_ms = duration.count() / static_cast<double>(kTaskCount),
        .tasks_per_second = kTaskCount * 1000.0 / duration.count()};
    print_timing_result("Pool_10K_Tasks", result);
}

/**
 * @brief Stress test: ThreadPool with 50,000 simple tasks
 *
 * Heavy stress test for ThreadPool.
 */
TEST(StressThreadPool, Pool_50K_Tasks)
{
    constexpr size_t kTaskCount = kTaskCount_Extreme; // 50000
    std::atomic<size_t> counter{0};

    // Setup
    ThreadPool pool(std::thread::hardware_concurrency());

    for (size_t i = 0; i < kTaskCount; ++i) {
        pool.add_task([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    // Execute
    auto start = Clock::now();
    pool.run();
    pool.wait();
    auto end = Clock::now();

    // Validate
    EXPECT_EQ(counter.load(), kTaskCount)
        << "Not all tasks executed. Expected: " << kTaskCount << ", Got: " << counter.load();

    // Report
    auto duration = std::chrono::duration_cast<Duration>(end - start);
    TimingResult result{
        .total = duration,
        .min_task = DurationMicro{0},
        .max_task = DurationMicro{0},
        .avg_task_ms = duration.count() / static_cast<double>(kTaskCount),
        .tasks_per_second = kTaskCount * 1000.0 / duration.count()};
    print_timing_result("Pool_50K_Tasks", result);
}

/**
 * @brief Stress test: ThreadPool with 100,000 simple tasks
 *
 * Extreme stress test for ThreadPool.
 */
TEST(StressThreadPool, Pool_100K_Tasks)
{
    constexpr size_t kTaskCount = kTaskCount_Ultra; // 100000
    std::atomic<size_t> counter{0};

    // Setup
    ThreadPool pool(std::thread::hardware_concurrency());

    for (size_t i = 0; i < kTaskCount; ++i) {
        pool.add_task([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    // Execute
    auto start = Clock::now();
    pool.run();
    pool.wait();
    auto end = Clock::now();

    // Validate
    EXPECT_EQ(counter.load(), kTaskCount)
        << "Not all tasks executed. Expected: " << kTaskCount << ", Got: " << counter.load();

    // Report
    auto duration = std::chrono::duration_cast<Duration>(end - start);
    TimingResult result{
        .total = duration,
        .min_task = DurationMicro{0},
        .max_task = DurationMicro{0},
        .avg_task_ms = duration.count() / static_cast<double>(kTaskCount),
        .tasks_per_second = kTaskCount * 1000.0 / duration.count()};
    print_timing_result("Pool_100K_Tasks", result);
}

// ============================================================================
// Long-Running Task Tests
// ============================================================================

/**
 * @brief Stress test: ThreadPool with 1,000 long-running tasks (10ms each)
 *
 * Tests ThreadPool with tasks that have measurable execution time.
 */
TEST(StressThreadPool, Pool_LongRunning_1K)
{
    constexpr size_t kTaskCount = kTaskCount_Light; // 1000
    constexpr size_t kWorkDurationMs = 10;
    std::atomic<size_t> counter{0};

    // Setup
    ThreadPool pool(std::thread::hardware_concurrency());

    for (size_t i = 0; i < kTaskCount; ++i) {
        pool.add_task([&counter, kWorkDurationMs]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(kWorkDurationMs));
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    // Execute
    auto start = Clock::now();
    pool.run();
    pool.wait();
    auto end = Clock::now();

    // Validate
    EXPECT_EQ(counter.load(), kTaskCount)
        << "Not all tasks executed. Expected: " << kTaskCount << ", Got: " << counter.load();

    // Report
    auto duration = std::chrono::duration_cast<Duration>(end - start);
    const size_t thread_count = std::thread::hardware_concurrency();
    TimingResult result{
        .total = duration,
        .min_task = DurationMicro{kWorkDurationMs * 1000},
        .max_task = DurationMicro{kWorkDurationMs * 1000},
        .avg_task_ms = duration.count() / static_cast<double>(kTaskCount),
        .tasks_per_second = kTaskCount * 1000.0 / duration.count()};
    std::cout << "  Hardware threads:  " << thread_count << "\n";
    std::cout << "  Expected min time: " << (kTaskCount * kWorkDurationMs / thread_count) << " ms\n";
    print_timing_result("Pool_LongRunning_1K", result);
}

/**
 * @brief Stress test: ThreadPool with varying task durations
 *
 * Tests ThreadPool with a mix of fast and slow tasks.
 */
TEST(StressThreadPool, Pool_VaryingTaskDuration_500)
{
    constexpr size_t kTaskCount = 500;
    constexpr size_t kFastDurationMs = 1;
    constexpr size_t kSlowDurationMs = 100;
    std::atomic<size_t> counter{0};

    // Setup
    ThreadPool pool(std::thread::hardware_concurrency());

    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::uniform_int_distribution<size_t> dist(0, 1);

    for (size_t i = 0; i < kTaskCount; ++i) {
        const size_t duration = dist(rng) ? kSlowDurationMs : kFastDurationMs;
        pool.add_task([&counter, duration]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(duration));
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    // Execute
    auto start = Clock::now();
    pool.run();
    pool.wait();
    auto end = Clock::now();

    // Validate
    EXPECT_EQ(counter.load(), kTaskCount)
        << "Not all tasks executed. Expected: " << kTaskCount << ", Got: " << counter.load();

    // Report
    auto duration = std::chrono::duration_cast<Duration>(end - start);
    TimingResult result{
        .total = duration,
        .min_task = DurationMicro{kFastDurationMs * 1000},
        .max_task = DurationMicro{kSlowDurationMs * 1000},
        .avg_task_ms = duration.count() / static_cast<double>(kTaskCount),
        .tasks_per_second = kTaskCount * 1000.0 / duration.count()};
    print_timing_result("Pool_VaryingTaskDuration_500", result);
}

// ============================================================================
// Thread Scaling Tests
// ============================================================================

/**
 * @brief Stress test: ThreadPool with different thread counts
 *
 * Compares performance across 1, 2, 4, 8, 16 threads.
 */
TEST(StressThreadPool, Pool_ThreadScaling_10K)
{
    constexpr size_t kTaskCount = kTaskCount_Heavy; // 10000
    constexpr size_t kWorkDurationMs = 1;

    std::vector<size_t> thread_counts = {1, 2, 4, 8, 16};
    std::vector<Duration> durations;
    durations.reserve(thread_counts.size());

    for (const size_t thread_count : thread_counts) {
        std::atomic<size_t> counter{0};

        // Setup
        ThreadPool pool(thread_count);

        for (size_t i = 0; i < kTaskCount; ++i) {
            pool.add_task([&counter, kWorkDurationMs]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(kWorkDurationMs));
                counter.fetch_add(1, std::memory_order_relaxed);
            });
        }

        // Execute
        auto start = Clock::now();
        pool.run();
        pool.wait();
        auto end = Clock::now();

        // Validate
        EXPECT_EQ(counter.load(), kTaskCount) << "Not all tasks executed with " << thread_count << " threads";

        durations.push_back(std::chrono::duration_cast<Duration>(end - start));
    }

    // Report
    std::cout << "\n  Thread Scaling Results:\n";
    std::cout << "  " << std::string(60, '-') << "\n";
    std::cout << "  Threads | Total Time (ms) | Tasks/sec\n";
    std::cout << "  " << std::string(60, '-') << "\n";

    for (size_t i = 0; i < thread_counts.size(); ++i) {
        const double tasks_per_sec = kTaskCount * 1000.0 / durations[i].count();
        std::cout << "  " << std::setw(7) << thread_counts[i] << " | " << std::setw(15) << durations[i].count() << " | "
                  << std::setw(10) << static_cast<int>(tasks_per_sec) << "\n";
    }
    std::cout << "  " << std::string(60, '-') << "\n";

    // Verify that more threads generally means faster execution
    // (with some tolerance for scheduling overhead)
    EXPECT_LT(durations[4].count(), durations[0].count() * 0.3)
        << "16 threads should be significantly faster than 1 thread";
}

// ============================================================================
// Clear Under Load Tests
// ============================================================================

/**
 * @brief Stress test: ThreadPool clear queued tasks while executing
 *
 * Tests that clear_queued_tasks() works correctly under load.
 */
TEST(StressThreadPool, Pool_ClearUnderLoad_5K)
{
    constexpr size_t kTaskCount = kTaskCount_Medium; // 5000
    constexpr size_t kWorkDurationMs = 5;
    std::atomic<size_t> counter{0};

    // Setup
    ThreadPool pool(std::thread::hardware_concurrency());

    for (size_t i = 0; i < kTaskCount; ++i) {
        pool.add_task([&counter, kWorkDurationMs]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(kWorkDurationMs));
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    // Start worker threads
    pool.run();

    // Let some tasks execute
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Clear remaining queued tasks
    pool.clear_queued_tasks();

    // Wait for remaining executing tasks to finish
    pool.wait();

    // Validate: Some tasks should have executed, but not all
    const size_t executed = counter.load();
    EXPECT_LT(executed, kTaskCount) << "All tasks executed before clear could take effect";
    EXPECT_GT(executed, 0) << "No tasks executed at all";

    std::cout << "  Tasks executed before clear: " << executed << " / " << kTaskCount << "\n";
    std::cout << "  Tasks cleared: " << (kTaskCount - executed) << "\n";
}

// ============================================================================
// Concurrent Submission Tests
// ============================================================================

/**
 * @brief Stress test: ThreadPool with concurrent task submission
 *
 * Tests thread safety of task submission from multiple threads.
 */
TEST(StressThreadPool, Pool_ConcurrentSubmission_10K)
{
    constexpr size_t kTaskCount = kTaskCount_Heavy; // 10000
    constexpr size_t kSubmitterThreads = 4;
    constexpr size_t kTasksPerThread = kTaskCount / kSubmitterThreads;
    std::atomic<size_t> counter{0};

    // Setup
    ThreadPool pool(std::thread::hardware_concurrency());

    // Create submitter threads
    std::vector<std::thread> submitters;
    submitters.reserve(kSubmitterThreads);

    for (size_t i = 0; i < kSubmitterThreads; ++i) {
        submitters.emplace_back([&pool, &counter, kTasksPerThread]() {
            for (size_t j = 0; j < kTasksPerThread; ++j) {
                pool.add_task([&counter]() {
                    counter.fetch_add(1, std::memory_order_relaxed);
                });
            }
        });
    }

    // Wait for all submitters to finish
    for (auto& submitter : submitters) {
        submitter.join();
    }

    // Execute
    auto start = Clock::now();
    pool.run();
    pool.wait();
    auto end = Clock::now();

    // Validate
    EXPECT_EQ(counter.load(), kTaskCount)
        << "Not all tasks executed. Expected: " << kTaskCount << ", Got: " << counter.load();

    // Report
    auto duration = std::chrono::duration_cast<Duration>(end - start);
    TimingResult result{
        .total = duration,
        .min_task = DurationMicro{0},
        .max_task = DurationMicro{0},
        .avg_task_ms = duration.count() / static_cast<double>(kTaskCount),
        .tasks_per_second = kTaskCount * 1000.0 / duration.count()};
    std::cout << "  Submitter threads: " << kSubmitterThreads << "\n";
    print_timing_result("Pool_ConcurrentSubmission_10K", result);
}

// ============================================================================
// Memory and Resource Tests
// ============================================================================

/**
 * @brief Stress test: ThreadPool with many small tasks
 *
 * Tests memory handling with a large number of small tasks.
 */
TEST(StressThreadPool, Pool_ManySmallTasks_50K)
{
    constexpr size_t kTaskCount = kTaskCount_Extreme; // 50000
    std::atomic<size_t> counter{0};

    // Setup
    ThreadPool pool(std::thread::hardware_concurrency());

    for (size_t i = 0; i < kTaskCount; ++i) {
        pool.add_task([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    // Execute
    auto start = Clock::now();
    pool.run();
    pool.wait();
    auto end = Clock::now();

    // Validate
    EXPECT_EQ(counter.load(), kTaskCount)
        << "Not all tasks executed. Expected: " << kTaskCount << ", Got: " << counter.load();

    // Report
    auto duration = std::chrono::duration_cast<Duration>(end - start);
    TimingResult result{
        .total = duration,
        .min_task = DurationMicro{0},
        .max_task = DurationMicro{0},
        .avg_task_ms = duration.count() / static_cast<double>(kTaskCount),
        .tasks_per_second = kTaskCount * 1000.0 / duration.count()};
    print_timing_result("Pool_ManySmallTasks_50K", result);
}

// ============================================================================
// Stress Test with Completion Callback
// ============================================================================

/**
 * @brief Stress test: ThreadPool with completion callback
 *
 * Tests that the on_complete callback is called correctly.
 */
TEST(StressThreadPool, Pool_WithCompletionCallback)
{
    constexpr size_t kTaskCount = kTaskCount_Light; // 1000
    std::atomic<size_t> counter{0};
    std::atomic<int> callback_called{0};

    // Setup
    ThreadPool pool(std::thread::hardware_concurrency(), [&callback_called]() {
        callback_called.fetch_add(1, std::memory_order_acq_rel);
    });

    std::atomic<bool> block{true};
    for (size_t i = 0; i < kTaskCount; ++i) {
        pool.add_task([&counter, &block]() {
            while (block.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }
    block.store(false, std::memory_order_release);
    // Execute
    pool.run();
    pool.wait();

    // Validate
    EXPECT_EQ(counter.load(), kTaskCount) << "Not all tasks executed";
    EXPECT_EQ(callback_called.load(), 1) << "Completion callback was not called once";
}

} // namespace tw::stress
