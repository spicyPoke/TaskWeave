// Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "TaskWeave/Edge.h"
#include "TaskWeave/Node.h"

#include <gtest/gtest.h>

namespace tw::test {

// Test Edge with data
TEST(EdgeTest, SetAndGet)
{
    Node<int> node;
    Edge<int> edge(&node);

    EXPECT_FALSE(edge.is_retrievable());

    edge.set_data(42);

    EXPECT_TRUE(edge.is_retrievable());
    EXPECT_EQ(edge.get_data(), 42);
}

TEST(EdgeTest, SetDataOverwrites)
{
    Node<int> node;
    Edge<int> edge(&node);

    edge.set_data(100);
    EXPECT_EQ(edge.get_data(), 100);

    edge.set_data(200);
    EXPECT_EQ(edge.get_data(), 200);
}

TEST(EdgeTest, GetOwner)
{
    Node<int> node;
    Edge<int> edge(&node);

    EXPECT_EQ(edge.get_owner(), &node);
}

TEST(EdgeTest, WaitUntilRetrievable)
{
    Node<int> node;
    Edge<int> edge(&node);

    // Start a thread that will set data after a delay
    std::thread setter([&edge]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        edge.set_data(99);
    });

    // This should block until data is set
    edge.wait_until_retrievable();

    EXPECT_TRUE(edge.is_retrievable());
    EXPECT_EQ(edge.get_data(), 99);

    setter.join();
}

// Test Edge<void> specialization
TEST(EdgeVoidTest, SetData)
{
    Node<void> node;
    Edge<void> edge(&node);

    EXPECT_FALSE(edge.is_retrievable());

    edge.set_data();

    EXPECT_TRUE(edge.is_retrievable());
}

TEST(EdgeVoidTest, GetOwner)
{
    Node<void> node;
    Edge<void> edge(&node);

    EXPECT_EQ(edge.get_owner(), &node);
}

TEST(EdgeVoidTest, WaitUntilRetrievable)
{
    Node<void> node;
    Edge<void> edge(&node);

    // Start a thread that will set data after a delay
    std::thread setter([&edge]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        edge.set_data();
    });

    // This should block until data is set
    edge.wait_until_retrievable();

    EXPECT_TRUE(edge.is_retrievable());

    setter.join();
}
} // namespace tw::test
