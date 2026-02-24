// Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "TaskWeave/Helper.h"
#include "TaskWeave/ITask.h"
#include "ThreadPool.h"

// STL
#include <memory>
#include <mutex>
#include <thread>

namespace tw {

/**
 * @brief High-level executor that manages task distribution to a ThreadPool.
 *
 * ThreadPoolExecutor provides a simplified interface for executing tasks
 * with automatic dependency management. It handles:
 * - Automatic reachability computation for task dependencies
 * - Topological sorting of tasks based on dependencies
 * - Task submission to the underlying thread pool
 *
 * Usage:
 * @code
 * ThreadPoolExecutor executor;
 * executor.add_task(&task1);
 * executor.add_task(&task2);
 * executor.run();
 * executor.wait();
 * @endcode
 */
class ThreadPoolExecutor {
public:
    /**
     * @brief Default constructor - creates executor without a thread pool.
     *
     * Thread pool will be lazily created in run() if not provided.
     */
    ThreadPoolExecutor() = default;

    /**
     * @brief Move constructor - transfers ownership from another executor.
     * @param other Executor to move from.
     */
    ThreadPoolExecutor(ThreadPoolExecutor&& other) noexcept
    {
        *this = std::move(other);
    }

    /**
     * @brief Move assignment operator - transfers ownership from another executor.
     * @param other Executor to move from.
     * @return Reference to this executor.
     */
    auto operator=(ThreadPoolExecutor&& other) noexcept -> ThreadPoolExecutor&
    {
        pool_ = std::move(other.pool_);
        tasks_to_run_ = std::move(other.tasks_to_run_);
        return *this;
    }

    // Uncopyable class
    ThreadPoolExecutor(ThreadPoolExecutor const&) = delete;
    auto operator=(ThreadPoolExecutor const&) -> ThreadPoolExecutor& = delete;

    /**
     * @brief Adds a task to the execution queue.
     * @param task Pointer to task implementing ITask interface.
     *
     * @note Tasks should be added before calling run().
     * @note Thread pool is not created until run() is called.
     */
    void add_task(ITask* task)
    {
        tasks_to_run_.emplace_back(task);
    }

    /**
     * @brief Prepares and submits all tasks to the thread pool for execution.
     *
     * This method:
     * 1. Creates thread pool if not already created (uses hardware_concurrency)
     * 2. Computes reachability for automatic dependency detection
     * 3. Sorts tasks topologically based on dependencies
     * 4. Submits all tasks to the thread pool
     * 5. Starts worker threads
     *
     * @note Tasks with dependencies will execute only after their dependencies complete.
     * @note Thread pool size defaults to std::thread::hardware_concurrency() if not set.
     */
    void run()
    {
        if (pool_ == nullptr) {
            pool_ = std::make_unique<ThreadPool>(std::thread::hardware_concurrency());
        }
        // Auto-compute reachability before sorting and execution
        tw::compute_reachability(tasks_to_run_);
        std::sort(tasks_to_run_.begin(), tasks_to_run_.end(), [](auto a, auto b) {
            return *a < *b;
        });
        for (auto const& task : tasks_to_run_) {
            pool_->add_task(std::function<void()>([task]() {
                task->run();
            }));
        }
        pool_->run();
    }

    /**
     * @brief Cancels all pending tasks in the queue.
     *
     * Removes tasks that have been queued but not yet started execution.
     * Does not affect tasks currently running or already completed.
     *
     * @note Safe to call even if thread pool was not created.
     */
    void cancel()
    {
        if (pool_ == nullptr) {
            return;
        }
        pool_->clear_queued_tasks();
    }

    /**
     * @brief Blocks until all submitted tasks have completed execution.
     *
     * Waits for the underlying thread pool to finish all tasks.
     *
     * @note Safe to call even if thread pool was not created (no-op).
     */
    void wait()
    {
        if (pool_ == nullptr) {
            return;
        }
        pool_->wait();
    }

private:
    std::unique_ptr<ThreadPool> pool_;          ///< Underlying thread pool
    std::vector<ITask*> tasks_to_run_;          ///< Tasks pending execution
};
} // namespace tw
