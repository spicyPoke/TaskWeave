// Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "Executor/ThreadPoolExecutor.h"
#include "TaskWeave/Task.h"

// STL
#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <numeric>
#include <utility>
#include <vector>

namespace tw::stress {

// ============================================================================
// Timing Utilities
// ============================================================================

using Clock = std::chrono::high_resolution_clock;
using TimePoint = Clock::time_point;
using Duration = std::chrono::milliseconds;
using DurationMicro = std::chrono::microseconds;

/// @brief Result of timing measurement for a stress test
struct TimingResult {
    Duration total;          ///< Total execution time
    DurationMicro min_task;  ///< Minimum task duration
    DurationMicro max_task;  ///< Maximum task duration
    double avg_task_ms;      ///< Average task duration in milliseconds
    double tasks_per_second; ///< Throughput (tasks/second)
};

/// @brief Statistics for a set of measurements
template<typename T>
struct Statistics {
    T count;       ///< Number of samples
    T min;         ///< Minimum value
    T max;         ///< Maximum value
    double mean;   ///< Arithmetic mean
    double stddev; ///< Standard deviation
};

// ============================================================================
// Task Generators - Independent Tasks
// ============================================================================

/**
 * @brief Generate independent void tasks that increment a counter
 * @param count Number of tasks to generate
 * @param counter Atomic counter to increment
 * @return Vector of tasks
 */
inline std::vector<Task<void>> generate_independent_void_tasks(size_t count, std::atomic<size_t>& counter)
{
    std::vector<Task<void>> tasks(count);
    for (auto& task : tasks) {
        task.set_callable([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }
    return tasks;
}

/**
 * @brief Generate independent int-returning tasks
 * @param count Number of tasks to generate
 * @param counter Atomic counter to track execution
 * @return Vector of tasks returning their index
 */
inline std::vector<Task<int>> generate_independent_int_tasks(size_t count, std::atomic<size_t>& counter)
{
    std::vector<Task<int>> tasks(count);
    for (size_t i = 0; i < count; ++i) {
        tasks[i].set_callable([i, &counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
            return static_cast<int>(i);
        });
    }
    return tasks;
}

/**
 * @brief Generate independent tasks with varying work duration
 * @param count Number of tasks to generate
 * @param base_duration_ms Base duration in milliseconds
 * @param counter Atomic counter to track execution
 * @return Vector of tasks
 */
inline std::vector<Task<void>> generate_timed_tasks(size_t count, size_t base_duration_ms, std::atomic<size_t>& counter)
{
    std::vector<Task<void>> tasks(count);
    for (size_t i = 0; i < count; ++i) {
        tasks[i].set_callable([i, base_duration_ms, &counter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(base_duration_ms));
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }
    return tasks;
}

// ============================================================================
// Task Generators - Dependency Graphs
// ============================================================================

/**
 * @brief Generate a linear chain of dependent tasks
 * @param count Number of tasks in the chain
 * @return Vector of tasks where task[i] depends on task[i-1]
 *
 * Pattern: T0 -> T1 -> T2 -> ... -> Tn-1
 * Each task adds 1 to the previous result.
 *
 * Note: Uses Task<int, int> for all tasks. First task ignores its input.
 */
inline std::vector<Task<int, int>> generate_linear_chain(size_t count)
{
    if (count == 0) {
        return {};
    }

    std::vector<Task<int, int>> tasks(count);

    // First task - ignores input, returns 1
    tasks[0].set_callable([](int) {
        return 1;
    });

    // Subsequent tasks depend on the previous one
    for (size_t i = 1; i < count; ++i) {
        tasks[i].set_callable([](int prev) {
            return prev + 1;
        });
        tasks[i].add_inward_edge<0>(tasks[i - 1].get_outward_edge());
    }

    return tasks;
}

/**
 * @brief Generate a linear chain of dependent void tasks
 * @param count Number of tasks in the chain
 * @return Vector of tasks where task[i] depends on task[i-1]
 *
 * Pattern: T0 -> T1 -> T2 -> ... -> Tn-1
 * Each task increments a counter when executed.
 *
 * Note: Uses Task<void, void> for all tasks. First task has no dependency set.
 */
inline std::vector<std::unique_ptr<Task<void, void>>> generate_void_linear_chain(size_t count)
{
    if (count == 0) {
        return {};
    }

    std::vector<std::unique_ptr<Task<void, void>>> tasks(count);

    // All tasks use Task<void, void>
    for (size_t i = 0; i < count; ++i) {
        tasks[i] = std::make_unique<Task<void, void>>();
        tasks[i]->set_callable([]() {
            // Task body (empty for void tasks)
        });

        // Set dependency for all tasks except the first one
        if (i > 0) {
            tasks[i]->add_inward_edge<0>(tasks[i - 1]->get_outward_edge());
        }
    }

    return tasks;
}

/**
 * @brief Generate a binary tree dependency graph
 * @param levels Number of levels in the tree
 * @return Vector of tasks forming a binary tree
 *
 * Pattern:
 *         T0
 *       /    \
 *      T1     T2
 *     /  \   /  \
 *    T3  T4 T5  T6
 *
 * Each task returns the sum of its children + 1
 * Note: All tasks use Task<int, int, int> (return int, 2 int inputs).
 * Leaf nodes ignore their inputs.
 */
inline std::vector<Task<int, int, int>> generate_binary_tree(size_t levels)
{
    if (levels == 0) {
        return {};
    }

    const size_t count = (1ULL << levels) - 1; // 2^levels - 1
    std::vector<Task<int, int, int>> tasks(count);

    // Initialize leaf nodes (last level) - ignore inputs, return 1
    const size_t leaf_start = (1ULL << (levels - 1)) - 1;
    for (size_t i = leaf_start; i < count; ++i) {
        tasks[i].set_callable([](int, int) {
            return 1;
        });
    }

    // Initialize internal nodes (bottom-up)
    for (size_t i = leaf_start; i-- > 0;) {
        const size_t left_child = 2 * i + 1;
        const size_t right_child = 2 * i + 2;

        tasks[i].set_callable([](int left, int right) {
            return left + right + 1;
        });
        tasks[i].add_inward_edge<0>(tasks[left_child].get_outward_edge());
        tasks[i].add_inward_edge<1>(tasks[right_child].get_outward_edge());
    }

    return tasks;
}

/**
 * @brief Generate diamond pattern dependencies
 * @param repetitions Number of diamond patterns to create
 * @return Vector of tasks forming diamond patterns
 *
 * Pattern (single diamond):
 *    T0
 *   /  \
 *  T1  T2
 *   \  /
 *    T3
 *
 * T3 waits for both T1 and T2, which both wait for T0
 * Note: All tasks use Task<int, int, int> (return int, 2 int inputs).
 * T0, T1, T2 ignore unused inputs.
 */
inline std::vector<Task<int, int, int>> generate_diamond_pattern(size_t repetitions)
{
    if (repetitions == 0) {
        return {};
    }

    const size_t count = repetitions * 4; // 4 tasks per diamond
    std::vector<Task<int, int, int>> tasks(count);

    for (size_t i = 0; i < repetitions; ++i) {
        const size_t base = i * 4;

        // Top of diamond - ignores inputs
        tasks[base].set_callable([](int, int) {
            return 1;
        });

        // Left side - uses first input, ignores second
        tasks[base + 1].set_callable([](int top, int) {
            return top + 1;
        });
        tasks[base + 1].add_inward_edge<0>(tasks[base].get_outward_edge());

        // Right side - uses first input, ignores second
        tasks[base + 2].set_callable([](int top, int) {
            return top + 2;
        });
        tasks[base + 2].add_inward_edge<0>(tasks[base].get_outward_edge());

        // Bottom of diamond - depends on both sides
        tasks[base + 3].set_callable([](int left, int right) {
            return left + right;
        });
        tasks[base + 3].add_inward_edge<0>(tasks[base + 1].get_outward_edge());
        tasks[base + 3].add_inward_edge<1>(tasks[base + 2].get_outward_edge());
    }

    return tasks;
}

/**
 * @brief Generate fan-in pattern (many producers, one consumer)
 * @param producer_count Number of producer tasks
 * @return Pair of (producers, unique_ptr to consumer)
 *
 * Pattern:
 *   T0  T1  T2  ...  Tn-1
 *    \   \   \   /   /
 *              Tn
 *
 * Note: Consumer uses Task<int, int, int, int, int, int, int, int, int, int, int>
 * which has return type int and 10 int inputs. Limited to 10 producers max.
 *
 * Returns unique_ptr for consumer because Task is non-movable due to mutex members.
 */
inline auto generate_fan_in(size_t producer_count)
    -> std::pair<std::vector<Task<int>>, std::unique_ptr<Task<int, int, int, int, int, int, int, int, int, int, int>>>
{
    using ConsumerTask = Task<int, int, int, int, int, int, int, int, int, int, int>;
    using result_type = std::pair<std::vector<Task<int>>, std::unique_ptr<ConsumerTask>>;

    if (producer_count > 10) {
        return result_type(std::vector<Task<int>>(), nullptr);
    }

    std::vector<Task<int>> producers(producer_count);
    for (size_t i = 0; i < producer_count; ++i) {
        producers[i].set_callable([i]() {
            return static_cast<int>(i + 1);
        });
    }

    // Consumer with fixed number of inputs (up to 10)
    auto consumer = std::make_unique<ConsumerTask>();
    const size_t connect_count = std::min(producer_count, size_t(10));

    consumer->set_callable([](int a, int b, int c, int d, int e, int f, int g, int h, int i, int j) {
        return a + b + c + d + e + f + g + h + i + j;
    });

    // Add edges from producers to consumer
    if (connect_count > 0) {
        consumer->add_inward_edge<0>(producers[0].get_outward_edge());
    }
    if (connect_count > 1) {
        consumer->add_inward_edge<1>(producers[1].get_outward_edge());
    }
    if (connect_count > 2) {
        consumer->add_inward_edge<2>(producers[2].get_outward_edge());
    }
    if (connect_count > 3) {
        consumer->add_inward_edge<3>(producers[3].get_outward_edge());
    }
    if (connect_count > 4) {
        consumer->add_inward_edge<4>(producers[4].get_outward_edge());
    }
    if (connect_count > 5) {
        consumer->add_inward_edge<5>(producers[5].get_outward_edge());
    }
    if (connect_count > 6) {
        consumer->add_inward_edge<6>(producers[6].get_outward_edge());
    }
    if (connect_count > 7) {
        consumer->add_inward_edge<7>(producers[7].get_outward_edge());
    }
    if (connect_count > 8) {
        consumer->add_inward_edge<8>(producers[8].get_outward_edge());
    }
    if (connect_count > 9) {
        consumer->add_inward_edge<9>(producers[9].get_outward_edge());
    }

    return result_type(std::move(producers), std::move(consumer));
}

/**
 * @brief Generate fan-out pattern (one producer, many consumers)
 * @param consumer_count Number of consumer tasks
 * @return Pair of (unique_ptr to producer, consumers)
 *
 * Pattern:
 *              T0
 *            / | \
 *           /  |  \
 *          T1 T2  T3 ... Tn
 *
 * Returns unique_ptr for producer because Task is non-movable due to mutex members.
 */
inline auto
generate_fan_out(size_t consumer_count) -> std::pair<std::unique_ptr<Task<int>>, std::vector<Task<int, int>>>
{
    using result_type = std::pair<std::unique_ptr<Task<int>>, std::vector<Task<int, int>>>;

    if (consumer_count == 0) {
        return result_type(std::make_unique<Task<int>>(), std::vector<Task<int, int>>{});
    }

    auto producer = std::make_unique<Task<int>>();
    producer->set_callable([]() {
        return 42;
    });

    std::vector<Task<int, int>> consumers(consumer_count);
    for (size_t i = 0; i < consumer_count; ++i) {
        consumers[i].set_callable([i](int value) {
            return value + static_cast<int>(i);
        });
        consumers[i].add_inward_edge<0>(producer->get_outward_edge());
    }

    return result_type(std::move(producer), std::move(consumers));
}

// ============================================================================
// Validation Helpers
// ============================================================================

/**
 * @brief Verify results of a linear chain
 * @param tasks Vector of tasks in the chain
 * @param expected_final Expected final value
 * @return true if all tasks have correct results
 */
inline bool verify_linear_chain_results(const std::vector<Task<int, int>>& tasks, int expected_final)
{
    if (tasks.empty()) {
        return expected_final == 0;
    }

    // Last task should have the expected final value
    if (tasks.back().get_result() != expected_final) {
        return false;
    }

    // All tasks should be complete
    for (const auto& task : tasks) {
        if (task.get_state() != TaskState::Complete) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Verify results of a void linear chain
 * @param tasks Vector of tasks in the chain
 * @return true if all tasks are complete
 */
inline bool verify_void_linear_chain_results(const std::vector<std::unique_ptr<Task<void, void>>>& tasks)
{
    // All tasks should be complete
    for (const auto& task : tasks) {
        if (task->get_state() != TaskState::Complete) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Verify results of a binary tree
 * @param tasks Vector of tasks in the tree
 * @param levels Number of levels in the tree
 * @return true if root task has correct result
 *
 * For a tree with N levels, root should return 2^N - 1
 */
inline bool verify_tree_results(const std::vector<Task<int, int, int>>& tasks, size_t levels)
{
    if (tasks.empty()) {
        return levels == 0;
    }

    const int expected = static_cast<int>((1ULL << levels) - 1);
    return tasks[0].get_result() == expected;
}

/**
 * @brief Verify diamond pattern results
 * @param tasks Vector of tasks (groups of 4)
 * @param repetitions Number of diamond patterns
 * @return true if all diamonds have correct results
 *
 * For each diamond: bottom = left(2) + right(3) = 5
 */
inline bool verify_diamond_results(const std::vector<Task<int, int, int>>& tasks, size_t repetitions)
{
    if (tasks.empty()) {
        return repetitions == 0;
    }

    for (size_t i = 0; i < repetitions; ++i) {
        const size_t base = i * 4;
        // Bottom of diamond should be: left(2) + right(3) = 5
        if (tasks[base + 3].get_result() != 5) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// Statistics and Reporting
// ============================================================================

/**
 * @brief Calculate statistics for a vector of values
 * @param values Vector of values to analyze
 * @return Statistics structure with count, min, max, mean, stddev
 */
template<typename T>
inline Statistics<T> calculate_statistics(const std::vector<T>& values)
{
    Statistics<T> stats{
        .count = static_cast<T>(values.size()),
        .min = values.empty() ? T{} : *std::min_element(values.begin(), values.end()),
        .max = values.empty() ? T{} : *std::max_element(values.begin(), values.end()),
        .mean = 0.0,
        .stddev = 0.0};

    if (values.empty()) {
        return stats;
    }

    // Calculate mean
    const double sum = std::accumulate(values.begin(), values.end(), 0.0);
    stats.mean = sum / static_cast<double>(values.size());

    // Calculate standard deviation
    if (values.size() > 1) {
        double sq_sum = 0.0;
        for (const auto& value : values) {
            const double diff = static_cast<double>(value) - stats.mean;
            sq_sum += diff * diff;
        }
        stats.stddev = std::sqrt(sq_sum / static_cast<double>(values.size() - 1));
    }

    return stats;
}

/**
 * @brief Print timing result to stdout
 * @param test_name Name of the test
 * @param result Timing result to print
 */
inline void print_timing_result(const std::string& test_name, const TimingResult& result)
{
    std::cout << "=== " << test_name << " ===\n";
    std::cout << "  Total time:        " << result.total.count() << " ms\n";
    std::cout << "  Min task time:     " << result.min_task.count() << " μs\n";
    std::cout << "  Max task time:     " << result.max_task.count() << " μs\n";
    std::cout << "  Avg task time:     " << result.avg_task_ms << " ms\n";
    std::cout << "  Throughput:        " << result.tasks_per_second << " tasks/sec\n";
}

/**
 * @brief Print statistics to stdout
 * @param label Label for the statistics
 * @param stats Statistics to print
 */
template<typename T>
inline void print_statistics(const std::string& label, const Statistics<T>& stats)
{
    std::cout << "  " << label << ":\n";
    std::cout << "    Count: " << stats.count << "\n";
    std::cout << "    Min:   " << stats.min << "\n";
    std::cout << "    Max:   " << stats.max << "\n";
    std::cout << "    Mean:  " << stats.mean << "\n";
    std::cout << "    StdDev:" << stats.stddev << "\n";
}

// ============================================================================
// Configurable Constants
// ============================================================================

// Task counts for different stress levels
constexpr size_t kTaskCount_Light = 1000;
constexpr size_t kTaskCount_Medium = 5000;
constexpr size_t kTaskCount_Heavy = 10000;
constexpr size_t kTaskCount_Extreme = 50000;
constexpr size_t kTaskCount_Ultra = 100000;

// Timeout configurations
constexpr auto kDefaultTimeout = std::chrono::seconds(60);
constexpr auto kLongTimeout = std::chrono::seconds(300);
constexpr auto kShortTimeout = std::chrono::seconds(10);

// Thread counts for scaling tests
constexpr std::array<size_t, 5> kThreadCounts = {1, 2, 4, 8, 16};

} // namespace tw::stress
