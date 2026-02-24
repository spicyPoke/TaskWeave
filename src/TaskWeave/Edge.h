// Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "IEdge.h"
#include "INode.h"

// STL

namespace tw {

/**
 * @brief Edge class that carries data between task nodes.
 *
 * Edge is a templated class that encapsulates data passed from one task to another.
 * It provides thread-safe data transfer with synchronization:
 * - Producer tasks call set_data() to write data and mark it as retrievable
 * - Consumer tasks call get_data() to retrieve the data (after waiting if needed)
 *
 * @tparam T The type of data carried by the edge.
 *
 * Thread Safety:
 * - Uses IEdge's condition variable and mutex for synchronization
 * - wait_until_retrievable() blocks until set_data() is called
 *
 * Usage:
 * @code
 * Edge<int> edge(&producer_node);
 * producer_task: edge.set_data(42);
 * consumer_task: edge.wait_until_retrievable();
 *              int value = edge.get_data();
 * @endcode
 */
template<typename T>
class Edge : public IEdge {
public:
    /**
     * @brief Constructs an edge with the specified owner node.
     * @param owner Pointer to the node that owns this edge.
     */
    Edge(INode* owner)
        : IEdge(owner)
    {
    }

    /**
     * @brief Sets the data and marks the edge as retrievable.
     * @param data Data to store in the edge.
     *
     * This method:
     * 1. Stores the data in the edge
     * 2. Marks the edge as retrievable (via set_as_retrievable)
     * 3. Notifies all waiting tasks that data is available
     *
     * @note Thread-safe: triggers condition variable notification.
     */
    auto set_data(const T& data) noexcept -> void
    {
        data_ = data;
        set_as_retrievable();
    }

    /**
     * @brief Retrieves the data stored in the edge.
     * @return Copy of the stored data.
     *
     * Should be called after wait_until_retrievable() returns, or when
     * is_retrievable() returns true.
     *
     * @note Does not block - caller must ensure data is ready.
     */
    auto get_data() const noexcept -> T
    {
        return data_;
    }

private:
    T data_{};  ///< Data stored in the edge
};

/**
 * @brief Specialization of Edge for void type (no data transfer).
 *
 * This specialization is used when tasks have dependencies but don't
 * need to pass data between them. It only provides synchronization
 * to signal task completion.
 *
 * Usage:
 * @code
 * Edge<void> edge(&predecessor_node);
 * producer_task: edge.set_data();  // Signal completion
 * consumer_task: edge.wait_until_retrievable();  // Wait for signal
 * @endcode
 */
template<>
class Edge<void> : public IEdge {
public:
    /**
     * @brief Constructs a void edge with the specified owner node.
     * @param owner Pointer to the node that owns this edge.
     */
    Edge(INode* owner)
        : IEdge(owner)
    {
    }

    /**
     * @brief Marks the edge as retrievable without storing data.
     *
     * This method signals task completion to dependent tasks.
     * Calls set_as_retrievable() to notify all waiting tasks.
     *
     * @note Thread-safe: triggers condition variable notification.
     */
    auto set_data() noexcept -> void
    {
        set_as_retrievable();
    }
};

} // namespace tw
