// Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "TaskWeave/Edge.h"
#include "TaskWeave/Node.h"
#include "TaskWeave/Task.h"

#include <gtest/gtest.h>

namespace tw::test {

// Test basic Node construction
TEST(NodeTest, DefaultConstruction)
{
    Node<int> node;

    EXPECT_EQ(node.get_inward_edges_count(), 0);
    EXPECT_EQ(node.get_reachability(), 0);
    EXPECT_NE(node.get_outward_edge(), nullptr);
    EXPECT_FALSE(node.get_outward_edge()->is_retrievable());
}

TEST(NodeTest, VoidDefaultConstruction)
{
    Node<void> node;

    EXPECT_EQ(node.get_inward_edges_count(), 0);
    EXPECT_EQ(node.get_reachability(), 0);
}

// Test Node with single input
TEST(NodeTest, SingleInput)
{
    Node<int, int> node;

    EXPECT_EQ(node.get_inward_edges_count(), 1);

    Edge<int> edge(&node);
    edge.set_data(42);

    node.add_inward_edge<int>(&edge);

    auto edges = node.get_inward_edges();
    EXPECT_EQ(edges.size(), 1);
    EXPECT_EQ(edges[0], &edge);
}

// Test Node with multiple inputs
TEST(NodeTest, MultipleInputs)
{
    Node<int, int, double, float> node;

    EXPECT_EQ(node.get_inward_edges_count(), 3);

    Edge<int> edge1(&node);
    Edge<double> edge2(&node);
    Edge<float> edge3(&node);

    edge1.set_data(42);
    edge2.set_data(3.14);
    edge3.set_data(2.71f);

    node.add_inward_edge<int>(&edge1);
    node.add_inward_edge<double>(&edge2);
    node.add_inward_edge<float>(&edge3);

    auto edges = node.get_inward_edges();
    EXPECT_EQ(edges.size(), 3);
}

// Test get_inward_edge_value by type
TEST(NodeTest, GetInwardEdgeValueByType)
{
    Node<int, int, double> node;

    Edge<int> edge1(&node);
    Edge<double> edge2(&node);

    edge1.set_data(100);
    edge2.set_data(2.5);

    node.add_inward_edge<int>(&edge1);
    node.add_inward_edge<double>(&edge2);

    EXPECT_EQ(node.get_inward_edge_value<int>(), 100);
    EXPECT_DOUBLE_EQ(node.get_inward_edge_value<double>(), 2.5);
}

// Test get_inward_edge_value by index
TEST(NodeTest, GetInwardEdgeValueByIndex)
{
    Node<int, int, double> node;

    Edge<int> edge1(&node);
    Edge<double> edge2(&node);

    edge1.set_data(100);
    edge2.set_data(2.5);

    node.add_inward_edge<0, int>(&edge1);
    node.add_inward_edge<1, double>(&edge2);

    EXPECT_EQ(node.get_inward_edge_value<0>(), 100);
    EXPECT_DOUBLE_EQ(node.get_inward_edge_value<1>(), 2.5);
}

// Test set_out_edge_data through Task
TEST(NodeTest, SetOutEdgeData)
{
    Task<int> task;
    task.set_callable([]() -> int {
        return 42;
    });
    task.run();

    auto out_edge = task.get_outward_edge();
    EXPECT_TRUE(out_edge->is_retrievable());
    EXPECT_EQ(out_edge->get_data(), 42);
}

TEST(NodeTest, SetOutEdgeDataVoid)
{
    Task<void> task;
    task.set_callable([]() {
    });
    task.run();

    auto out_edge = task.get_outward_edge();
    EXPECT_TRUE(out_edge->is_retrievable());
}

// Test reachability
TEST(NodeTest, ReachabilityNoDependencies)
{
    Task<int> task;
    task.set_callable([]() -> int {
        return 1;
    });
    task.run();

    EXPECT_EQ(task.get_reachability(), 0);
}

TEST(NodeTest, ReachabilityWithDependencies)
{
    // Create a chain: node1 -> node2 -> node3
    Task<int> task1;
    task1.set_callable([]() -> int {
        return 1;
    });

    Task<int, int> task2;
    task2.set_callable([](int v) -> int {
        return v + 1;
    });

    Task<int, int> task3;
    task3.set_callable([](int v) -> int {
        return v + 1;
    });

    // Connect tasks
    task2.add_inward_edge<int>(task1.get_outward_edge());
    task3.add_inward_edge<int>(task2.get_outward_edge());

    task3.set_reachability();

    EXPECT_EQ(task2.get_reachability(), 1);
    EXPECT_EQ(task3.get_reachability(), 2);
}

TEST(NodeTest, ReachabilityWithTraverseMarker)
{
    Node<int> node1;
    Node<int, int> node2;

    Edge<int> edge(&node1);
    node2.add_inward_edge<int>(&edge);

    INode::TraverseMarkerT marker;

    node1.set_reachability(marker);
    node2.set_reachability(marker);

    // node1 has no dependencies (reachability 0 for leaf nodes)
    // node2 depends on node1 (reachability 1)
    EXPECT_EQ(node1.get_reachability(), 0);
    EXPECT_EQ(node2.get_reachability(), 1);
}

// Test operator<
TEST(NodeTest, LessThanOperator)
{
    Task<int> task1;
    task1.set_callable([]() -> int {
        return 1;
    });

    Task<int, int> task2;
    task2.set_callable([](int v) -> int {
        return v + 1;
    });

    task2.add_inward_edge<int>(task1.get_outward_edge());

    task2.set_reachability();

    // task1 should have lower reachability than task2
    EXPECT_LT(task1, task2);
}

// Test contained_no_duplicate_types
TEST(ContainedNoDuplicateTypesTest, NoDuplicates)
{
    static_assert(contained_no_duplicate_types<std::tuple<int, double, float>>);
}

TEST(ContainedNoDuplicateTypesTest, WithDuplicates)
{
    static_assert(!contained_no_duplicate_types<std::tuple<int, double, int>>);
}

TEST(ContainedNoDuplicateTypesTest, EmptyTuple)
{
    static_assert(contained_no_duplicate_types<std::tuple<>>);
}

TEST(ContainedNoDuplicateTypesTest, SingleType)
{
    static_assert(contained_no_duplicate_types<std::tuple<int>>);
}

} // namespace tw::test
