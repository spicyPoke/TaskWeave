// Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

// STL
#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

namespace tw {

class INode;

/**
 * @brief Enumeration of task execution states.
 *
 * Represents the lifecycle states of a task during execution.
 */
enum class TaskState : uint32_t {
    Incomplete,  ///< Task has not started execution
    Running,     ///< Task is currently executing
    Complete,    ///< Task has finished execution
    Size_        ///< Count of states (for bounds checking)
};

/**
 * @brief Interface class representing a task.
 *
 * ITask defines the interface for all task implementations. It provides:
 * - Task identification (name, description)
 * - Execution state tracking (Incomplete, Running, Complete)
 * - Timing information (start time, end time, duration)
 * - Dependency graph integration (via INode)
 * - Execution control (run, wait)
 *
 * Implementations:
 * - Task<ReturnT, InputTs...>: Template implementation for typed tasks
 *
 * Thread Safety:
 * - State is atomic for lock-free access
 * - Timing data is not synchronized (assume single writer)
 */
class ITask {
public:
    /**
     * @brief Virtual destructor for polymorphic deletion.
     */
    virtual ~ITask() = default;

    /**
     * @brief Returns the task name.
     * @return String view of the task name.
     */
    auto get_name() const noexcept -> std::string_view
    {
        return name_;
    }

    /**
     * @brief Returns the task description.
     * @return String view of the task description.
     */
    auto get_description() const noexcept -> std::string_view
    {
        return desc_;
    }

    /**
     * @brief Returns the current execution state.
     * @return Current TaskState (Incomplete, Running, or Complete).
     *
     * @note Lock-free: uses atomic load with relaxed semantics.
     */
    auto get_state() const noexcept -> TaskState
    {
        return state_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Returns the task execution duration.
     * @tparam DurationRepT Duration type to cast the result to.
     * @return Duration representing execution time (end_time - start_time).
     *
     * @note Requires both start_time and end_time to be set.
     */
    template<typename DurationRepT>
    auto get_duration() const noexcept -> std::chrono::duration<typename DurationRepT::rep, typename DurationRepT::period>
    {
        return std::chrono::duration_cast<DurationRepT>(end_time_ - start_time_);
    }

    /**
     * @brief Executes the task logic.
     *
     * Implementations should:
     * 1. Set state to Running
     * 2. Record start time
     * 3. Wait for dependencies (inward edges) to be retrievable
     * 4. Execute the wrapped callable
     * 5. Set output data to outward edges
     * 6. Record end time and set state to Complete
     *
     * @note Should be called by the Executor worker.
     */
    virtual void run() = 0;

    /**
     * @brief Waits for the task to complete execution.
     * @return TaskState after completion.
     *
     * Blocks until the task reaches Complete state.
     *
     * @note Implementation should use condition variable or spin-wait.
     */
    virtual auto wait() const noexcept -> TaskState = 0;

    /**
     * @brief Comparison operator for topological sorting.
     * @param other Task to compare with.
     * @return true if this task should execute before the other task.
     *
     * Used by the Executor to sort tasks based on dependencies.
     * Delegates to the underlying INode comparison.
     */
    virtual auto operator<(const ITask& other) const noexcept -> bool = 0;

    /**
     * @brief Returns the underlying node for dependency graph integration.
     * @return Pointer to the INode representing this task.
     *
     * Allows access to dependency information (inward/outward edges, reachability).
     */
    virtual auto as_node() noexcept -> INode* = 0;

    /**
     * @brief Returns the underlying node for dependency graph integration (const).
     * @return Const pointer to the INode representing this task.
     */
    virtual auto as_node() const noexcept -> const INode* = 0;

protected:
    /**
     * @brief Sets the task start time.
     * @param start_time Time point when task execution began.
     */
    void set_start_time(const std::chrono::steady_clock::time_point& start_time) noexcept
    {
        start_time_ = start_time;
    }

    /**
     * @brief Sets the task end time.
     * @param end_time Time point when task execution completed.
     */
    void set_end_time(const std::chrono::steady_clock::time_point& end_time) noexcept
    {
        end_time_ = end_time;
    }

    /**
     * @brief Sets the task name.
     * @param name Name string to assign.
     */
    void set_name(const std::string& name) noexcept
    {
        name_ = name;
    }

    /**
     * @brief Sets the task description.
     * @param desc Description string to assign.
     */
    void set_description(const std::string& desc) noexcept
    {
        desc_ = desc;
    }

    /**
     * @brief Sets the task execution state.
     * @param state New TaskState to assign.
     *
     * @note Uses relaxed memory ordering (caller ensures synchronization).
     */
    void set_state(TaskState state) noexcept
    {
        state_.store(state, std::memory_order_relaxed);
    }

private:
    std::string name_;                                    ///< Task name
    std::string desc_;                                    ///< Task description
    std::chrono::steady_clock::time_point start_time_;    ///< Task start time
    std::chrono::steady_clock::time_point end_time_;      ///< Task end time
    std::atomic<TaskState> state_;                        ///< Current execution state
};

} // namespace tw
