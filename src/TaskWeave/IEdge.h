// Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

// STL
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <string>

namespace tw {

class INode;

/**
 * @brief Base class representing an edge in the task dependency graph.
 *
 * IEdge represents the connection between nodes (tasks) in the dependency graph.
 * Edges carry data between tasks and provide synchronization mechanisms to
 * ensure data is available before dependent tasks access it.
 *
 * Thread Safety:
 * - Uses atomic flag for retrievable state
 * - Condition variable for blocking until data is ready
 * - Mutex protects state transitions
 */
class IEdge {
public:
    /**
     * @brief Constructs an edge with the specified owner node.
     * @param owner Pointer to the node that owns this edge.
     */
    IEdge(INode* owner)
        : owner_(owner)
    {
    }

    /**
     * @brief Checks if the data contained by the edge is ready for retrieval.
     * @return true if data is available, false otherwise.
     *
     * @note Lock-free: uses atomic load with acquire semantics.
     * @warning Result may be stale immediately after return.
     */
    auto is_retrievable() const noexcept -> bool
    {
        return is_retrievable_.load(std::memory_order_acquire);
    }

    /**
     * @brief Returns the owner node of this edge.
     * @return Pointer to the owning INode.
     */
    auto get_owner() const noexcept -> INode*
    {
        return owner_;
    }

    /**
     * @brief Blocks until the edge data becomes retrievable.
     *
     * Waits on condition variable until is_retrievable() returns true.
     * Used by dependent tasks to synchronize data access.
     *
     * @note Thread-safe: uses mutex and condition variable.
     */
    auto wait_until_retrievable() const noexcept -> void
    {
        std::unique_lock lk{mtx_};
        cv_.wait(lk, [this]() {
            return is_retrievable_.load(std::memory_order_acquire);
        });
    }

protected:
    /**
     * @brief Marks the edge data as retrievable and notifies waiters.
     *
     * Called by the producing task after writing data to the edge.
     * Wakes up all tasks waiting for this edge's data.
     *
     * @note Thread-safe: uses mutex and condition variable.
     */
    auto set_as_retrievable() noexcept -> void
    {
        {
            std::unique_lock lk{mtx_};
            is_retrievable_.store(true, std::memory_order_release);
        }
        cv_.notify_all();
    }

private:
    INode* owner_;                              ///< Owner node of this edge
    std::string owner_name_{};                  ///< Owner node name (unused)
    mutable std::condition_variable cv_;        ///< Notifies when data is ready
    mutable std::mutex mtx_;                    ///< Protects retrievable state
    std::atomic<bool> is_retrievable_{};        ///< Flag indicating data availability
};

} // namespace tw
