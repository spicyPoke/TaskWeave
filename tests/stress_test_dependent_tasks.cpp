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
// Linear Chain Tests
// ============================================================================

/**
 * @brief Stress test: Linear chain of 1,000 dependent tasks
 *
 * Pattern: T0 -> T1 -> T2 -> ... -> T999
 * Each task adds 1 to the previous result.
 * Final result should be 1000.
 */
TEST(StressDependentTasks, LinearChain_1K)
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

    // All tasks should be complete
    for (size_t i = 0; i < kTaskCount; ++i) {
        EXPECT_EQ(tasks[i].get_state(), TaskState::Complete) << "Task " << i << " did not complete";
    }

    // Report
    auto duration = std::chrono::duration_cast<Duration>(end - start);
    TimingResult result{
        .total = duration,
        .min_task = DurationMicro{0},
        .max_task = DurationMicro{0},
        .avg_task_ms = duration.count() / static_cast<double>(kTaskCount),
        .tasks_per_second = kTaskCount * 1000.0 / duration.count()};
    print_timing_result("LinearChain_1K", result);
}

/**
 * @brief Stress test: Linear chain of 5,000 dependent tasks
 *
 * Deep dependency chain to test sequential execution under load.
 * Note: This test is inherently sequential due to dependencies.
 */
TEST(StressDependentTasks, LinearChain_5K)
{
    constexpr size_t kTaskCount = kTaskCount_Medium; // 5000

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
    print_timing_result("LinearChain_5K", result);
}

/**
 * @brief Stress test: Linear chain of void tasks
 *
 * Tests dependency chain without return values using Task<void, void>.
 *
 * Pattern: T0 -> T1 -> T2 -> ... -> Tn-1
 * Each task depends on the completion of the previous task.
 */
TEST(StressDependentTasks, DeepDependencyChain_Void_1K)
{
    constexpr size_t kTaskCount = kTaskCount_Light; // 1000

    // Setup
    std::vector<std::unique_ptr<Task<void, void>>> tasks = generate_void_linear_chain(kTaskCount);

    ThreadPoolExecutor executor;
    for (auto& task : tasks) {
        executor.add_task(task.get());
    }

    // Execute
    auto start = Clock::now();
    executor.run();
    executor.wait();
    auto end = Clock::now();

    // Validate
    EXPECT_TRUE(verify_void_linear_chain_results(tasks))
        << "Void linear chain results incorrect. Not all tasks completed.";

    // Report
    auto duration = std::chrono::duration_cast<Duration>(end - start);
    TimingResult result{
        .total = duration,
        .min_task = DurationMicro{0},
        .max_task = DurationMicro{0},
        .avg_task_ms = duration.count() / static_cast<double>(kTaskCount),
        .tasks_per_second = kTaskCount * 1000.0 / duration.count()};
    print_timing_result("DeepDependencyChain_Void_1K", result);
}

// ============================================================================
// Binary Tree Tests
// ============================================================================

/**
 * @brief Stress test: Binary tree dependency graph (10 levels, 1023 tasks)
 *
 * Pattern:
 *         T0
 *       /    \
 *      T1     T2
 *     /  \   /  \
 *    T3  T4 T5  T6
 *
 * Each task returns sum of children + 1.
 * Root should return 2^levels - 1 = 1023.
 */
TEST(StressDependentTasks, BinaryTree_1K)
{
    constexpr size_t kLevels = 10;
    const size_t kTaskCount = (1ULL << kLevels) - 1; // 1023

    // Setup
    std::vector<Task<int, int, int>> tasks = generate_binary_tree(kLevels);

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
    EXPECT_TRUE(verify_tree_results(tasks, kLevels))
        << "Binary tree results incorrect. Expected root value: " << ((1 << kLevels) - 1)
        << ", Got: " << tasks[0].get_result();

    // All tasks should be complete
    for (size_t i = 0; i < kTaskCount; ++i) {
        EXPECT_EQ(tasks[i].get_state(), TaskState::Complete) << "Task " << i << " did not complete";
    }

    // Report
    TimingResult result{
        .total = std::chrono::duration_cast<Duration>(end - start),
        .min_task = DurationMicro{0},
        .max_task = DurationMicro{0},
        .avg_task_ms = std::chrono::duration_cast<Duration>(end - start).count() / static_cast<double>(kTaskCount),
        .tasks_per_second = kTaskCount * 1000.0 / std::chrono::duration_cast<Duration>(end - start).count()};
    print_timing_result("BinaryTree_1K", result);
}

/**
 * @brief Stress test: Binary tree dependency graph (11 levels, 2047 tasks)
 *
 * Larger tree to test deeper parallel dependency resolution.
 */
