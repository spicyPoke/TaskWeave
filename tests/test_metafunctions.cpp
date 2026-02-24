// Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "TaskWeave/Metafunctions.h"

#include <functional>
#include <gtest/gtest.h>
#include <string>

namespace tw::test {

// Test fixtures for function traits
TEST(FunctionTraitsTest, FunctionPointer)
{
    using traits = function_traits<int(int, double)>;
    static_assert(traits::arity == 2);
    static_assert(std::is_same_v<typename traits::return_type, int>);
    static_assert(std::is_same_v<typename traits::template arg<0>::type, int>);
    static_assert(std::is_same_v<typename traits::template arg<1>::type, double>);
}

TEST(FunctionTraitsTest, FunctionPointerWithPointer)
{
    using traits = function_traits<int (*)(int, double)>;
    static_assert(traits::arity == 2);
    static_assert(std::is_same_v<typename traits::return_type, int>);
}

TEST(FunctionTraitsTest, FunctionReference)
{
    using traits = function_traits<int (&)(int, double)>;
    static_assert(traits::arity == 2);
    static_assert(std::is_same_v<typename traits::return_type, int>);
}

TEST(FunctionTraitsTest, Lambda)
{
    auto lambda = [](int x, double y) -> int {
        return x + static_cast<int>(y);
    };
    using traits = function_traits<decltype(lambda)>;
    static_assert(traits::arity == 2);
    static_assert(std::is_same_v<typename traits::return_type, int>);
    static_assert(std::is_same_v<typename traits::template arg<0>::type, int>);
    static_assert(std::is_same_v<typename traits::template arg<1>::type, double>);
}

TEST(FunctionTraitsTest, ConstMemberFunction)
{
    struct Foo {
        int bar(int x) const
        {
            return x;
        }
    };
    using traits = function_traits<int (Foo::*)(int) const>;
    static_assert(traits::arity == 1);
    static_assert(std::is_same_v<typename traits::return_type, int>);
    static_assert(std::is_same_v<typename traits::class_type, Foo>);
}

TEST(FunctionTraitsTest, NonConstMemberFunction)
{
    struct Foo {
        int bar(int x)
        {
            return x;
        }
    };
    using traits = function_traits<int (Foo::*)(int)>;
    static_assert(traits::arity == 1);
    static_assert(std::is_same_v<typename traits::return_type, int>);
    static_assert(std::is_same_v<typename traits::class_type, Foo>);
}

TEST(FunctionTraitsTest, VoidReturn)
{
    using traits = function_traits<void(int, double)>;
    static_assert(traits::arity == 2);
    static_assert(std::is_same_v<typename traits::return_type, void>);
}

TEST(FunctionTraitsTest, NoArguments)
{
    using traits = function_traits<int()>;
    static_assert(traits::arity == 0);
    static_assert(std::is_same_v<typename traits::return_type, int>);
}

// Test function_has_the_required_prototype_v
TEST(FunctionHasRequiredPrototypeTest, ValidLambda)
{
    auto lambda = [](int x, double y) -> int {
        return x + static_cast<int>(y);
    };
    static_assert(function_has_the_required_prototype_v<decltype(lambda), int, int, double>);
}

TEST(FunctionHasRequiredPrototypeTest, InvalidReturnType)
{
    auto lambda = [](int x, double y) -> int {
        return x + static_cast<int>(y);
    };
    static_assert(!function_has_the_required_prototype_v<decltype(lambda), double, int, double>);
}

TEST(FunctionHasRequiredPrototypeTest, InvalidArgType)
{
    auto lambda = [](int x, double y) -> int {
        return x + static_cast<int>(y);
    };
    static_assert(!function_has_the_required_prototype_v<decltype(lambda), int, int, int>);
}

TEST(FunctionHasRequiredPrototypeTest, FunctionPointer)
{
    auto func = [](int, double) -> int {
        return 0;
    };
    static_assert(function_has_the_required_prototype_v<decltype(func), int, int, double>);
}

// Test the concept
TEST(HasRequiredPrototypeConceptTest, ValidFunction)
{
    auto lambda = [](int x, double y) -> int {
        return x + static_cast<int>(y);
    };
    static_assert(has_required_prototype<decltype(lambda), int, int, double>);
}

TEST(HasRequiredPrototypeConceptTest, InvalidFunction)
{
    auto lambda = [](int x, double y) -> int {
        return x + static_cast<int>(y);
    };
    static_assert(!has_required_prototype<decltype(lambda), double, int, double>);
}

// Test function_has_the_required_prototype_v2 (tuple version)
TEST(FunctionHasRequiredPrototype2Test, ValidLambda)
{
    auto lambda = [](int x, double y) -> int {
        return x + static_cast<int>(y);
    };
    static_assert(function_has_the_required_prototype_v2<decltype(lambda), int, std::tuple<int, double>>);
}

TEST(FunctionHasRequiredPrototype2Test, InvalidReturnType)
{
    auto lambda = [](int x, double y) -> int {
        return x + static_cast<int>(y);
    };
    static_assert(!function_has_the_required_prototype_v2<decltype(lambda), double, std::tuple<int, double>>);
}

TEST(FunctionHasRequiredPrototype2Test, InvalidArgType)
{
    auto lambda = [](int x, double y) -> int {
        return x + static_cast<int>(y);
    };
    static_assert(!function_has_the_required_prototype_v2<decltype(lambda), int, std::tuple<int, int>>);
}

// Test has_required_prototype2 concept
TEST(HasRequiredPrototype2ConceptTest, ValidFunction)
{
    auto lambda = [](int x, double y) -> int {
        return x + static_cast<int>(y);
    };
    static_assert(has_required_prototype2<decltype(lambda), int, std::tuple<int, double>>);
}

TEST(HasRequiredPrototype2ConceptTest, InvalidFunction)
{
    auto lambda = [](int x, double y) -> int {
        return x + static_cast<int>(y);
    };
    static_assert(!has_required_prototype2<decltype(lambda), double, std::tuple<int, double>>);
}

// Test as_false
TEST(AsFalseTest, AlwaysFalse)
{
    static_assert(!as_false_v<int>);
    static_assert(!as_false_v<void>);
    static_assert(!as_false_v<std::string>);
}

// Test filter_void
TEST(FilterVoidTest, NonVoidType)
{
    using result = filter_void<int>;
    static_assert(std::is_same_v<result, std::tuple<int>>);
}

TEST(FilterVoidTest, VoidType)
{
    using result = filter_void<void>;
    static_assert(std::is_same_v<result, std::tuple<>>);
}

// Test remove_voids
TEST(RemoveVoidsTest, NoVoids)
{
    using result = remove_voids<int, double, float>;
    static_assert(std::is_same_v<result, std::tuple<int, double, float>>);
}

TEST(RemoveVoidsTest, WithVoids)
{
    using result = remove_voids<int, void, double, void, float>;
    static_assert(std::is_same_v<result, std::tuple<int, double, float>>);
}

TEST(RemoveVoidsTest, AllVoids)
{
    using result = remove_voids<void, void, void>;
    static_assert(std::is_same_v<result, std::tuple<>>);
}

// Test function_from_tuple
TEST(FunctionFromTupleTest, BasicFunction)
{
    using result = function_from_tuple<int, std::tuple<int, double>>::type;
    static_assert(std::is_same_v<result, std::function<int(int, double)>>);
}

TEST(FunctionFromTupleTest, VoidReturn)
{
    using result = function_from_tuple<void, std::tuple<int, double>>::type;
    static_assert(std::is_same_v<result, std::function<void(int, double)>>);
}

TEST(FunctionFromTupleTest, NoArguments)
{
    using result = function_from_tuple<int, std::tuple<>>::type;
    static_assert(std::is_same_v<result, std::function<int()>>);
}

// Test integer_sequence_void_filter
TEST(IntegerSequenceVoidFilterTest, NoVoids)
{
    using filter = integer_sequence_void_filter<int, double, float>;
    using filtered = filter::filtered;
    static_assert(std::is_same_v<filtered, std::index_sequence<0, 1, 2>>);
}

TEST(IntegerSequenceVoidFilterTest, WithVoids)
{
    using filter = integer_sequence_void_filter<int, void, double, void, float>;
    using filtered = filter::filtered;
    static_assert(std::is_same_v<filtered, std::index_sequence<0, 2, 4>>);
}

TEST(IntegerSequenceVoidFilterTest, AllVoids)
{
    using filter = integer_sequence_void_filter<void, void, void>;
    using filtered = filter::filtered;
    static_assert(std::is_same_v<filtered, std::index_sequence<>>);
}

} // namespace tw::test
