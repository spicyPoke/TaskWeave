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
// Independent Void Tasks
// ============================================================================

/**
 * @brief Stress test: 1,000 independent void tasks
 *
 * Validates basic parallel execution capability with a moderate task count.
 */
TEST(StressIndependentTasks, IndependentTasks_1K)
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
    print_timing_result("IndependentTasks_1K", result);
}

/**
 * @brief Stress test: 5,000 independent void tasks
 *
 * Medium stress test to validate scaling.
 */
TEST(StressIndependentTasks, IndependentTasks_5K)
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
    print_timing_result("IndependentTasks_5K", result);
}

/**
 * @brief Stress test: 10,000 independent void tasks
 *
 * Heavy stress test to validate framework under significant load.
 */
TEST(StressIndependentTasks, IndependentTasks_10K)
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
    print_timing_result("IndependentTasks_10K", result);
}

// ============================================================================
// Independent Tasks with Return Values
// ============================================================================

/**
 * @brief Stress test: 1,000 independent tasks with int return values
 *
 * Validates that return values are correctly propagated through the system.
 */
TEST(StressIndependentTasks, IndependentTasksWithReturn_1K)
{
    constexpr size_t kTaskCount = kTaskCount_Light; // 1000
    std::atomic<size_t> counter{0};

    // Setup
    std::vector<Task<int>> tasks = generate_independent_int_tasks(kTaskCount, counter);

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

    // Validate return values
    bool all_correct = true;
    for (size_t i = 0; i < kTaskCount; ++i) {
        if (tasks[i].get_result() != static_cast<int>(i)) {
            all_correct = false;
            break;
        }
    }
    EXPECT_TRUE(all_correct) << "Some tasks returned incorrect values";

    // Report
    auto duration = std::chrono::duration_cast<Duration>(end - start);
    TimingResult result{
        .total = duration,
        .min_task = DurationMicro{0},
        .max_task = DurationMicro{0},
        .avg_task_ms = duration.count() / static_cast<double>(kTaskCount),
        .tasks_per_second = kTaskCount * 1000.0 / duration.count()};
    print_timing_result("IndependentTasksWithReturn_1K", result);
}

/**
 * @brief Stress test: 5,000 independent tasks with int return values
 *
 * Medium stress test with return value validation.
 */
TEST(StressIndependentTasks, IndependentTasksWithReturn_5K)
{
    constexpr size_t kTaskCount = kTaskCount_Medium; // 5000
    std::atomic<size_t> counter{0};

    // Setup
    std::vector<Task<int>> tasks = generate_independent_int_tasks(kTaskCount, counter);

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

    // Validate return values
    bool all_correct = true;
    for (size_t i = 0; i < kTaskCount; ++i) {
        if (tasks[i].get_result() != static_cast<int>(i)) {
            all_correct = false;
            break;
        }
    }
    EXPECT_TRUE(all_correct) << "Some tasks returned incorrect values";

    // Report
    auto duration = std::chrono::duration_cast<Duration>(end - start);
    TimingResult result{
        .total = duration,
        .min_task = DurationMicro{0},
        .max_task = DurationMicro{0},
        .avg_task_ms = duration.count() / static_cast<double>(kTaskCount),
        .tasks_per_second = kTaskCount * 1000.0 / duration.count()};
    print_timing_result("IndependentTasksWithReturn_5K", result);
}

// ============================================================================
// Mixed Workload Tests
// ============================================================================

/**
 * @brief Stress test: Mixed workload of void and return-value tasks
 *
 * Tests the framework's ability to handle heterogeneous task types.
 */
TEST(StressIndependentTasks, IndependentTasks_MixedWorkload)
{
    constexpr size_t kVoidTaskCount = 2500;
    constexpr size_t kIntTaskCount = 2500;
    constexpr size_t kTotalCount = kVoidTaskCount + kIntTaskCount;

    std::atomic<size_t> counter{0};

    // Setup void tasks
    std::vector<Task<void>> void_tasks = generate_independent_void_tasks(kVoidTaskCount, counter);

    // Setup int tasks
    std::vector<Task<int>> int_tasks = generate_independent_int_tasks(kIntTaskCount, counter);

    ThreadPoolExecutor executor;
    for (auto& task : void_tasks) {
        executor.add_task(&task);
    }
    for (auto& task : int_tasks) {
        executor.add_task(&task);
    }

    // Execute
    auto start = Clock::now();
    executor.run();
    executor.wait();
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
    print_timing_result("IndependentTasks_MixedWorkload", result);
}

// ============================================================================
// Thread Pool Saturation Tests
// ============================================================================

/**
 * @brief Stress test: Tasks exceeding thread pool capacity
 *
 * Validates that the framework handles task queuing correctly when
 * there are far more tasks than available threads.
 */
TEST(StressIndependentTasks, IndependentTasks_ThreadPoolSaturation)
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
    const size_t thread_count = std::thread::hardware_concurrency();
    TimingResult result{
        .total = duration,
        .min_task = DurationMicro{0},
        .max_task = DurationMicro{0},
        .avg_task_ms = duration.count() / static_cast<double>(kTaskCount),
        .tasks_per_second = kTaskCount * 1000.0 / duration.count()};
    std::cout << "  Hardware threads:  " << thread_count << "\n";
    print_timing_result("IndependentTasks_ThreadPoolSaturation", result);
}

// ============================================================================
// Repeated Execution Tests
// ============================================================================

/**
 * @brief Stress test: Repeated execution of task batches
 *
 * Validates that the executor can be reused for multiple batches
 * without resource leaks or state corruption.
 */
TEST(StressIndependentTasks, IndependentTasks_RepeatedExecution)
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
    print_timing_result("IndependentTasks_RepeatedExecution", result);
}

// ============================================================================
// Long-Running Task Tests
// ============================================================================

/**
 * @brief Stress test: Tasks with simulated work (1ms sleep each)
 *
 * Validates that the framework handles tasks that take measurable time.
 */
TEST(StressIndependentTasks, IndependentTasks_WithWork_100)
{
    constexpr size_t kTaskCount = 100;
    constexpr size_t kWorkDurationMs = 1;
    std::atomic<size_t> counter{0};

    // Setup
    std::vector<Task<void>> tasks = generate_timed_tasks(kTaskCount, kWorkDurationMs, counter);

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
        .min_task = DurationMicro{kWorkDurationMs * 1000},
        .max_task = DurationMicro{kWorkDurationMs * 1000},
        .avg_task_ms = duration.count() / static_cast<double>(kTaskCount),
        .tasks_per_second = kTaskCount * 1000.0 / duration.count()};
    print_timing_result("IndependentTasks_WithWork_100", result);

    // Expected minimum time is kTaskCount * kWorkDurationMs / thread_count
    // This is a rough sanity check
    const size_t thread_count = std::thread::hardware_concurrency();
    const size_t expected_min_ms = (kTaskCount * kWorkDurationMs) / thread_count;
    EXPECT_GE(duration.count(), expected_min_ms * 0.8) << "Execution was faster than expected. Expected at least "
                                                       << expected_min_ms << "ms with " << thread_count << " threads";
}

} // namespace tw::stress