TEST(StressDependentTasks, BinaryTree_2K)
{
    constexpr size_t kLevels = 11;
    const size_t kTaskCount = (1ULL << kLevels) - 1; // 2047

    // Setup
    std::vector<Task<int, int, int>> tasks = generate_binary_tree(kLevels);

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
    EXPECT_TRUE(verify_tree_results(tasks, kLevels))
        << "Binary tree results incorrect. Expected root value: " << ((1 << kLevels) - 1)
        << ", Got: " << tasks[0].get_result();

    // Report
    TimingResult result{
        .total = std::chrono::duration_cast<Duration>(end - start),
        .min_task = DurationMicro{0},
        .max_task = DurationMicro{0},
        .avg_task_ms = std::chrono::duration_cast<Duration>(end - start).count() / static_cast<double>(kTaskCount),
        .tasks_per_second = kTaskCount * 1000.0 / std::chrono::duration_cast<Duration>(end - start).count()};
    print_timing_result("BinaryTree_2K", result);
}

// ============================================================================
// Diamond Pattern Tests
// ============================================================================

/**
 * @brief Stress test: Diamond pattern repeated 500 times (2000 tasks)
 *
 * Pattern per diamond:
 *    T0
 *   /  \
 *  T1  T2
 *   \  /
 *    T3
 *
 * Tests multiple independent dependency graphs in parallel.
 */
TEST(StressDependentTasks, DiamondPattern_500)
{
    constexpr size_t kRepetitions = 500;
    const size_t kTaskCount = kRepetitions * 4; // 2000

    // Setup
    std::vector<Task<int, int, int>> tasks = generate_diamond_pattern(kRepetitions);

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
    EXPECT_TRUE(verify_diamond_results(tasks, kRepetitions)) << "Diamond pattern results incorrect";

    // All tasks should be complete
    for (size_t i = 0; i < kTaskCount; ++i) {
        EXPECT_EQ(tasks[i].get_state(), TaskState::Complete) << "Task " << i << " did not complete";
    }

    // Report
    TimingResult result{
        .total = std::chrono::duration_cast<Duration>(end - start),
        .min_task = DurationMicro{0},
        .max_task = DurationMicro{0},
        .avg_task_ms = std::chrono::duration_cast<Duration>(end - start).count() / static_cast<double>(kTaskCount),
        .tasks_per_second = kTaskCount * 1000.0 / std::chrono::duration_cast<Duration>(end - start).count()};
    print_timing_result("DiamondPattern_500", result);
}

// ============================================================================
// Fan-Out Tests
// ============================================================================

/**
 * @brief Stress test: Fan-out pattern (1 producer -> 1000 consumers)
 *
 * Pattern:
 *              T0
 *            / | \
 *           /  |  \
 *          T1 T2  T3 ... T1000
 *
 * Tests one-to-many dependency pattern.
 */
TEST(StressDependentTasks, FanOut_1K)
{
    constexpr size_t kConsumerCount = kTaskCount_Light; // 1000
    const size_t kTotalTasks = kConsumerCount + 1;

    // Setup
    auto [producer, consumers] = generate_fan_out(kConsumerCount);

    ThreadPoolExecutor executor;
    executor.add_task(producer.get());
    for (auto& consumer : consumers) {
        executor.add_task(&consumer);
    }

    // Execute
    auto start = Clock::now();
    executor.run();
    executor.wait();
    auto end = Clock::now();

    // Validate
    EXPECT_EQ(producer->get_state(), TaskState::Complete) << "Producer did not complete";
    EXPECT_EQ(producer->get_result(), 42) << "Producer returned wrong value";

    for (size_t i = 0; i < kConsumerCount; ++i) {
        EXPECT_EQ(consumers[i].get_state(), TaskState::Complete) << "Consumer " << i << " did not complete";
        EXPECT_EQ(consumers[i].get_result(), 42 + static_cast<int>(i)) << "Consumer " << i << " returned wrong value";
    }

    // Report
    TimingResult result{
        .total = std::chrono::duration_cast<Duration>(end - start),
        .min_task = DurationMicro{0},
        .max_task = DurationMicro{0},
        .avg_task_ms = std::chrono::duration_cast<Duration>(end - start).count() / static_cast<double>(kTotalTasks),
        .tasks_per_second = kTotalTasks * 1000.0 / std::chrono::duration_cast<Duration>(end - start).count()};
    print_timing_result("FanOut_1K", result);
}

// ============================================================================
// Fan-In Tests
// ============================================================================

/**
 * @brief Stress test: Fan-in pattern (10 producers -> 1 consumer)
 *
 * Pattern:
 *   T0  T1  T2  ...  T9
 *    \   \   \   /   /
 *              T10
 *
 * Tests many-to-one dependency pattern.
 */
