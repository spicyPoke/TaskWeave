# TaskWeave

A modern C++20 task scheduling and execution framework designed for complex dependency management and parallel processing.

## Overview

TaskWeave is a header-only C++20 library that provides a sophisticated task graph system with thread pool execution capabilities. It enables developers to define complex task dependencies and execute them efficiently in parallel.

## Features

- **Template-based Tasks**: Strongly-typed tasks with support for return values and dependencies
- **Dependency Management**: Tasks can depend on other tasks through a graph-based relationship system
- **Thread Pool Execution**: Configurable thread pool with efficient task distribution
- **Compile-time Validation**: Make sure to catch mistakes early at compile time

## Build Requirements

- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 19.29+)
- CMake 3.15 or later
- Conan package manager
- Ninja build system

## Building

### Prerequisites

Before building, ensure you have Conan package manager installed in your local Python environment:

```bash
# Create and activate a Python virtual environment
python3 -m venv .venv
source .venv/bin/activate

# Install Conan
pip install conan

# Verify installation
conan --version
```

### Building with Conan

Available profiles: `gcc-debug`, `gcc-release`, `clang-debug`, `clang-release`

**Note:** Conan generates CMake presets dynamically with the naming convention: `conan-<arch>-<os>-<compiler>-<build_type>` (e.g., `conan-x86_64-linux-gcc-release`). These presets are created in the output folder during `conan install`.

Build folders by compiler:
- GCC: `out/gcc`
- Clang: `out/clang`

```bash
# Clone and configure
git clone <repository-url>
cd TaskWeave

# Activate the virtual environment
source .venv/bin/activate

# Install dependencies via Conan (using gcc-release profile)
# For debug builds, use tools/conan/profiles/gcc-debug instead
conan install . --profile:build=tools/conan/profiles/gcc-release --profile:host=tools/conan/profiles/gcc-release --output-folder=out/gcc --build=missing

# Configure the project using the generated preset
cmake --preset conan-x86_64-linux-gcc-release

# Build the project
cmake --build --preset conan-x86_64-linux-gcc-release

# Run tests
ctest
```

### Using Clang Compiler

For Clang builds, use the clang-release profile (or clang-debug for debug builds):

```bash
# Activate the virtual environment
source .venv/bin/activate

# Install dependencies via Conan using the clang-release profile
conan install . --profile:build=tools/conan/profiles/clang-release --profile:host=tools/conan/profiles/clang-release --output-folder=out/clang --build=missing

# Configure and build using the generated preset (preset name is dynamically created by Conan)
cmake --preset conan-x86_64-linux-clang-release
cmake --build --preset conan-x86_64-linux-clang-release

# Run tests
ctest
```

### Integration

Since TaskWeave is header-only, you can integrate it into your project by simply including the headers:

```cpp
#include "TaskWeave/Task.h"
#include "Executor/ThreadPoolExecutor.h"
```

## Usage Example

### Basic Usage

```cpp
#include "TaskWeave/Task.h"
#include "TaskWeave/Helper.h"
#include "Executor/ThreadPoolExecutor.h"

#include <iostream>

int main() {
    // Create a thread pool executor
    tw::ThreadPoolExecutor executor;

    // Define a simple task
    tw::Task<int> task;
    task.set_callable([]() -> int {
        std::cout << "Executing task" << std::endl;
        return 42;
    });

    // Add task to executor
    executor.add_task(&task);

    // Run the scheduled tasks
    executor.run();

    // Wait for completion
    executor.wait();

    std::cout << "Result: " << task.get_result() << std::endl;
    return 0;
}
```

### Tasks with Dependencies

```cpp
#include "TaskWeave/Task.h"
#include "Executor/ThreadPoolExecutor.h"

int main() {
    tw::ThreadPoolExecutor executor;

    // Create producer task
    tw::Task<int> producer;
    producer.set_callable([]() -> int {
        return 42;
    });

    // Create consumer task that depends on producer
    tw::Task<int, int> consumer;
    consumer.set_callable([](int value) -> int {
        return value * 2;
    });

    // Connect producer to consumer
    consumer.add_inward_edge<int>(producer.get_outward_edge());

    // Add tasks to executor
    executor.add_task(&producer);
    executor.add_task(&consumer);

    // Run - reachability is computed automatically
    executor.run();
    executor.wait();

    // consumer result is 84 (42 * 2)
    return 0;
}
```

