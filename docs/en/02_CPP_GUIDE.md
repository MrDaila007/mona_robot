# C++ Development Guide

> **Hard Real-Time & Safety Rules**
>
> Robot control code directly affects physical safety. We prioritize memory predictability, strict typing, and the absence of blocking operations.

## 1. Creating packages

We use the `ament_cmake` build system:

```bash
ros2 pkg create mona_new_package --build-type ament_cmake --dependencies rclcpp rclcpp_lifecycle
```

## 2. Safe node template (Managed Node)

When using `MultiThreadedExecutor` in ROS 2 there is a risk of **"Zombie Callbacks"** — a situation where the node receives a signal to shut down (`on_deactivate` or `on_shutdown`), but an old message from the queue is still processed in a parallel thread and can overwrite a safe state (for example, re‑enabling the motors).

**Mandatory pattern:** use an atomic flag `is_processing_allowed_`.

**Example structure (`src/my_node.cpp`):**

```c++
#include <atomic>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"

class MySafeNode : public rclcpp_lifecycle::LifecycleNode {
private:
    std::atomic<bool> is_processing_allowed_{false};

public:
    explicit MySafeNode(const rclcpp::NodeOptions & options)
    : rclcpp_lifecycle::LifecycleNode("my_safe_node", options) {}

    // Ensure safety when the OS destroys the object
    ~MySafeNode() { disable_hardware(); }

    CallbackReturn on_activate(const rclcpp_lifecycle::State &) override {
        is_processing_allowed_ = true; // Allow processing
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_deactivate(const rclcpp_lifecycle::State &) override {
        is_processing_allowed_ = false; // Immediately block all callbacks
        disable_hardware();
        return CallbackReturn::SUCCESS;
    }
    
    CallbackReturn on_shutdown(const rclcpp_lifecycle::State &) override {
        is_processing_allowed_ = false; 
        disable_hardware();
        return CallbackReturn::SUCCESS;
    }

private:
    void my_topic_callback(const std_msgs::msg::String::SharedPtr msg) {
        if (!is_processing_allowed_) return; // Zombie protection
        
        // ... business logic ...
    }
    
    void disable_hardware() {
        // Code to open contactors or send zero velocities
    }
};
```

## 3. CMake best practices

In `CMakeLists.txt` you must enable warning flags (`-Wall -Wextra -Wpedantic`). Linters `ament_cppcheck` and `ament_cpplint` should be run inside the `BUILD_TESTING` block.

```cmake
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

if(BUILD_TESTING)
  # 1. Use explicit discovery of specific packages (instead of ament_lint_auto)
  find_package(ament_cmake_xmllint    REQUIRED)
  find_package(ament_cmake_lint_cmake REQUIRED)
  find_package(ament_cmake_flake8     REQUIRED)
  find_package(ament_cmake_copyright  REQUIRED)

  # 2. Run each linter manually
  # License checks
  ament_copyright()

  # Python linter with custom config
  ament_flake8(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../configs/setup.cfg")

  # XML (package, launch, xacro)
  ament_xmllint()

  # CMake style
  ament_lint_cmake()
endif()
```

> Always use `find_package(xxx REQUIRED)` for all dependencies to fail fast at compile time when something is missing.

## 4. Logging

Use `RCLCPP_INFO`, `RCLCPP_WARN`, `RCLCPP_ERROR` macros.  
Using `std::cout` or `printf` in production code is forbidden because it breaks log standardization.  
For critical sections you may use ANSI colors (for example, `\033[34m` for blue).

