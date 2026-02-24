// Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

// STL
#include <set>
#include <vector>

namespace tw {

class IEdge;

/**
 * @brief Interface class representing a node in the task dependency graph.
 *
 * INode represents a task node in the dependency graph. It provides methods
 * for accessing inward edges (dependencies) and computing reachability
 * (the number of tasks that depend on this task, directly or transitively).
 *
 * Implementations:
 * - Node<ReturnT, InputTs...>: Template implementation for typed tasks
 *
 * The reachability computation uses a traversal algorithm with markers to
 * avoid counting the same dependent task multiple times.
 */
class INode {
public:
    /**
     * @brief Marker set used during reachability traversal.
     *
     * Stores node identifiers visited during reachability computation
     * to prevent counting the same node multiple times.
     */
    using TraverseMarkerT = std::set<size_t>;

public:
    /**
     * @brief Virtual destructor for polymorphic deletion.
     */
    virtual ~INode() = default;

    /**
     * @brief Returns all inward edges (dependencies) of this node.
     * @return Vector of pointers to inward edges.
     *
     * Inward edges represent data flowing into this task from predecessor tasks.
     */
    virtual auto get_inward_edges() const noexcept -> std::vector<const IEdge*> = 0;

    /**
     * @brief Returns the count of inward edges.
     * @return Number of dependencies this task has.
     */
    virtual auto get_inward_edges_count() const noexcept -> size_t = 0;

    /**
     * @brief Comparison operator for topological sorting.
     * @param other Node to compare with.
     * @return true if this node should execute before the other node.
     *
     * Used to sort tasks so that tasks with dependencies execute after
     * their dependencies complete.
     */
    virtual auto operator<(const INode& other) const noexcept -> bool = 0;

    /**
     * @brief Returns the reachability count for this node.
     * @return Reachability number, that is tasks that this task depend on (directly or transitively).
     *
     * Reachability is computed by traversing inward edges and counting
     * unique reachable nodes.
     *
     * @note Must call set_reachability() before calling this method.
     */
    virtual auto get_reachability() const noexcept -> size_t = 0;

    /**
     * @brief Computes and sets the reachability for this node.
     *
     * Traverses the dependency graph starting from this node and counts
     * all reachable nodes. Uses an internal traverse marker.
     *
     * @note Should be called before task execution to enable proper ordering.
     */
    virtual auto set_reachability() noexcept -> void = 0;

    /**
     * @brief Computes and sets the reachability using a provided traverse marker.
     * @param traverse_marker Marker set to track visited nodes during traversal.
     *
     * Overload that allows sharing a traverse marker across multiple nodes
     * to avoid redundant counting when computing reachability for a group
     * of tasks that overlap.
     *
     * @note The marker is modified during traversal.
     */
    virtual auto set_reachability(TraverseMarkerT& traverse_marker) noexcept -> void = 0;
};

} // namespace tw
