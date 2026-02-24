// Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "../src/TaskWeave/Metafunctions.h"
#include "../src/TaskWeave/Task.h"

#include <functional>
#include <gtest/gtest.h>
#include <string>

// Test basic function traits
TEST(FunctionTraitsTest, BasicFunctionArity)
{
    static_assert(tw::function_traits<int(int, double)>::arity == 2);
}

TEST(FunctionTraitsTest, BasicFunctionReturnType)
{
    static_assert(std::is_same_v<typename tw::function_traits<int(int, double)>::return_type, int>);
}

TEST(FunctionTraitsTest, BasicFunctionArg0Type)
{
    static_assert(std::is_same_v<typename tw::function_traits<int(int, double)>::template arg<0>::type, int>);
}

TEST(FunctionTraitsTest, BasicFunctionArg1Type)
{
    static_assert(std::is_same_v<typename tw::function_traits<int(int, double)>::template arg<1>::type, double>);
}

// Test function pointer traits
TEST(FunctionTraitsTest, FunctionPointerArity)
{
    static_assert(tw::function_traits<int (*)(int, double)>::arity == 2);
}

TEST(FunctionTraitsTest, FunctionPointerReturnType)
{
    static_assert(std::is_same_v<typename tw::function_traits<int (*)(int, double)>::return_type, int>);
}

// Test lambda traits
TEST(FunctionTraitsTest, LambdaFunctionHasRequiredPrototype)
{
    auto test_lambda = [](int x, double y) -> int {
        return x + static_cast<int>(y);
    };
    static_assert(tw::function_has_the_required_prototype_v<decltype(test_lambda), int, int, double>);
}

TEST(FunctionTraitsTest, LambdaFunctionWrongReturnType)
{
    auto test_lambda = [](int x, double y) -> int {
        return x + static_cast<int>(y);
    };
    static_assert(!tw::function_has_the_required_prototype_v<decltype(test_lambda), double, int, double>);
}

TEST(FunctionTraitsTest, LambdaFunctionWrongArgType)
{
    auto test_lambda = [](int x, double y) -> int {
        return x + static_cast<int>(y);
    };
    static_assert(!tw::function_has_the_required_prototype_v<decltype(test_lambda), int, int, int>);
}

// Test the concept
TEST(FunctionTraitsTest, ConceptHasRequiredPrototypeValid)
{
    auto test_lambda = [](int x, double y) -> int {
        return x + static_cast<int>(y);
    };
    static_assert(tw::has_required_prototype<decltype(test_lambda), int, int, double>);
}

TEST(FunctionTraitsTest, ConceptHasRequiredPrototypeInvalidReturnType)
{
    auto test_lambda = [](int x, double y) -> int {
        return x + static_cast<int>(y);
    };
    static_assert(!tw::has_required_prototype<decltype(test_lambda), double, int, double>);
}

// Test Task class with valid function
TEST(TaskTest, ValidFunctionPrototype)
{
    int i = 9;

    auto add_func = [i](int a, int b) -> int {
        return i + a + b;
    };

    tw::Task<int, int, int> task;

    if constexpr (tw::has_required_prototype<decltype(add_func), int, int, int>) {
        task.set_callable(add_func);
        SUCCEED() << "Successfully set callable with correct prototype";
    }
    else {
        FAIL() << "Function prototype should match task requirements";
    }
}

// Test that invalid function prototype is rejected at compile time
// This test verifies that the concept properly rejects mismatched signatures
TEST(TaskTest, InvalidFunctionPrototypeRejected)
{
    auto wrong_func = [](int a, double b) -> int {
        return a + static_cast<int>(b);
    };

    // Verify that the concept correctly identifies the mismatch
    static_assert(!tw::has_required_prototype<decltype(wrong_func), int, int, int>);
    SUCCEED() << "Invalid function prototype correctly rejected at compile time";
}
