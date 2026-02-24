// Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "TaskWeave/Task.h"
#include "TaskWeave/Helper.h"
#include "Executor/ThreadPoolExecutor.h"

#include <gtest/gtest.h>

// Test that reachability is computed automatically by ThreadPoolExecutor
TEST(AutoReachabilityTest, SimpleProducerConsumer)
{
    tw::ThreadPoolExecutor executor;

    tw::Task<int> producer;
    producer.set_callable([]() -> int {
        return 42;
    });

    tw::Task<int, int> consumer;
    consumer.set_callable([](int value) -> int {
        return value * 2;
    });

    // Connect producer to consumer
    consumer.add_inward_edge<int>(producer.get_outward_edge());

    // Add tasks WITHOUT calling set_reachability()
    executor.add_task(&producer);
    executor.add_task(&consumer);

    // Run - reachability should be computed automatically
    executor.run();
    executor.wait();

    EXPECT_EQ(producer.get_result(), 42);
    EXPECT_EQ(consumer.get_result(), 84);
}

// Test auto reachability with linear chain of tasks
TEST(AutoReachabilityTest, LinearChain)
{
    tw::ThreadPoolExecutor executor;

    // Create a chain: T0 -> T1 -> T2 -> T3 -> T4
    auto t0 = std::make_unique<tw::Task<int>>();
    t0->set_callable([]() -> int { return 1; });

    auto t1 = std::make_unique<tw::Task<int, int>>();
    t1->set_callable([](int prev) -> int { return prev + 1; });
    t1->add_inward_edge<int>(t0->get_outward_edge());

    auto t2 = std::make_unique<tw::Task<int, int>>();
    t2->set_callable([](int prev) -> int { return prev + 1; });
    t2->add_inward_edge<int>(t1->get_outward_edge());

    auto t3 = std::make_unique<tw::Task<int, int>>();
    t3->set_callable([](int prev) -> int { return prev + 1; });
    t3->add_inward_edge<int>(t2->get_outward_edge());

    auto t4 = std::make_unique<tw::Task<int, int>>();
    t4->set_callable([](int prev) -> int { return prev + 1; });
    t4->add_inward_edge<int>(t3->get_outward_edge());

    // Add all tasks WITHOUT calling set_reachability()
    executor.add_task(t0.get());
    executor.add_task(t1.get());
    executor.add_task(t2.get());
    executor.add_task(t3.get());
    executor.add_task(t4.get());

    executor.run();
    executor.wait();

    // Final result should be 5 (1 + 1 + 1 + 1 + 1)
    EXPECT_EQ(t4->get_result(), 5);
}

// Test auto reachability with fan-out pattern
TEST(AutoReachabilityTest, FanOut)
{
    tw::ThreadPoolExecutor executor;

    // Producer
    auto producer = std::make_unique<tw::Task<int>>();
    producer->set_callable([]() -> int {
        return 100;
    });

    // Consumers
    std::vector<tw::Task<int, int>> consumers(5);
    for (int i = 0; i < 5; ++i) {
        consumers[i].set_callable([i](int value) -> int {
            return value + i;
        });
        consumers[i].add_inward_edge<int>(producer->get_outward_edge());
        executor.add_task(&consumers[i]);
    }

    executor.add_task(producer.get());
    executor.run();
    executor.wait();

    EXPECT_EQ(producer->get_result(), 100);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(consumers[i].get_result(), 100 + i);
    }
}

