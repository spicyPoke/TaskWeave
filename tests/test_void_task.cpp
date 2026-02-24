// Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "Executor/ThreadPoolExecutor.h"
#include "TaskWeave/Task.h"

#include <gtest/gtest.h>

namespace tw::test {

// Test Task<void, void> - a task that takes void input and returns void
TEST(VoidTaskTest, TaskVoidVoid_Basic)
{
    Task<void, void> task;
    int counter = 0;
    task.set_callable([&counter]() {
        counter++;
    });

    // Task should be able to run
    task.run();
    task.wait();

    EXPECT_EQ(counter, 1);
    EXPECT_EQ(task.get_state(), TaskState::Complete);
}

// Test Task<void, void> with dependency on another Task<void>
TEST(VoidTaskTest, TaskVoidVoid_WithDependency)
{
    Task<void> producer;
    Task<void, void> consumer;

    int producer_counter = 0;
    int consumer_counter = 0;

    producer.set_callable([&producer_counter]() {
        producer_counter++;
    });

    consumer.set_callable([&consumer_counter]() {
        consumer_counter++;
    });

    // Connect producer to consumer
    consumer.add_inward_edge<void>(producer.get_outward_edge());

    ThreadPoolExecutor executor;
    executor.add_task(&producer);
    executor.add_task(&consumer);
    executor.run();
    executor.wait();

    EXPECT_EQ(producer_counter, 1);
    EXPECT_EQ(consumer_counter, 1);
}

// Test chain of Task<void, void>
TEST(VoidTaskTest, TaskVoidVoid_Chain)
{
    Task<void> start;
    Task<void, void> middle1;
    Task<void, void> middle2;
    Task<void, void> end;

    int counter = 0;

    start.set_callable([&counter]() {
        counter = 1;
    });

    middle1.set_callable([&counter]() {
        counter = 2;
    });

    middle2.set_callable([&counter]() {
        counter = 3;
    });

    end.set_callable([&counter]() {
        counter = 4;
    });

    // Create chain: start -> middle1 -> middle2 -> end
    middle1.add_inward_edge<void>(start.get_outward_edge());
    middle2.add_inward_edge<void>(middle1.get_outward_edge());
    end.add_inward_edge<void>(middle2.get_outward_edge());

    ThreadPoolExecutor executor;
    executor.add_task(&start);
    executor.add_task(&middle1);
    executor.add_task(&middle2);
    executor.add_task(&end);
    executor.run();
    executor.wait();

    EXPECT_EQ(counter, 4);
}

// Test multiple dependencies on Task<void, void>
TEST(VoidTaskTest, TaskVoidVoid_MultipleDependencies)
{
    Task<void> dep1;
    Task<void> dep2;
    Task<void, void, void> consumer;

    int dep1_counter = 0;
    int dep2_counter = 0;
    int consumer_counter = 0;

    dep1.set_callable([&dep1_counter]() {
        dep1_counter++;
    });

    dep2.set_callable([&dep2_counter]() {
        dep2_counter++;
    });

    consumer.set_callable([&consumer_counter]() {
        consumer_counter++;
    });

    // Connect both dependencies to consumer using indexed version
    // (needed because both inputs are the same type: void)
    consumer.add_inward_edge<0>(dep1.get_outward_edge());
    consumer.add_inward_edge<1>(dep2.get_outward_edge());

    ThreadPoolExecutor executor;
    executor.add_task(&dep1);
    executor.add_task(&dep2);
    executor.add_task(&consumer);
    executor.run();
    executor.wait();

    EXPECT_EQ(dep1_counter, 1);
    EXPECT_EQ(dep2_counter, 1);
    EXPECT_EQ(consumer_counter, 1);
}

} // namespace tw::test
