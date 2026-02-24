# Copyright 2026 Experian Elitiawan and contributors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME}
    # PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    INTERFACE $<INSTALL_INTERFACE:include>)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

target_sources(${PROJECT_NAME}
  # PRIVATE
  # TaskWeave/Error.cpp
  INTERFACE
  FILE_SET HEADERS
    FILES
      Executor/ThreadPool.h
      Executor/ThreadPoolExecutor.h
      TaskWeave/Edge.h
      TaskWeave/Helper.h
      TaskWeave/IEdge.h
      TaskWeave/INode.h
      TaskWeave/ITask.h
      TaskWeave/Metafunctions.h
      TaskWeave/Node.h
      TaskWeave/Task.h
)
