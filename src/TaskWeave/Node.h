// Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "Edge.h"
#include "INode.h"
#include "Metafunctions.h"

// STL
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace tw {

/**
 * @brief Template implementation of a node in the task dependency graph.
 *
 * Node represents a task vertex in the dependency graph with:
 * - One outward edge carrying the task's output (OutwardT)
 * - Zero or more inward edges carrying inputs from predecessor tasks (InwardTs...)
 *
 * @tparam OutwardT The output type produced by this task (void for no output).
 * @tparam InwardTs The input types consumed by this task from dependencies.
 *
 * Edge Management:
 * - Inward edges are stored in a tuple indexed by type or position
 * - Outward edge is owned by this node and created at construction
 *
 * Reachability:
 * - Computed as max(inward reachabilities) + 1
 * - Root nodes (no dependencies) have reachability 0
 * - Used for topological sorting: lower reachability executes first
 *
 * Thread Safety:
 * - Edge access uses IEdge's synchronization
 * - Reachability computation is single-threaded (before execution)
 */
template<typename OutwardT, typename... InwardTs>
class Node : public INode {
private:
    using InwardEdgesTupleType = std::tuple<const Edge<InwardTs>*...>;
    constexpr static size_t inward_edges_type_count = sizeof...(InwardTs);

public:
    /**
     * @brief Constructs a node with initialized outward edge.
     *
     * The outward edge is owned by this node and points to itself as owner.
     * Inward edges are initially null and must be set via add_inward_edge().
     */
    Node()
        : out_edge_(this)
    {
    }

    /**
     * @brief Adds an inward edge by type matching.
     * @tparam InputT The type of data carried by the edge.
     * @param edge Pointer to the edge to add.
     *
     * Finds the matching edge slot in the tuple by type and stores the pointer.
     * Requires no duplicate types in InwardTs for unambiguous matching.
     *
     * @note Compile-time assertion fails if InputT is not in InwardTs.
     */
    template<typename InputT>
        requires(contained_no_duplicate_types<InwardEdgesTupleType>)
    auto add_inward_edge(const Edge<InputT>* edge) noexcept -> void
    {
        if constexpr ((std::is_same_v<InputT, InwardTs> || ...)) {
            auto& in_edge = std::get<const Edge<InputT>*>(in_edges_);
            in_edge = edge;
        }
        else {
            // Fail the compilation here
            static_assert(tw::as_false_v<InputT>, "This type is not one of the Node's input types.");
        }
    }

    /**
     * @brief Adds an inward edge by explicit index.
     * @tparam Idx The index of the edge in the tuple.
     * @tparam InputT The type of data carried by the edge.
     * @param edge Pointer to the edge to add.
     *
     * Stores the edge at the specified tuple index.
     * Both type and index must match the declared InwardTs.
     *
     * @note Compile-time assertion fails if type/index mismatch.
     */
    template<size_t Idx, typename InputT>
    auto add_inward_edge(const Edge<InputT>* edge) noexcept -> void
    {
        if constexpr (
            (std::is_same_v<InputT, InwardTs> || ...) &&
            std::is_same_v<std::tuple_element_t<Idx, decltype(in_edges_)>, const Edge<InputT>*>) {
            auto& in_edge = std::get<Idx>(in_edges_);
            in_edge = edge;
        }
        else {
            // Fail the compilation here
            static_assert(tw::as_false_v<InputT>, "This type is not one of the Node's input types.");
        }
    }

    /**
     * @brief Retrieves data from an inward edge by type.
     * @tparam InputT The type of data to retrieve.
     * @return Copy of the data, or default-constructed value if edge is null.
     *
     * Waits for the edge to be retrievable before accessing data.
     * Requires no duplicate types for unambiguous matching.
     */
    template<typename InputT>
        requires(contained_no_duplicate_types<InwardEdgesTupleType>)
    auto get_inward_edge_value() const noexcept -> InputT
    {
        auto edge = std::get<const Edge<InputT>*>(in_edges_);
        if (edge) {
            return edge->get_data();
        }
        else {
            return InputT();
        }
    }

    /**
     * @brief Retrieves data from an inward edge by index.
     * @tparam Idx The index of the edge in the tuple.
     * @return Copy of the data, or default-constructed value if edge is null.
     *
     * Waits for the edge to be retrievable before accessing data.
     */
    template<size_t Idx>
    auto get_inward_edge_value() const noexcept -> std::tuple_element_t<Idx, std::tuple<InwardTs...>>
    {
        auto edge = std::get<Idx>(in_edges_);
        if (edge) {
            return edge->get_data();
        }
        else {
            return std::tuple_element_t<Idx, std::tuple<InwardTs...>>();
        }
    }

    /**
     * @brief Returns the outward edge of this node.
     * @return Pointer to the outward edge carrying output data.
     *
     * The returned edge is owned by this node and remains valid for its lifetime.
     */
    auto get_outward_edge() const noexcept -> const Edge<OutwardT>*
    {
        return &out_edge_;
    }

    /**
     * @brief Returns all inward edges as a vector.
     * @return Vector of const IEdge pointers.
     *
     * Expands the tuple of inward edges into a homogeneous vector.
     * Used for reachability computation and dependency traversal.
     */
    auto get_inward_edges() const noexcept -> std::vector<const IEdge*> override
    {
        return [this]<size_t... Idxs>(std::index_sequence<Idxs...>) {
            return std::vector<const IEdge*>{std::get<Idxs>(in_edges_)...};
        }(std::make_index_sequence<std::tuple_size_v<decltype(in_edges_)>>{});
    }

    /**
     * @brief Returns the count of inward edges.
     * @return Number of input dependencies.
     */
    auto get_inward_edges_count() const noexcept -> size_t override
    {
        return inward_edges_type_count;
    }

    /**
     * @brief Returns the computed reachability value.
     * @return Number of tasks that this task depend on (directly or transitively).
     *
     * @note Must call set_reachability() before calling this method.
     */
    auto get_reachability() const noexcept -> size_t override
    {
        return reachability_;
    }

    /**
     * @brief Computes and sets the reachability for this node.
     *
     * Algorithm:
     * 1. Create a traverse marker to track visited nodes
     * 2. For each inward edge, recursively compute predecessor reachability
     * 3. Set reachability to max(inward reachabilities) + 1
     *
     * Root nodes (no dependencies) get reachability 0.
     */
    auto set_reachability() noexcept -> void override
    {
        std::set<size_t> traverse_marker;
        auto inward_edges = get_inward_edges();
        std::array<int, inward_edges_type_count> inward_reachabilities{};
        assert(inward_edges.size() <= inward_edges_type_count);

        for (decltype(inward_edges.size()) i = 0; i < inward_edges.size(); i++) {
            auto edge = inward_edges[i];
            size_t current_inward_node_reachability = 0;
            if (edge) {
                auto node = edge->get_owner();
                node->set_reachability(traverse_marker);
                current_inward_node_reachability = node->get_reachability();
            }
            inward_reachabilities[i] = current_inward_node_reachability;
        }
        reachability_ = *std::max_element(inward_reachabilities.begin(), inward_reachabilities.end()) + 1;
    }

    /**
     * @brief Computes and sets reachability using a shared traverse marker.
     * @param traverse_marker Marker set to track visited nodes across multiple calls.
     *
     * Overload that shares a traverse marker to avoid redundant counting
     * when computing reachability for a group of tasks.
     *
     * If this node was already visited (in marker), returns early.
     */
    auto set_reachability(TraverseMarkerT& traverse_marker) noexcept -> void override
    {
        if (traverse_marker.find(reinterpret_cast<size_t>(this)) != traverse_marker.end()) {
            return;
        }
        traverse_marker.emplace(reinterpret_cast<size_t>(this));
        auto inward_edges = get_inward_edges();
        std::array<int, inward_edges_type_count> inward_reachabilities{};
        assert(inward_edges.size() <= inward_edges_type_count);

        for (decltype(inward_edges.size()) i = 0; i < inward_edges.size(); i++) {
            auto edge = inward_edges[i];
            size_t current_inward_node_reachability = 0;
            if (edge) {
                auto node = edge->get_owner();
                node->set_reachability(traverse_marker);
                current_inward_node_reachability = node->get_reachability();
            }
            inward_reachabilities[i] = current_inward_node_reachability;
        }
        reachability_ = *std::max_element(inward_reachabilities.begin(), inward_reachabilities.end()) + 1;
    }

    /**
     * @brief Comparison operator for topological sorting.
     * @param other Node to compare with.
     * @return true if this node has lower reachability than other.
     *
     * Tasks with lower reachability execute first (fewer dependents).
     */
    auto operator<(const INode& other) const noexcept -> bool override
    {
        return get_reachability() < other.get_reachability();
    }

protected:
    /**
     * @brief Sets the outward edge data for void output type.
     *
     * Marks the outward edge as retrievable without storing data.
     * Called after task execution completes.
     */
    auto set_out_edge_data() noexcept -> void
        requires std::is_same_v<OutwardT, void>
    {
        out_edge_.set_data();
    }

    /**
     * @brief Sets the outward edge data for non-void output type.
     * @tparam U Output type (deduced from argument).
     * @param data Data to store in the outward edge.
     *
     * Stores the task result in the outward edge and marks it retrievable.
     * Called after task execution completes.
     */
    template<typename U = OutwardT>
    auto set_out_edge_data(const U& data) noexcept -> void
        requires(!std::is_same_v<U, void>)
    {
        out_edge_.set_data(data);
    }

private:
    InwardEdgesTupleType in_edges_{}; ///< Tuple of inward edge pointers
    Edge<OutwardT> out_edge_;         ///< Outward edge owned by this node
    size_t reachability_{};           ///< Computed reachability value
};

/**
 * @brief Specialization of Node for tasks with no dependencies.
 *
 * This specialization handles root nodes in the dependency graph.
 * These nodes have:
 * - No inward edges (no dependencies)
 * - Reachability of 0 (base case)
 * - Only an outward edge for output
 *
 * @tparam OutwardT The output type produced by this task.
 */
template<typename OutwardT>
class Node<OutwardT> : public INode {
private:
    constexpr static size_t inward_edges_type_count = 0;

public:
    /**
     * @brief Constructs a root node with initialized outward edge.
     */
    Node()
        : out_edge_(this)
    {
    }

    /**
     * @brief Returns empty vector (no dependencies).
     * @return Empty vector of IEdge pointers.
     */
    auto get_inward_edges() const noexcept -> std::vector<const IEdge*> override
    {
        return {};
    }

    /**
     * @brief Returns the outward edge of this node.
     * @return Pointer to the outward edge carrying output data.
     */
    auto get_outward_edge() const noexcept -> const Edge<OutwardT>*
    {
        return &out_edge_;
    }

    /**
     * @brief Returns zero (no dependencies).
     * @return 0
     */
    auto get_inward_edges_count() const noexcept -> size_t override
    {
        return inward_edges_type_count;
    }

    /**
     * @brief Returns zero reachability for root nodes.
     * @return 0
     */
    auto get_reachability() const noexcept -> size_t override
    {
        return 0;
    }

    /**
     * @brief No-op for root nodes (no dependencies to traverse).
     */
    auto set_reachability() noexcept -> void override
    {
    }

    /**
     * @brief Marks this node as visited and returns.
     * @param traverse_marker Marker set to track visited nodes.
     *
     * Root nodes have no dependencies, so just mark as visited.
     */
    auto set_reachability(TraverseMarkerT& traverse_marker) noexcept -> void override
    {
        if (traverse_marker.find(reinterpret_cast<size_t>(this)) != traverse_marker.end()) {
            return;
        }
        traverse_marker.emplace(reinterpret_cast<size_t>(this));
    }

    /**
     * @brief Comparison operator for topological sorting.
     * @param other Node to compare with.
     * @return true if this node has lower reachability than other.
     *
     * Root nodes always have reachability 0, so they execute first.
     */
    auto operator<(const INode& other) const noexcept -> bool override
    {
        return get_reachability() < other.get_reachability();
    }

protected:
    /**
     * @brief Sets the outward edge data for void output type.
     */
    auto set_out_edge_data() noexcept -> void
        requires std::is_same_v<OutwardT, void>
    {
        out_edge_.set_data();
    }

    /**
     * @brief Sets the outward edge data for non-void output type.
     * @tparam U Output type (deduced from argument).
     * @param data Data to store in the outward edge.
     */
    template<typename U = OutwardT>
    auto set_out_edge_data(const U& data) noexcept -> void
        requires(!std::is_same_v<U, void>)
    {
        out_edge_.set_data(data);
    }

private:
    Edge<OutwardT> out_edge_; ///< Outward edge owned by this node
};

} // namespace tw
