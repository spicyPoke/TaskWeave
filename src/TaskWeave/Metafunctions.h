// Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

// STL
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace tw {

/**
 * @brief Traits to extract function signature components
 */
template<typename FnT>
struct function_traits : function_traits<decltype(&std::remove_cvref_t<FnT>::operator())> {};

// Specialization for function pointers
template<typename R, typename... Args>
struct function_traits<R (*)(Args...)> {
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
    static constexpr std::size_t arity = sizeof...(Args);

    template<std::size_t I>
    struct arg {
        static_assert(I < arity, "Index out of range");
        using type = std::tuple_element_t<I, args_tuple>;
    };
};

// Specialization for function references
template<typename R, typename... Args>
struct function_traits<R (&)(Args...)> {
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
    static constexpr std::size_t arity = sizeof...(Args);

    template<std::size_t I>
    struct arg {
        static_assert(I < arity, "Index out of range");
        using type = std::tuple_element_t<I, args_tuple>;
    };
};

// Specialization for function types (no pointer/reference)
template<typename R, typename... Args>
struct function_traits<R(Args...)> {
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
    static constexpr std::size_t arity = sizeof...(Args);

    template<std::size_t I>
    struct arg {
        static_assert(I < arity, "Index out of range");
        using type = std::tuple_element_t<I, args_tuple>;
    };
};

// Specialization for member functions (const)
template<typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const> {
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
    static constexpr std::size_t arity = sizeof...(Args);
    using class_type = C;

    template<std::size_t I>
    struct arg {
        static_assert(I < arity, "Index out of range");
        using type = std::tuple_element_t<I, args_tuple>;
    };
};

// Specialization for member functions (non-const)
template<typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...)> {
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
    static constexpr std::size_t arity = sizeof...(Args);
    using class_type = C;

    template<std::size_t I>
    struct arg {
        static_assert(I < arity, "Index out of range");
        using type = std::tuple_element_t<I, args_tuple>;
    };
};

/**
 * @brief A convenient alias to check if a function has the required prototype
 * This handles function pointers, references, and callable objects (lambdas, functors)
 */
template<typename FnT, typename RequiredReturnT, typename... RequiredArgsT>
struct function_has_the_required_prototype {
private:
    template<size_t... Idx>
    static constexpr auto is_all_argument_matched(std::index_sequence<Idx...>) -> bool
    {
        return (std::is_same_v<typename function_traits<FnT>::template arg<Idx>::type, RequiredArgsT> && ...);
    }

public:
    static constexpr bool value = std::is_same_v<typename function_traits<FnT>::return_type, RequiredReturnT> &&
                                  is_all_argument_matched(std::make_index_sequence<sizeof...(RequiredArgsT)>{});
};

// Simplified version for easier usage
template<typename FnT, typename RequiredReturnT, typename... RequiredArgsT>
constexpr bool function_has_the_required_prototype_v =
    function_has_the_required_prototype<FnT, RequiredReturnT, RequiredArgsT...>::value;

/**
 * @brief Concept to check if a function has the required prototype
 * This can be used in requires clauses
 */
template<typename FnT, typename RequiredReturnT, typename... RequiredArgsT>
concept has_required_prototype = function_has_the_required_prototype_v<FnT, RequiredReturnT, RequiredArgsT...>;

/**
 * @brief A convenient alias to check if a function has the required prototype
 * This handles function pointers, references, and callable objects (lambdas, functors)
 */
template<typename FnT, typename RequiredReturnT, typename TupleT>
struct function_has_the_required_prototype2;

/**
 * @brief A convenient alias to check if a function has the required prototype
 * This handles function pointers, references, and callable objects (lambdas, functors)
 */
