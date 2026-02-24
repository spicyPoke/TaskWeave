// Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "TaskWeave/Edge.h"
#include "TaskWeave/Node.h"
#include "TaskWeave/Task.h"

#include <gtest/gtest.h>
#include <thread>

namespace tw::test {

// Test Task with return type and no dependencies
TEST(TaskTest, NoDependencies)
{
    Task<int> task;
    task.set_callable([]() -> int {
        return 42;
    });

    EXPECT_EQ(task.get_state(), TaskState::Incomplete);

    task.run();

    EXPECT_EQ(task.get_state(), TaskState::Complete);
    EXPECT_EQ(task.get_result(), 42);
}

// Test Task with return type and dependencies
TEST(TaskTest, WithDependencies)
{
    // Create producer task
    Task<int> producer;
    producer.set_callable([]() -> int {
        return 100;
    });

    // Create consumer task that depends on producer
    Task<int, int> consumer;
    consumer.set_callable([](int value) -> int {
        return value * 2;
    });

    // Connect producer to consumer
    auto producer_edge = producer.get_outward_edge();
    consumer.add_inward_edge<int>(producer_edge);

    // Run producer first
    producer.run();

    // Now run consumer (it should get the value from producer)
    consumer.run();

    EXPECT_EQ(consumer.get_state(), TaskState::Complete);
    EXPECT_EQ(consumer.get_result(), 200);
}

// Test Task wait
TEST(TaskTest, Wait)
{
    Task<int> task;
    std::atomic<bool> started{false};
    task.set_callable([&started]() -> int {
        started = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return 42;
    });

    // Start task in a thread
    std::thread runner([&task]() {
        task.run();
    });

    // Wait for completion
    auto state = task.wait();

    EXPECT_EQ(state, TaskState::Complete);
    EXPECT_EQ(task.get_result(), 42);

    runner.join();
}

// Test Task as_node
TEST(TaskTest, AsNode)
{
    Task<int> task;

    const INode* node = task.as_node();
    EXPECT_NE(node, nullptr);
    EXPECT_EQ(node, static_cast<const INode*>(&task));
}

// Test Task operator<
TEST(TaskTest, LessThanOperator)
{
    Task<int> task1;
    task1.set_callable([]() -> int {
        return 1;
    });

    Task<int, int> task2;
    task2.set_callable([](int) -> int {
        return 2;
    });

    auto edge = task1.get_outward_edge();
    task2.add_inward_edge<int>(edge);

    task2.set_reachability();

    EXPECT_LT(task1, task2);
}

// Test Task with void return type
TEST(TaskVoidTest, NoDependencies)
{
    Task<void> task;
    int result = 0;
    task.set_callable([&result]() {
        result = 42;
    });

    EXPECT_EQ(task.get_state(), TaskState::Incomplete);

    task.run();

    EXPECT_EQ(task.get_state(), TaskState::Complete);
    EXPECT_EQ(result, 42);
}

// Test Task with void return type and dependencies
TEST(TaskVoidTest, WithDependencies)
{
    Task<int> producer;
    producer.set_callable([]() -> int {
        return 100;
    });

    Task<void, int> consumer;
    int consumed_value = 0;
    consumer.set_callable([&consumed_value](int value) {
        consumed_value = value;
    });

    auto producer_edge = producer.get_outward_edge();
    consumer.add_inward_edge<int>(producer_edge);

    producer.run();
    consumer.run();

    EXPECT_EQ(consumer.get_state(), TaskState::Complete);
    EXPECT_EQ(consumed_value, 100);
}

// Test Task void wait
TEST(TaskVoidTest, Wait)
{
    Task<void> task;
    bool completed = false;
    task.set_callable([&completed]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        completed = true;
    });

    std::thread runner([&task]() {
        task.run();
    });

    auto state = task.wait();
    EXPECT_EQ(state, TaskState::Complete);
    EXPECT_TRUE(completed);

    runner.join();
}

// Test Task with multiple dependencies
TEST(TaskTest, MultipleDependencies)
{
    Task<int> producer1;
    producer1.set_callable([]() -> int {
        return 10;
    });

    Task<double> producer2;
    producer2.set_callable([]() -> double {
        return 3.14;
    });

    Task<int, int, double> consumer;
    consumer.set_callable([](int a, double b) -> int {
        return static_cast<int>(a + b);
    });

    consumer.add_inward_edge<int>(producer1.get_outward_edge());
    consumer.add_inward_edge<double>(producer2.get_outward_edge());

    producer1.run();
    producer2.run();
    consumer.run();

    EXPECT_EQ(consumer.get_state(), TaskState::Complete);
    EXPECT_EQ(producer1.get_result(), 10);
    EXPECT_DOUBLE_EQ(producer2.get_result(), 3.14);
    EXPECT_EQ(consumer.get_result(), 13);
}

// Test Task set_callable with concept validation
TEST(TaskTest, SetCallableWithValidPrototype)
{
    Task<int, int, double> task;

    // Use std::move to pass lambda by value (avoids reference issues with function_traits)
    task.set_callable([](int a, double b) -> int {
        return a + static_cast<int>(b);
    });

    // Add dependencies and run
    Task<int> dep1;
    dep1.set_callable([]() -> int {
        return 5;
    });

    Task<double> dep2;
    dep2.set_callable([]() -> double {
        return 3.14;
    });

    task.add_inward_edge<int>(dep1.get_outward_edge());
    task.add_inward_edge<double>(dep2.get_outward_edge());

    dep1.run();
    dep2.run();
    task.run();

    EXPECT_EQ(task.get_result(), 8);
}

// Test Task name and description (inherited from ITask)
// Note: set_name and set_description are protected, so we can only test getters
TEST(TaskTest, NameAndDescription)
{
    Task<int> task;
    // Name and description setters are protected - accessible only to derived classes
    // The getters return empty strings by default
    EXPECT_EQ(task.get_name(), "");
    EXPECT_EQ(task.get_description(), "");
}

// Test Task duration
TEST(TaskTest, Duration)
{
    Task<int> task;
    task.set_callable([]() -> int {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return 42;
    });

    task.run();

    auto duration = task.get_duration<std::chrono::milliseconds>();
    EXPECT_GE(duration.count(), 50);
}

} // namespace tw::test
