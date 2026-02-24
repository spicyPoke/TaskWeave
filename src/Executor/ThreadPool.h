// Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "../TaskWeave/Metafunctions.h"

// STL
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace tw {

/**
 * @brief A fixed-size thread pool for concurrent task execution.
 *
 * ThreadPool provides a thread-safe mechanism for submitting and executing
 * callable tasks across a fixed number of worker threads. It supports
 * task queuing, completion notification, and graceful shutdown.
 *
 * Thread Safety:
 * - Task queue is protected by a shared_mutex for concurrent reads
 * - Worker coordination uses mutex and condition variable
 * - Active task count is atomic for lock-free queries
 *
 * Usage:
 * @code
 * ThreadPool pool{4};  // 4 worker threads
 * pool.run();          // Start workers
 * pool.add_task([]{ <task logic> });
 * pool.wait();         // Wait for completion
 * @endcode
 */
class ThreadPool {
public:
    /**
     * @brief Constructs a ThreadPool with specified worker count.
     * @param thread_count Number of worker threads to spawn.
     * @param on_complete_callback Optional callback invoked when all tasks complete.
     */
    explicit ThreadPool(size_t thread_count, const std::function<void()>& on_complete_callback = nullptr)
        : on_complete_(on_complete_callback)
        , thread_count_(thread_count)
    {
    }

    /**
     * @brief Destructor - initiates shutdown and joins all worker threads.
     *
     * Sets shutdown flag, notifies all workers to exit, and blocks until
     * all threads have been joined.
     */
    ~ThreadPool()
    {
        {
            std::lock_guard lck{worker_mtx_};
            is_shutting_down_ = true;
        }
        worker_cv_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    // Uncopyable class
    ThreadPool(const ThreadPool&) = delete;
    auto operator=(const ThreadPool&) -> ThreadPool& = delete;

    // Unmovable class
    ThreadPool(ThreadPool&& other) noexcept = delete;
    auto operator=(ThreadPool&& other) noexcept -> ThreadPool& = delete;

    /**
     * @brief Submits a callable task to the execution queue.
     *
     * @tparam Fn Callable type (function pointer, lambda, std::function, etc.).
     * @tparam Args Argument types to forward to the callable.
     * @param fn Callable to execute.
     * @param args Arguments to forward to the callable.
     * @return true if task was successfully queued, false if fn is nullptr.
     *
     * @note Performs compile-time check to reject nullptr function pointers
     *       and std::function objects. Other callable types bypass this check.
     * @note Thread-safe: acquires worker and task locks.
     */
    template<typename Fn, typename... Args>
    auto add_task(Fn&& fn, Args&&... args) -> bool
    {
        // Only check for nullptr if it's a function pointer or an std::function types
        if constexpr (std::is_pointer_v<std::remove_cvref_t<Fn>> || is_std_function_v<std::remove_cvref_t<Fn>>) {
            if (fn == nullptr) {
                return false;
            }
        }
        {
            {
                std::unique_lock worker_lck{worker_mtx_};
                std::unique_lock lck(tasks_mtx_);
                tasks_.emplace([f = std::forward<Fn>(fn), ... captured_args = std::forward<Args>(args)]() {
                    std::invoke(f, std::move(captured_args)...);
                });
                active_task_count_.fetch_add(1, std::memory_order_acq_rel);
            }
            worker_cv_.notify_one();
        }
        return true;
    }

    /**
     * @brief Clears all pending tasks from the queue without executing them.
     *
     * Reduces active task count by the number of cleared tasks.
     * Does not affect tasks currently in execution.
     *
     * @note Thread-safe: acquires task lock.
     */
    auto clear_queued_tasks() -> void
    {
        std::unique_lock lck{tasks_mtx_};
        active_task_count_.fetch_sub(tasks_.size(), std::memory_order_release);
        if (!tasks_.empty()) {
            tasks_ = {};
        }
    }

    /**
     * @brief Spawns worker threads and begins task processing.
     *
     * Must be called after construction to activate the thread pool.
     * Workers will block on condition variable until tasks are available.
     *
     * @note Not thread-safe: should be called once before / after submitting tasks.
     */
    auto run() noexcept -> void
    {
        spawn_thread(thread_count_);
    }

    /**
     * @brief Blocks until all submitted tasks have completed execution.
     *
     * Waits on condition variable until active_task_count_ reaches zero.
     * Safe to call from any thread.
     *
     * @note Thread-safe: uses dedicated wait mutex.
     */
    auto wait() const noexcept -> void
    {
        std::unique_lock lck(wait_mtx_);
        wait_cv_.wait(lck, [this]() {
            return is_idle();
        });
    }

    /**
     * @brief Returns a copy of the current task queue.
     * @return Copy of pending tasks queue (does not include executing tasks).
     *
     * @note Thread-safe: uses shared lock allowing concurrent reads.
     * @warning Creates a full copy of the queue - may be expensive.
     */
    auto get_queued_tasks() const noexcept -> std::queue<std::function<void()>>
    {
        std::shared_lock lck(tasks_mtx_);
        return tasks_;
    }

    /**
     * @brief Checks if the thread pool has no active tasks.
     * @return true if no tasks are currently executing or queued.
     *
     * @note Lock-free: uses atomic load with acquire semantics.
     * @warning Race condition possible: result may be stale immediately.
     */
    auto is_idle() const noexcept -> bool
    {
        auto count = active_task_count_.load(std::memory_order_acquire);
        return count == 0;
    }

    /**
     * @brief Returns the count of currently active tasks.
     * @return Number of tasks being executed or waiting in queue.
     *
     * @note Lock-free: uses atomic load with acquire semantics.
     */
    auto active_task_count() const noexcept -> int
    {
        return active_task_count_.load(std::memory_order_acquire);
    }

    /**
     * @brief Checks if the task queue is empty.
     * @return true if no tasks are waiting in the queue.
     *
     * @note Thread-safe: uses shared lock.
     * @warning Does not indicate if tasks are currently executing.
     */
    auto empty() const noexcept -> bool
    {
        std::shared_lock lck{tasks_mtx_};
        return tasks_.empty();
    }

    /**
     * @brief Returns the number of tasks in the queue.
     * @return Count of pending tasks (not including executing tasks).
     *
     * @note Thread-safe: uses shared lock.
     */
    auto size() const noexcept -> size_t
    {
        std::shared_lock lck{tasks_mtx_};
        return tasks_.size();
    }

    /**
     * @brief Returns the configured number of worker threads.
     * @return Thread count specified at construction.
     */
    auto worker_count() const noexcept -> size_t
    {
        return thread_count_;
    }

private:
    /**
     * @brief Executes a single task from the queue.
     *
     * Pops the front task from the queue and invokes it.
     * Decrements active task count and notifies waiters if count reaches zero.
     * Invokes on_complete_ callback when all tasks finish.
     *
     * @note Called by worker threads only.
     */
    auto execute_task() -> void
    {
        std::function<void()> task_to_do;
        {
            std::unique_lock lck{tasks_mtx_};
            if (!tasks_.empty()) {
                task_to_do = std::move(tasks_.front());
                tasks_.pop();
            }
        }

        if (task_to_do) {
            task_to_do();
            std::unique_lock lck{wait_mtx_};
            active_task_count_.fetch_sub(1, std::memory_order_acq_rel);
            auto count = active_task_count_.load(std::memory_order_acquire);
            if (count == 0) {
                if (on_complete_) {
                    on_complete_();
                }
                wait_cv_.notify_all();
            }
        }
    }

    /**
     * @brief Spawns worker threads that continuously process tasks.
     *
     * Each worker loops until shutdown, waiting on condition variable
     * for tasks to become available or shutdown signal.
     *
     * @param count Number of threads to spawn.
     * @note Called once during run().
     */
    auto spawn_thread(size_t count) -> void
    {
        for (size_t i = 0; i < count; i++) {
            workers_.emplace_back([this]() {
                while (!is_shutting_down_) {
                    {
                        std::unique_lock lock{worker_mtx_};
                        worker_cv_.wait(lock, [this]() {
                            return !empty() || is_shutting_down_;
                        });
                    }
                    if (is_shutting_down_) {
                        return;
                    }
                    execute_task();
                }
            });
        }
    }

private:
    std::queue<std::function<void()>> tasks_;       ///< Queue of pending tasks
    std::vector<std::thread> workers_;              ///< Worker thread handles
    mutable std::shared_mutex tasks_mtx_;           ///< Protects task queue (shared for reads)
    std::mutex worker_mtx_;                         ///< Protects shutdown flag
    std::condition_variable worker_cv_;             ///< Notifies workers of new tasks
    mutable std::mutex wait_mtx_;                   ///< Protects wait condition
    mutable std::condition_variable wait_cv_;       ///< Notifies waiters when idle
    std::function<void()> on_complete_;             ///< Callback when all tasks complete
    std::atomic<int> active_task_count_{0};         ///< Count of active/pending tasks
    size_t thread_count_{};                         ///< Configured worker count
    bool is_shutting_down_ = false;                 ///< Shutdown flag
};
} // namespace tw
