# Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

#######################################################################################
# PROJECT SETUP
#######################################################################################
find_package(GTest REQUIRED)

list(APPEND test_list
    test_function_traits.cpp
    test_metafunctions.cpp
    test_edge.cpp
    test_node.cpp
    test_task.cpp
    test_thread_pool.cpp
    test_pooled_task_executor.cpp
    test_void_task.cpp
    test_auto_reachability.cpp
    stress_test_utils.h
    stress_test_independent_tasks.cpp
    stress_test_dependent_tasks.cpp
    stress_test_pooled_executor.cpp
    stress_test_thread_pool.cpp
)

set(test_name ${PROJECT_NAME}-test)
add_executable(${test_name})
target_sources(${test_name} PRIVATE ${test_list})
target_compile_features(${test_name} PRIVATE cxx_std_20)
target_link_libraries(${test_name} PRIVATE ${PROJECT_NAME} GTest::gtest_main)

# Apply pedantic warning flags for tests
target_compile_options(${test_name} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(NAME ${test_name} COMMAND ${test_name})
