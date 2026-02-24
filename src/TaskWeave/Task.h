// Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "ITask.h"
#include "Metafunctions.h"
#include "Node.h"
#include "TaskWeave/INode.h"

// STL
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <utility>

namespace tw {

/**
 * @brief Template class representing a task with return type and dependencies.
 *
 * Task is the primary unit of work. It combines:
 * - ITask interface for execution control and state tracking
 * - Node<ReturnT, InputTs...> for dependency graph integration
 *
 * @tparam ReturnT The return type of the task (void specialization exists).
 * @tparam InputTs The input types from predecessor tasks (dependencies).
 *
 * Execution Flow:
 * 1. Wait for all inward edges (dependencies) to be retrievable
 * 2. Set state to Running and record start time
 * 3. Retrieve input values from inward edges
 * 4. Execute the wrapped callable
 * 5. Store result and set outward edge data
 * 6. Record end time and set state to Complete
 * 7. Notify waiting threads
 *
 * Thread Safety:
 * - Uses condition variable for wait() synchronization
 * - State is atomic (via ITask)
 * - Callable is set before execution (single writer)
 *
 * Usage:
 * @code
 * Task<int, double> task;  // Returns int, takes double from dependency
 * task.set_callable([](double value) { return static_cast<int>(value * 2); });
 * task.run();  // Executes after dependencies complete
 * task.wait(); // Blocks until complete
 * int result = task.get_result();
 * @endcode
 */
template<typename ReturnT, typename... InputTs>
class Task
    : public Node<ReturnT, InputTs...>
    , public ITask {
private:
    using SuperNode = Node<ReturnT, InputTs...>;
    using SuperTask = ITask;

public:
    /**
     * @brief Executes the task logic.
     *
     * This method:
     * 1. Waits for all dependency edges to be retrievable
     * 2. Sets state to Running and records start time
     * 3. Retrieves input values from inward edges (filtering void types)
     * 4. Invokes the wrapped callable with inputs
     * 5. Stores the result
     * 6. Sets the outward edge data for successor tasks
     * 7. Records end time and sets state to Complete
     * 8. Notifies one waiting thread
     *
     * @note Called by ThreadPool worker threads.
     * @note Blocks until all dependencies complete.
     */
    virtual void run() override
    {
        for (const auto edge : SuperNode::get_inward_edges()) {
            if (edge) {
                edge->wait_until_retrievable();
            }
        }
        set_state(TaskState::Running);
        set_start_time(std::chrono::steady_clock::now());

        // Get input values and call the function with them
        if constexpr (sizeof...(InputTs) == 0) {
            result_ = callable_();
        }
        else {
            result_ = run_impl(typename integer_sequence_void_filter<InputTs...>::filtered{});
        }

        set_end_time(std::chrono::steady_clock::now());
        SuperNode::set_out_edge_data(result_);
        set_state(TaskState::Complete);
        std::lock_guard lk{mtx_};
        cv_.notify_one();
    }

    /**
     * @brief Helper to invoke callable with unpacked edge values.
     * @tparam Idx Indices for tuple element access.
     * @return Result of calling the wrapped callable.
     *
     * Retrieves values from inward edges by index and forwards to callable.
     * Used when InputTs contains non-void types.
     */
    template<size_t... Idx>
    auto run_impl(std::index_sequence<Idx...>) -> ReturnT
    {
        return callable_(SuperNode::template get_inward_edge_value<Idx>()...);
    }

    /**
     * @brief Blocks until task execution completes.
     * @return TaskState after completion (should be Complete).
     *
     * Waits on condition variable until state becomes TaskState::Complete.
     * Safe to call from any thread.
     *
     * @note Thread-safe: uses mutex and condition variable.
     */
    virtual auto wait() const noexcept -> TaskState override
    {
        std::unique_lock lk{mtx_};
        cv_.wait(lk, [this]() {
            return get_state() == TaskState::Complete;
        });
        return get_state();
    }

    /**
     * @brief Returns the underlying node for dependency graph integration.
     * @return Pointer to INode (this object).
     */
    virtual auto as_node() noexcept -> INode* override
    {
        return static_cast<INode*>(this);
    }

    /**
     * @brief Returns the underlying node for dependency graph integration (const).
     * @return Const pointer to INode (this object).
     */
    virtual auto as_node() const noexcept -> const INode* override
    {
        return static_cast<const INode*>(this);
    }

    /**
     * @brief Comparison operator for topological sorting.
     * @param other Task to compare with.
     * @return true if this task should execute before other.
     *
     * Delegates to INode comparison based on reachability.
     */
    virtual auto operator<(const ITask& other) const noexcept -> bool override
    {
        return *(this->as_node()) < *(other.as_node());
    }

    /**
     * @brief Returns the computed result of the task.
     * @return Copy of the result value.
     *
     * Should be called after wait() returns to ensure result is ready.
     */
    auto get_result() const noexcept -> ReturnT
    {
        return result_;
    }

    /**
     * @brief Sets the callable to be executed by this task.
     * @tparam FuncT Callable type (deduced).
     * @param fn Callable object to store.
     *
     * @note Requires callable to match signature: ReturnT(InputTs...)
     *       (void types in InputTs are filtered out)
     * @note Must be called before run().
     */
    template<typename FuncT>
        requires has_required_prototype2<FuncT, ReturnT, remove_voids<InputTs...>>
    auto set_callable(FuncT&& fn) noexcept -> void
    {
        callable_ = std::forward<FuncT>(fn);
    }

private:
    typename function_from_tuple<ReturnT, remove_voids<InputTs...>>::type callable_;  ///< Wrapped callable
    ReturnT result_;                                                                  ///< Computed result
    mutable std::condition_variable cv_;                                              ///< Completion notifier
    mutable std::mutex mtx_;                                                          ///< Protects wait
};

/**
 * @brief Specialization of Task for void return type.
 *
 * This specialization handles tasks that doesn't have a return value. 
 * The execution flow is identical to the primary template, except:
 * - No result is stored
 * - Outward edge is marked retrievable without data
 *
 * @tparam InputTs The input types from predecessor tasks (dependencies).
 */
template<typename... InputTs>
class Task<void, InputTs...>
    : public Node<void, InputTs...>
    , public ITask {
private:
    using SuperNode = Node<void, InputTs...>;
    using SuperTask = ITask;

public:
    /**
     * @brief Executes the task logic (void return specialization).
     *
     * This method:
     * 1. Waits for all dependency edges to be retrievable
     * 2. Sets state to Running and records start time
     * 3. Retrieves input values from inward edges (filtering void types)
     * 4. Invokes the wrapped callable with inputs
     * 5. Sets the outward edge as retrievable (no data)
     * 6. Records end time and sets state to Complete
     * 7. Notifies one waiting thread
     *
     * @note Called by ThreadPool worker threads.
     * @note Blocks until all dependencies complete.
     */
    virtual void run() override
    {
        // Wait for all dependencies
        for (const auto edge : SuperNode::get_inward_edges()) {
            if (edge) {
                edge->wait_until_retrievable();
            }
        }
        set_state(TaskState::Running);
        set_start_time(std::chrono::steady_clock::now());

        // Get input values and call the function with them
        if constexpr (sizeof...(InputTs) == 0) {
            callable_();
        }
        else {
            run_impl(typename integer_sequence_void_filter<InputTs...>::filtered{});
        }

        set_end_time(std::chrono::steady_clock::now());
        SuperNode::set_out_edge_data();
        set_state(TaskState::Complete);
        std::lock_guard lk{mtx_};
        cv_.notify_one();
    }

    /**
     * @brief Helper to invoke callable with unpacked edge values.
     * @tparam Idx Indices for tuple element access.
     *
     * Retrieves values from inward edges by index and forwards to callable.
     */
    template<size_t... Idx>
    void run_impl(std::index_sequence<Idx...>)
    {
        callable_(SuperNode::template get_inward_edge_value<Idx>()...);
    }

    /**
     * @brief Blocks until task execution completes.
     * @return TaskState after completion (should be Complete).
     *
     * Waits on condition variable until state becomes TaskState::Complete.
     *
     * @note Thread-safe: uses mutex and condition variable.
     */
    virtual auto wait() const noexcept -> TaskState override
    {
        std::unique_lock lk{mtx_};
        cv_.wait(lk, [this]() {
            return get_state() == TaskState::Complete;
        });
        return get_state();
    }

    /**
     * @brief Returns the underlying node for dependency graph integration.
     * @return Pointer to INode (this object).
     */
    virtual auto as_node() noexcept -> INode* override
    {
        return static_cast<INode*>(this);
    }

    /**
     * @brief Returns the underlying node for dependency graph integration (const).
     * @return Const pointer to INode (this object).
     */
    virtual auto as_node() const noexcept -> const INode* override
    {
        return static_cast<const INode*>(this);
    }

    /**
     * @brief Comparison operator for topological sorting.
     * @param other Task to compare with.
     * @return true if this task should execute before other.
     *
     * Delegates to INode comparison based on reachability.
     */
    virtual auto operator<(const ITask& other) const noexcept -> bool override
    {
        return *(this->as_node()) < *(other.as_node());
    }

    /**
     * @brief Sets the callable to be executed by this task.
     * @tparam FuncT Callable type (deduced).
     * @param fn Callable object to store.
     *
     * @note Requires callable to match signature: void(InputTs...)
     *       (void types in InputTs are filtered out)
     * @note Must be called before run().
     */
    template<typename FuncT>
        requires has_required_prototype2<FuncT, void, remove_voids<InputTs...>>
    auto set_callable(FuncT&& fn) noexcept -> void
    {
        callable_ = std::forward<FuncT>(fn);
    }

private:
    typename function_from_tuple<void, remove_voids<InputTs...>>::type callable_;  ///< Wrapped callable
    mutable std::condition_variable cv_;                                           ///< Completion notifier
    mutable std::mutex mtx_;                                                       ///< Protects wait
};

} // namespace tw
