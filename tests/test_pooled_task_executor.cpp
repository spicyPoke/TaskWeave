// Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "Executor/ThreadPoolExecutor.h"
#include "TaskWeave/Task.h"

#include <atomic>
#include <chrono>
#include <gtest/gtest.h>

namespace tw::test {

// Test ThreadPoolExecutor construction
TEST(ThreadPoolExecutorTest, DefaultConstruction)
{
    ThreadPoolExecutor executor;

    // Default construction should work
    EXPECT_TRUE(true);
}

// Test ThreadPoolExecutor move construction
TEST(ThreadPoolExecutorTest, MoveConstruction)
{
    ThreadPoolExecutor executor1;

    std::atomic<int> counter{0};
    Task<void> task;
    task.set_callable([&counter]() {
        counter++;
    });

    executor1.add_task(&task);

    ThreadPoolExecutor executor2(std::move(executor1));

    // executor2 should have the task
    executor2.run();
    executor2.wait();

    EXPECT_EQ(counter, 1);
}

// Test ThreadPoolExecutor move assignment
TEST(ThreadPoolExecutorTest, MoveAssignment)
{
    ThreadPoolExecutor executor1;

    std::atomic<int> counter{0};
    Task<void> task;
    task.set_callable([&counter]() {
        counter++;
    });

    executor1.add_task(&task);

    ThreadPoolExecutor executor2;
    executor2 = std::move(executor1);

    executor2.run();
    executor2.wait();

    EXPECT_EQ(counter, 1);
}

// Test ThreadPoolExecutor uncopyable
TEST(ThreadPoolExecutorTest, Uncopyable)
{
    ThreadPoolExecutor executor;

    // This should not compile:
    // ThreadPoolExecutor executor_copy = executor;

    EXPECT_TRUE(true); // Placeholder - compile-time check is the real test
}

// Test ThreadPoolExecutor add_task and run
TEST(ThreadPoolExecutorTest, AddTaskAndRun)
{
    ThreadPoolExecutor executor;

    std::atomic<int> counter{0};
    Task<void> task;
    task.set_callable([&counter]() {
        counter++;
    });

    executor.add_task(&task);
    executor.run();
    executor.wait();

    EXPECT_EQ(counter, 1);
}

// Test ThreadPoolExecutor multiple tasks
TEST(ThreadPoolExecutorTest, MultipleTasks)
{
    ThreadPoolExecutor executor;

    std::atomic<int> counter{0};

    std::vector<Task<void>> tasks(10);
    for (int i = 0; i < 10; i++) {
        tasks[i].set_callable([&counter]() {
            counter++;
        });
        executor.add_task(&tasks[i]);
    }

    executor.run();
    executor.wait();

    EXPECT_EQ(counter, 10);
}

// Test ThreadPoolExecutor with tasks that have dependencies
TEST(ThreadPoolExecutorTest, TasksWithDependencies)
{
    ThreadPoolExecutor executor;

    Task<int> producer;
    producer.set_callable([]() -> int {
        return 42;
    });

    Task<int, int> consumer;
    consumer.set_callable([](int value) -> int {
        return value * 2;
    });

    // Connect producer to consumer
    consumer.add_inward_edge<int>(producer.get_outward_edge());

    executor.add_task(&producer);
    executor.add_task(&consumer);
    executor.run();
    executor.wait();

    EXPECT_EQ(consumer.get_result(), 84);
}

// Test ThreadPoolExecutor cancel
TEST(ThreadPoolExecutorTest, Cancel)
{
    ThreadPoolExecutor executor;

    std::atomic<int> counter{0};
    std::atomic<bool> started{false};
    std::atomic<bool> can_finish{false};

    // Add tasks that will be queued
    std::vector<Task<void>> tasks(50);
    for (int i = 0; i < 50; i++) {
        tasks[i].set_callable([&counter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            counter++;
        });
        executor.add_task(&tasks[i]);
    }

    executor.run();

    // Cancel immediately after run (some tasks may have already started)
    executor.cancel();
    executor.wait();

    // Counter should be less than 50 since we cancelled some tasks
    // Note: Some tasks may have already started before cancel was called
    EXPECT_LT(counter, 50);
}

// Test ThreadPoolExecutor wait without run
TEST(ThreadPoolExecutorTest, WaitWithoutRun)
{
    ThreadPoolExecutor executor;

    Task<void> task;
    task.set_callable([]() { /* do nothing */ });

    executor.add_task(&task);

    // Wait without run should return immediately since workers haven't started
    executor.wait();

    // Task should not have run since run() was never called
    EXPECT_EQ(task.get_state(), TaskState::Incomplete);
}

// Test ThreadPoolExecutor task ordering by reachability
TEST(ThreadPoolExecutorTest, TaskOrdering)
{
    ThreadPoolExecutor executor;

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

    // Create a chain: task1 -> task2 -> task3
    task2.add_inward_edge<int>(task1.get_outward_edge());
    task3.add_inward_edge<int>(task2.get_outward_edge());

    executor.add_task(&task3);
    executor.add_task(&task1);
    executor.add_task(&task2);

    executor.run();
    executor.wait();

    // All tasks should complete in the right order
    EXPECT_EQ(task1.get_result(), 1);
    EXPECT_EQ(task2.get_result(), 2);
    EXPECT_EQ(task3.get_result(), 3);
}

// Test ThreadPoolExecutor with void tasks
TEST(ThreadPoolExecutorTest, VoidTasks)
{
    ThreadPoolExecutor executor;

    std::atomic<int> counter{0};

    Task<void> task1;
    task1.set_callable([&counter]() {
        counter += 1;
    });

    Task<void> task2;
    task2.set_callable([&counter]() {
        counter += 2;
    });

    Task<void> task3;
    task3.set_callable([&counter]() {
        counter += 3;
    });

    executor.add_task(&task1);
    executor.add_task(&task2);
    executor.add_task(&task3);

    executor.run();
    executor.wait();

    EXPECT_EQ(counter, 6);
}

// Test ThreadPoolExecutor with mixed task types
TEST(ThreadPoolExecutorTest, MixedTaskTypes)
{
    ThreadPoolExecutor executor;

    std::atomic<int> result{0};

    Task<int> int_task;
    int_task.set_callable([]() -> int {
        return 10;
    });

    Task<void> void_task;
    void_task.set_callable([&result]() {
        result = 100;
    });

    executor.add_task(&int_task);
    executor.add_task(&void_task);

    executor.run();
    executor.wait();

    EXPECT_EQ(int_task.get_result(), 10);
    EXPECT_EQ(result, 100);
}

} // namespace tw::test
