// Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "INode.h"

namespace tw {

/**
 * @brief Computes reachability for a collection of tasks.
 *
 * This function computes the reachability (dependency depth) for all tasks
 * in the given range. Reachability represents the longest path from a task
 * to any of its dependency in the dependency graph.
 *
 * Algorithm:
 * 1. Creates a shared traverse marker to track visited nodes
 * 2. For each task in the range, calls set_reachability() with the marker
 * 3. The marker ensures shared dependencies are not counted multiple times
 *
 * @tparam TaskRange A range type containing pointers to tasks (Task*, ITask*, etc.)
 * @param tasks A range of task pointers
 *
 * @note Must be called before task execution to enable proper topological sorting.
 *
 * Usage:
 * @code
 * std::vector<ITask*> tasks = {&task1, &task2, &task3};
 * tw::compute_reachability(tasks);
 * // Now tasks can be sorted by reachability and executed
 * @endcode
 */
template<typename TaskRange>
auto compute_reachability(TaskRange&& tasks) noexcept -> void
{
    INode::TraverseMarkerT marker;
    for (auto* task : tasks) {
        if (task) {
            task->as_node()->set_reachability(marker);
        }
    }
}

} // namespace tw