template<typename FnT, typename RequiredReturnT, typename... RequiredArgsT>
struct function_has_the_required_prototype2<FnT, RequiredReturnT, std::tuple<RequiredArgsT...>> {
private:
    template<size_t... Idx>
    static constexpr auto is_all_argument_matched(std::index_sequence<Idx...>) -> bool
    {
        return (std::is_same_v<typename function_traits<FnT>::template arg<Idx>::type, RequiredArgsT> && ...);
    }

public:
    static constexpr bool value = std::is_same_v<typename function_traits<FnT>::return_type, RequiredReturnT> &&
                                  is_all_argument_matched(std::make_index_sequence<sizeof...(RequiredArgsT)>{});
};

// Simplified version for easier usage
template<typename FnT, typename RequiredReturnT, typename TupleT>
constexpr bool function_has_the_required_prototype_v2 =
    function_has_the_required_prototype2<FnT, RequiredReturnT, TupleT>::value;

/**
 * @brief Concept to check if a function has the required prototype
 * This can be used in requires clauses
 */
template<typename FnT, typename RequiredReturnT, typename TupleT>
concept has_required_prototype2 = function_has_the_required_prototype_v2<FnT, RequiredReturnT, TupleT>;

/**
 * @brief Will always return false at compile time regardless the type
 *
 * This metafunction is useful to fail compilation process if an undesired type passed into the template parameter.
 *
 * @tparam T Any type that will be regarded as compile time false
 */
template<typename T>
struct as_false : std::false_type {};

template<typename T>
constexpr bool as_false_v = as_false<T>::value;

template<typename T>
using filter_void = std::conditional_t<std::is_same_v<T, void>, std::tuple<>, std::tuple<T>>;

template<typename... Ts>
using remove_voids = decltype(std::tuple_cat(std::declval<filter_void<Ts>>()...));

template<typename RT, typename TupleT>
struct function_from_tuple;

template<typename RT, typename... Args>
struct function_from_tuple<RT, std::tuple<Args...>> {
    using type = typename std::function<RT(Args...)>;
};

template<typename... Ts>
struct integer_sequence_void_filter {
    template<size_t... Idxs>
    constexpr static auto create_filtered_sequence(std::index_sequence<Idxs...>)
    {
        constexpr std::array<size_t, sizeof...(Ts)> indices = []() {
            std::array<size_t, sizeof...(Ts)> buffer{};
            size_t count = 0;
            ((!std::is_same_v<Ts, void> ? (buffer[count++] = Idxs, 0) : 0), ...);
            return buffer;
        }();

        constexpr size_t count = ((!std::is_same_v<Ts, void> ? 1 : 0) + ...);

        return [&indices]<size_t... Idxss>(std::index_sequence<Idxss...>) {
            return std::index_sequence<indices[Idxss]...>{};
        }(std::make_index_sequence<count>{});
    }

    using filtered = decltype(create_filtered_sequence(std::make_index_sequence<sizeof...(Ts)>{}));
};

template<typename FnT>
struct is_std_function : std::false_type {};

template<typename R, typename... ArgsT>
struct is_std_function<std::function<R(ArgsT...)>> : std::true_type {};

template<typename FnT>
bool constexpr is_std_function_v = is_std_function<FnT>::value;

/**
 * @brief Compile-time check that a tuple contains no duplicate types.
 *
 * @tparam TupleT The tuple type to check.
 * @return true if all types in the tuple are unique, false otherwise.
 *
 * Used to ensure inward edge types are distinct for type-safe access.
 */
template<typename TupleT>
constexpr bool contained_no_duplicate_types = []<size_t... Idx>(std::index_sequence<Idx...>) {
    return (
        []<size_t I>(std::integral_constant<size_t, I>) {
            return (
                []<size_t J>(std::integral_constant<size_t, J>) {
                    if constexpr (
                        (I < J) && std::is_same_v<std::tuple_element_t<I, TupleT>, std::tuple_element_t<J, TupleT>>) {
                        return false;
                    }
                    else {
                        return true;
                    }
                }(std::integral_constant<size_t, Idx>{}) &&
                ...);
        }(std::integral_constant<size_t, Idx>{}) &&
        ...);
}(std::make_index_sequence<std::tuple_size_v<TupleT>>{});

} // namespace tw
