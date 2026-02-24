# Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

cmake_minimum_required(VERSION 3.15)

project(taskweave LANGUAGES CXX)

#######################################################################################
# PROJECT SETUP
#######################################################################################

add_subdirectory(src)

include(CTest)
add_subdirectory(tests)

install(TARGETS ${PROJECT_NAME}
  FILE_SET HEADERS
)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "bin"
    LIBRARY DESTINATION "lib"
)