// Test auto reachability with fan-in pattern
TEST(AutoReachabilityTest, FanIn)
{
    tw::ThreadPoolExecutor executor;

    // Producers
    std::vector<tw::Task<int>> producers(5);
    for (int i = 0; i < 5; ++i) {
        producers[i].set_callable([i]() -> int {
            return i + 1;
        });
        executor.add_task(&producers[i]);
    }

    // Consumer that sums all inputs
    tw::Task<int, int, int, int, int, int> consumer;
    consumer.set_callable([](int a, int b, int c, int d, int e) -> int {
        return a + b + c + d + e;
    });

    consumer.add_inward_edge<0, int>(producers[0].get_outward_edge());
    consumer.add_inward_edge<1, int>(producers[1].get_outward_edge());
    consumer.add_inward_edge<2, int>(producers[2].get_outward_edge());
    consumer.add_inward_edge<3, int>(producers[3].get_outward_edge());
    consumer.add_inward_edge<4, int>(producers[4].get_outward_edge());

    executor.add_task(&consumer);
    executor.run();
    executor.wait();

    // Sum of 1+2+3+4+5 = 15
    EXPECT_EQ(consumer.get_result(), 15);
}

// Test auto reachability with diamond pattern
TEST(AutoReachabilityTest, DiamondPattern)
{
    tw::ThreadPoolExecutor executor;

    // Top of diamond
    tw::Task<int> top;
    top.set_callable([]() -> int {
        return 10;
    });

    // Middle tasks
    tw::Task<int, int> left;
    left.set_callable([](int value) -> int {
        return value * 2;
    });

    tw::Task<int, int> right;
    right.set_callable([](int value) -> int {
        return value * 3;
    });

    left.add_inward_edge<int>(top.get_outward_edge());
    right.add_inward_edge<int>(top.get_outward_edge());

    // Bottom of diamond - use indexed add_inward_edge for duplicate types
    tw::Task<int, int, int> bottom;
    bottom.set_callable([](int l, int r) -> int {
        return l + r;
    });

    bottom.add_inward_edge<0, int>(left.get_outward_edge());
    bottom.add_inward_edge<1, int>(right.get_outward_edge());

    executor.add_task(&top);
    executor.add_task(&left);
    executor.add_task(&right);
    executor.add_task(&bottom);

    executor.run();
    executor.wait();

    // top = 10, left = 20, right = 30, bottom = 50
    EXPECT_EQ(top.get_result(), 10);
    EXPECT_EQ(left.get_result(), 20);
    EXPECT_EQ(right.get_result(), 30);
    EXPECT_EQ(bottom.get_result(), 50);
}

// Test manual compute_reachability function
TEST(AutoReachabilityTest, ManualComputeReachability)
{
    tw::Task<int> task1;
    task1.set_callable([]() -> int { return 1; });

    tw::Task<int, int> task2;
    task2.set_callable([](int v) -> int { return v + 1; });

    task2.add_inward_edge<int>(task1.get_outward_edge());

    // Manually compute reachability before adding to executor
    std::vector<tw::ITask*> tasks{&task1, &task2};
    tw::compute_reachability(tasks);

    // Verify reachability was computed
    EXPECT_EQ(task1.as_node()->get_reachability(), 0);
    EXPECT_EQ(task2.as_node()->get_reachability(), 1);

    tw::ThreadPoolExecutor executor;
    executor.add_task(&task1);
    executor.add_task(&task2);

    executor.run();
    executor.wait();

    EXPECT_EQ(task1.get_result(), 1);
    EXPECT_EQ(task2.get_result(), 2);
}

// Test that existing set_reachability calls still work (backward compatibility)
TEST(AutoReachabilityTest, BackwardCompatibility)
{
    tw::Task<int> task1;
    task1.set_callable([]() -> int { return 42; });

    tw::Task<int, int> task2;
    task2.set_callable([](int v) -> int { return v * 2; });

    task2.add_inward_edge<int>(task1.get_outward_edge());

    // Old style: manually call set_reachability (should still work)
    task2.as_node()->set_reachability();

    tw::ThreadPoolExecutor executor;
    executor.add_task(&task1);
    executor.add_task(&task2);

    executor.run();
    executor.wait();

    EXPECT_EQ(task1.get_result(), 42);
    EXPECT_EQ(task2.get_result(), 84);
}