### Multiple Dependencies

A task can depend on multiple independent tasks:

```cpp
#include "TaskWeave/Task.h"
#include "Executor/ThreadPoolExecutor.h"
#include <iostream>

int main() {
    tw::ThreadPoolExecutor executor;

    // Create two independent data sources
    tw::Task<int> source_a;
    source_a.set_callable([]() -> int { return 10; });

    tw::Task<int> source_b;
    source_b.set_callable([]() -> int { return 20; });

    // Create aggregator that depends on both sources
    tw::Task<int, int, int> aggregator;
    aggregator.set_callable([](int a, int b) -> int {
        return a + b;
    });

    // Connect both sources to aggregator
    aggregator.add_inward_edge<int>(source_a.get_outward_edge());
    aggregator.add_inward_edge<int>(source_b.get_outward_edge());

    // Add all tasks to executor
    executor.add_task(&source_a);
    executor.add_task(&source_b);
    executor.add_task(&aggregator);

    executor.run();
    executor.wait();

    // aggregator result is 30 (10 + 20)
    std::cout << "Result: " << aggregator.get_result() << std::endl;
    return 0;
}
```

### Complex Dependency Chain

Tasks can form multi-level dependency chains:

```cpp
#include "TaskWeave/Task.h"
#include "Executor/ThreadPoolExecutor.h"
#include <iostream>

int main() {
    tw::ThreadPoolExecutor executor;

    // Level 1: Initial data generation
    tw::Task<int> generate;
    generate.set_callable([]() -> int { return 5; });

    // Level 2: Transform the data
    tw::Task<int, int> transform;
    transform.set_callable([](int x) -> int { return x * x; });
    transform.add_inward_edge<int>(generate.get_outward_edge());

    // Level 3: Further processing
    tw::Task<int, int> process;
    process.set_callable([](int x) -> int { return x + 10; });
    process.add_inward_edge<int>(transform.get_outward_edge());

    // Level 4: Final aggregation from multiple paths
    tw::Task<int, int, int> finalize;
    finalize.set_callable([](int a, int b) -> int { return a * b; });
    finalize.add_inward_edge<int>(process.get_outward_edge());
    finalize.add_inward_edge<int>(generate.get_outward_edge());  // Also depends on original

    executor.add_task(&generate);
    executor.add_task(&transform);
    executor.add_task(&process);
    executor.add_task(&finalize);

    executor.run();
    executor.wait();

    // Execution order: generate -> transform -> process -> finalize
    // generate: 5
    // transform: 25 (5 * 5)
    // process: 35 (25 + 10)
    // finalize: 175 (35 * 5)
    std::cout << "Final result: " << finalize.get_result() << std::endl;
    return 0;
}
```

## Design Philosophy

The library emphasizes:
- **Type Safety**: Compile-time validation of function signatures
- **Flexibility**: Support for complex dependency graphs and custom execution policies
- **Modern C++**: Leveraging C++20 features for clean, expressive APIs

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request. For major changes, please open an issue first to discuss what you would like to change.

### Development Setup

```bash
# Clone the repository
git clone <repository-url>
cd TaskWeave

# Activate the virtual environment
source .venv/bin/activate

# Install dependencies via Conan (debug profile)
conan install . --profile:build=tools/conan/profiles/gcc-debug --profile:host=tools/conan/profiles/gcc-debug --output-folder=out/gcc --build=missing

# Configure and build using presets
cmake --preset conan-x86_64-linux-gcc-debug
cmake --build --preset conan-x86_64-linux-gcc-debug

# Run tests
ctest
```

## License

This project is licensed under the BSD-style license - see the [LICENSE](LICENSE) file for details.
