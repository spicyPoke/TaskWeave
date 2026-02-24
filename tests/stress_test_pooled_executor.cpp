// Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "Executor/ThreadPoolExecutor.h"
#include "TaskWeave/Task.h"
#include "stress_test_utils.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <gtest/gtest.h>
#include <iostream>
#include <thread>
#include <vector>

namespace tw::stress {

// ============================================================================
// Basic Executor Stress Tests
// ============================================================================

/**
 * @brief Stress test: Executor with 1,000 tasks
 *
 * Basic stress test for ThreadPoolExecutor.
 */
TEST(StressPooledExecutor, Executor_1K_Tasks)
{
    constexpr size_t kTaskCount = kTaskCount_Light; // 1000
    std::atomic<size_t> counter{0};

    // Setup
    std::vector<Task<void>> tasks = generate_independent_void_tasks(kTaskCount, counter);

    ThreadPoolExecutor executor;
    for (auto& task : tasks) {
        executor.add_task(&task);
    }

    // Execute
    auto start = Clock::now();
    executor.run();
    executor.wait();
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
    print_timing_result("Executor_1K_Tasks", result);
}

/**
 * @brief Stress test: Executor with 5,000 tasks
 *
 * Medium stress test for ThreadPoolExecutor.
 */
TEST(StressPooledExecutor, Executor_5K_Tasks)
{
    constexpr size_t kTaskCount = kTaskCount_Medium; // 5000
    std::atomic<size_t> counter{0};

    // Setup
    std::vector<Task<void>> tasks = generate_independent_void_tasks(kTaskCount, counter);

    ThreadPoolExecutor executor;
    for (auto& task : tasks) {
        executor.add_task(&task);
    }

    // Execute
    auto start = Clock::now();
    executor.run();
    executor.wait();
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
    print_timing_result("Executor_5K_Tasks", result);
}

/**
 * @brief Stress test: Executor with 10,000 tasks
 *
 * Heavy stress test for ThreadPoolExecutor.
 */
TEST(StressPooledExecutor, Executor_10K_Tasks)
{
    constexpr size_t kTaskCount = kTaskCount_Heavy; // 10000
    std::atomic<size_t> counter{0};

    // Setup
    std::vector<Task<void>> tasks = generate_independent_void_tasks(kTaskCount, counter);

    ThreadPoolExecutor executor;
    for (auto& task : tasks) {
        executor.add_task(&task);
    }

    // Execute
    auto start = Clock::now();
    executor.run();
    executor.wait();
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
    print_timing_result("Executor_10K_Tasks", result);
}

// ============================================================================
// Executor with Mixed Dependencies
// ============================================================================

/**
 * @brief Stress test: Executor with 5,000 tasks with mixed dependencies
 *
 * Tests executor's ability to handle complex dependency patterns.
 */
TEST(StressPooledExecutor, Executor_MixedDependencies_5K)
{
    constexpr size_t kIndependentCount = 2000;
    constexpr size_t kChainCount = 1000;
    constexpr size_t kDiamondCount = 500; // 500 * 4 = 2000 tasks

    const size_t kDiamondTasks = kDiamondCount * 4;
    const size_t kTotalTasks = kIndependentCount + kChainCount + kDiamondTasks;

    std::atomic<size_t> counter{0};

    // Independent tasks
    std::vector<Task<void>> independent = generate_independent_void_tasks(kIndependentCount, counter);

    // Linear chain
    std::vector<Task<int, int>> chain = generate_linear_chain(kChainCount);

    // Diamond patterns
    std::vector<Task<int, int, int>> diamonds = generate_diamond_pattern(kDiamondCount);

    ThreadPoolExecutor executor;
    for (auto& task : independent)
        executor.add_task(&task);
    for (auto& task : chain)
        executor.add_task(&task);
    for (auto& task : diamonds)
        executor.add_task(&task);

    // Execute
    auto start = Clock::now();
    executor.run();
    executor.wait();
    auto end = Clock::now();

    // Validate
    EXPECT_EQ(counter.load(), kIndependentCount) << "Not all independent tasks executed";

    EXPECT_TRUE(verify_linear_chain_results(chain, static_cast<int>(kChainCount))) << "Chain results incorrect";

    EXPECT_TRUE(verify_diamond_results(diamonds, kDiamondCount)) << "Diamond results incorrect";

    // Report
    TimingResult result{
        .total = std::chrono::duration_cast<Duration>(end - start),
        .min_task = DurationMicro{0},
        .max_task = DurationMicro{0},
        .avg_task_ms = std::chrono::duration_cast<Duration>(end - start).count() / static_cast<double>(kTotalTasks),
        .tasks_per_second = kTotalTasks * 1000.0 / std::chrono::duration_cast<Duration>(end - start).count()};
    print_timing_result("Executor_MixedDependencies_5K", result);
}

// ============================================================================
// Executor Cancellation Tests
// ============================================================================

/**
 * @brief Stress test: Cancel executor under heavy load
 *
 * Tests that cancellation works correctly when many tasks are queued.
 */
TEST(StressPooledExecutor, Executor_CancelUnderLoad)
{
    constexpr size_t kTaskCount = kTaskCount_Heavy; // 10000
    std::atomic<size_t> counter{0};
    std::atomic<bool> should_stop{false};

    // Setup tasks that take some time
    std::vector<Task<void>> tasks(kTaskCount);
    for (auto& task : tasks) {
        task.set_callable([&counter, &should_stop]() {
            if (!should_stop.load(std::memory_order_relaxed)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                counter.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    ThreadPoolExecutor executor;
    for (auto& task : tasks) {
        executor.add_task(&task);
    }

    // Execute and cancel after a short delay
    executor.run();

    // Let some tasks start executing
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Cancel remaining queued tasks
    executor.cancel();
    executor.wait();

    // Validate: Some tasks should have executed, but not all
    const size_t executed = counter.load();
    EXPECT_LT(executed, kTaskCount) << "All tasks executed before cancel could take effect. This may indicate "
                                    << "a problem with the cancel functionality.";
    EXPECT_GT(executed, 0) << "No tasks executed at all. This may indicate a problem with task scheduling.";

    std::cout << "  Tasks executed before cancel: " << executed << " / " << kTaskCount << "\n";
    std::cout << "  Tasks cancelled: " << (kTaskCount - executed) << "\n";
}

// ============================================================================
// Executor Thread Scaling Tests
// ============================================================================

/**
 * @brief Stress test: Executor with varying thread counts
 *
 * Tests performance with different thread pool sizes.
 *
 * Note: This test uses the ThreadPool constructor directly to control
 * thread count, which requires accessing ThreadPool from ThreadPoolExecutor.
 * Since we can't modify ThreadPoolExecutor, we test with the default
 * thread pool size and report hardware concurrency.
 */
TEST(StressPooledExecutor, Executor_VaryingThreadCounts)
{
    constexpr size_t kTaskCount = kTaskCount_Medium; // 5000
    std::atomic<size_t> counter{0};

    // Setup
    std::vector<Task<void>> tasks = generate_independent_void_tasks(kTaskCount, counter);

    ThreadPoolExecutor executor;
    for (auto& task : tasks) {
        executor.add_task(&task);
    }

    // Execute
    auto start = Clock::now();
    executor.run();
    executor.wait();
    auto end = Clock::now();

    // Validate
    EXPECT_EQ(counter.load(), kTaskCount)
        << "Not all tasks executed. Expected: " << kTaskCount << ", Got: " << counter.load();

    // Report
    const size_t thread_count = std::thread::hardware_concurrency();
    auto duration = std::chrono::duration_cast<Duration>(end - start);
    TimingResult result{
        .total = duration,
        .min_task = DurationMicro{0},
        .max_task = DurationMicro{0},
        .avg_task_ms = duration.count() / static_cast<double>(kTaskCount),
        .tasks_per_second = kTaskCount * 1000.0 / duration.count()};
    std::cout << "  Hardware threads:  " << thread_count << "\n";
    print_timing_result("Executor_VaryingThreadCounts", result);
}

// ============================================================================
// Executor Repeated Use Tests
// ============================================================================

/**
 * @brief Stress test: Repeated executor use
 *
 * Tests that executor can be recreated and reused multiple times
 * without resource leaks or state corruption.
 */
TEST(StressPooledExecutor, Executor_RepeatedUse)
{
    constexpr size_t kBatchCount = 100;
    constexpr size_t kTasksPerBatch = 100;
    constexpr size_t kTotalCount = kBatchCount * kTasksPerBatch;

    std::atomic<size_t> counter{0};

    auto start = Clock::now();

    for (size_t batch = 0; batch < kBatchCount; ++batch) {
        std::vector<Task<void>> tasks = generate_independent_void_tasks(kTasksPerBatch, counter);

        ThreadPoolExecutor executor;
        for (auto& task : tasks) {
            executor.add_task(&task);
        }

        executor.run();
        executor.wait();
    }

    auto end = Clock::now();

    // Validate
    EXPECT_EQ(counter.load(), kTotalCount)
        << "Not all tasks executed. Expected: " << kTotalCount << ", Got: " << counter.load();

    // Report
    auto duration = std::chrono::duration_cast<Duration>(end - start);
    TimingResult result{
        .total = duration,
        .min_task = DurationMicro{0},
        .max_task = DurationMicro{0},
        .avg_task_ms = duration.count() / static_cast<double>(kTotalCount),
        .tasks_per_second = kTotalCount * 1000.0 / duration.count()};
    print_timing_result("Executor_RepeatedUse", result);
}

// ============================================================================
// Executor Chained Tasks Tests
// ============================================================================

/**
 * @brief Stress test: Executor with chained tasks
 *
 * Tests executor handling of a long chain of dependent tasks.
 */
TEST(StressPooledExecutor, Executor_ChainedTasks_1K)
{
    constexpr size_t kTaskCount = kTaskCount_Light; // 1000

    // Setup
    std::vector<Task<int, int>> tasks = generate_linear_chain(kTaskCount);

    ThreadPoolExecutor executor;
    for (auto& task : tasks) {
        executor.add_task(&task);
    }

    // Execute
    auto start = Clock::now();
    executor.run();
    executor.wait();
    auto end = Clock::now();

    // Validate
    EXPECT_TRUE(verify_linear_chain_results(tasks, static_cast<int>(kTaskCount)))
        << "Linear chain results incorrect. Expected final value: " << kTaskCount
        << ", Got: " << tasks.back().get_result();

    // Report
    auto duration = std::chrono::duration_cast<Duration>(end - start);
    TimingResult result{
        .total = duration,
        .min_task = DurationMicro{0},
        .max_task = DurationMicro{0},
        .avg_task_ms = duration.count() / static_cast<double>(kTaskCount),
        .tasks_per_second = kTaskCount * 1000.0 / duration.count()};
    print_timing_result("Executor_ChainedTasks_1K", result);
}

// ============================================================================
// Executor Complex DAG Tests
// ============================================================================

/**
 * @brief Stress test: Executor with complex DAG (2000 tasks)
 *
 * Tests executor with a complex multi-level dependency graph.
 */
TEST(StressPooledExecutor, Executor_ComplexDAG_2K)
{
    constexpr size_t kLevel0Count = 10;
    constexpr size_t kLevel1Count = 50;
    constexpr size_t kLevel2Count = 200;
    constexpr size_t kLevel3Count = 1740;
    const size_t kTotalTasks = kLevel0Count + kLevel1Count + kLevel2Count + kLevel3Count;

    std::vector<Task<int>> level0(kLevel0Count);
    std::vector<Task<int, int>> level1(kLevel1Count);
    std::vector<Task<int, int>> level2(kLevel2Count);
    std::vector<Task<int, int>> level3(kLevel3Count);

    // Level 0: Root tasks (no dependencies)
    for (size_t i = 0; i < kLevel0Count; ++i) {
        level0[i].set_callable([]() {
            return 1;
        });
    }

    // Level 1: Each depends on one level 0 task
    for (size_t i = 0; i < kLevel1Count; ++i) {
        level1[i].set_callable([](int val) {
            return val + 1;
        });
        level1[i].add_inward_edge<int>(level0[i % kLevel0Count].get_outward_edge());
    }

    // Level 2: Each depends on one level 1 task
    for (size_t i = 0; i < kLevel2Count; ++i) {
        level2[i].set_callable([](int val) {
            return val + 1;
        });
        level2[i].add_inward_edge<int>(level1[i % kLevel1Count].get_outward_edge());
    }

    // Level 3: Each depends on one level 2 task
    for (size_t i = 0; i < kLevel3Count; ++i) {
        level3[i].set_callable([](int val) {
            return val + 1;
        });
        level3[i].add_inward_edge<int>(level2[i % kLevel2Count].get_outward_edge());
    }

    ThreadPoolExecutor executor;
    for (auto& task : level0) {
        executor.add_task(&task);
    }
    for (auto& task : level1) {
        executor.add_task(&task);
    }
    for (auto& task : level2) {
        executor.add_task(&task);
    }
    for (auto& task : level3) {
        executor.add_task(&task);
    }

    // Execute
    auto start = Clock::now();
    executor.run();
    executor.wait();
    auto end = Clock::now();

    // Validate - all tasks should be complete
    size_t incomplete_count = 0;
    for (const auto& task : level0) {
        if (task.get_state() != TaskState::Complete) {
            incomplete_count++;
        }
    }
    for (const auto& task : level1) {
        if (task.get_state() != TaskState::Complete) {
            incomplete_count++;
        }
    }
    for (const auto& task : level2) {
        if (task.get_state() != TaskState::Complete) {
            incomplete_count++;
        }
    }
    for (const auto& task : level3) {
        if (task.get_state() != TaskState::Complete) {
            incomplete_count++;
        }
    }

    EXPECT_EQ(incomplete_count, 0) << incomplete_count << " tasks did not complete";

    // Report
    TimingResult result{
        .total = std::chrono::duration_cast<Duration>(end - start),
        .min_task = DurationMicro{0},
        .max_task = DurationMicro{0},
        .avg_task_ms = std::chrono::duration_cast<Duration>(end - start).count() / static_cast<double>(kTotalTasks),
        .tasks_per_second = kTotalTasks * 1000.0 / std::chrono::duration_cast<Duration>(end - start).count()};
    print_timing_result("Executor_ComplexDAG_2K", result);
}

} // namespace tw::stress