TEST(StressDependentTasks, FanIn_10)
{
    constexpr size_t kProducerCount = 10;
    const size_t kTotalTasks = kProducerCount + 1;

    // Setup
    auto [producers, consumer] = generate_fan_in(kProducerCount);

    ThreadPoolExecutor executor;
    for (auto& producer : producers) {
        executor.add_task(&producer);
    }
    executor.add_task(consumer.get());

    // Execute
    auto start = Clock::now();
    executor.run();
    executor.wait();
    auto end = Clock::now();

    // Validate
    EXPECT_EQ(consumer->get_state(), TaskState::Complete) << "Consumer did not complete";

    for (size_t i = 0; i < kProducerCount; ++i) {
        EXPECT_EQ(producers[i].get_state(), TaskState::Complete) << "Producer " << i << " did not complete";
    }

    // Report
    TimingResult result{
        .total = std::chrono::duration_cast<Duration>(end - start),
        .min_task = DurationMicro{0},
        .max_task = DurationMicro{0},
        .avg_task_ms = std::chrono::duration_cast<Duration>(end - start).count() / static_cast<double>(kTotalTasks),
        .tasks_per_second = kTotalTasks * 1000.0 / std::chrono::duration_cast<Duration>(end - start).count()};
    print_timing_result("FanIn_10", result);
}

// ============================================================================
// Multi-Level DAG Tests
// ============================================================================

/**
 * @brief Stress test: Multi-level DAG with varying fan-out
 *
 * Creates a DAG with multiple levels where each task depends on
 * tasks from the previous level.
 *
 * Level 0: 1 task
 * Level 1: 10 tasks (each depends on level 0)
 * Level 2: 100 tasks (each depends on 1 task from level 1)
 * Level 3: 1000 tasks (each depends on 1 task from level 2)
 *
 * Total: ~1111 tasks
 */
TEST(StressDependentTasks, MultiLevelDAG_1K)
{
    constexpr size_t kLevel0Count = 1;
    constexpr size_t kLevel1Count = 10;
    constexpr size_t kLevel2Count = 100;
    constexpr size_t kLevel3Count = 900;
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
    print_timing_result("MultiLevelDAG_1K", result);
}

// ============================================================================
// Mixed Dependency Graph Tests
// ============================================================================

/**
 * @brief Stress test: Complex graph mixing independent and dependent tasks
 *
 * Creates a complex dependency graph with:
 * - 1000 independent tasks
 * - 500 linear chain tasks
 * - 500 diamond pattern tasks (125 diamonds)
 * - 2000 fan-out consumers (10 producers -> 200 consumers each)
 *
 * Total: ~5000 tasks with mixed dependency patterns
 */
TEST(StressDependentTasks, MixedDependencyGraph_5K)
{
    constexpr size_t kIndependentCount = 1000;
    constexpr size_t kChainCount = 500;
    constexpr size_t kDiamondCount = 125; // 125 * 4 = 500 tasks
    constexpr size_t kFanOutProducers = 10;
    constexpr size_t kFanOutConsumersPerProducer = 200;

    const size_t kDiamondTasks = kDiamondCount * 4;
    const size_t kFanOutConsumers = kFanOutProducers * kFanOutConsumersPerProducer;
    const size_t kTotalTasks = kIndependentCount + kChainCount + kDiamondTasks + kFanOutProducers + kFanOutConsumers;

    std::atomic<size_t> counter{0};

    // Independent tasks
    std::vector<Task<void>> independent = generate_independent_void_tasks(kIndependentCount, counter);

    // Linear chain
    std::vector<Task<int, int>> chain = generate_linear_chain(kChainCount);

    // Diamond patterns
    std::vector<Task<int, int, int>> diamonds = generate_diamond_pattern(kDiamondCount);

    // Fan-out patterns
    std::vector<std::pair<std::unique_ptr<Task<int>>, std::vector<Task<int, int>>>> fan_outs;
    fan_outs.reserve(kFanOutProducers);
    for (size_t i = 0; i < kFanOutProducers; ++i) {
        fan_outs.push_back(generate_fan_out(kFanOutConsumersPerProducer));
    }

    ThreadPoolExecutor executor;

    // Add all tasks
    for (auto& task : independent) {
        executor.add_task(&task);
    }
    for (auto& task : chain) {
        executor.add_task(&task);
    }
    for (auto& task : diamonds) {
        executor.add_task(&task);
    }
    for (auto& [producer, consumers] : fan_outs) {
        executor.add_task(producer.get());
        for (auto& consumer : consumers) {
            executor.add_task(&consumer);
        }
    }

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
    print_timing_result("MixedDependencyGraph_5K", result);
}

} // namespace tw::stress
