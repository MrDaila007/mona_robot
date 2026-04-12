# C++ Development Guide

> **Hard Real-Time & Safety Rules**
> 
> Robot control logic directly impacts physical safety. We prioritize memory predictability, strict typing, and non-blocking operations.

## 1. Package Creation

Always use the `ament_cmake` build system.
```bash
ros2 pkg create mona_new_package --build-type ament_cmake --dependencies rclcpp rclcpp_lifecycle
```

---

## 2. Managed Node Pattern (Safe Lifecycles)

When using the `MultiThreadedExecutor`, there is a risk of **"Zombie Callbacks"**. This occurs when a node is deactivated (`on_deactivate`), but a parallel thread is still processing an old message from the queue, which might override the safe state (e.g., re-enabling motors).
**Mandatory Pattern:** Use an atomic `is_processing_allowed_` flag.

**Structure Example (`src/my_node.cpp`):**
```c++
#include <atomic>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"

class MySafeNode : public rclcpp_lifecycle::LifecycleNode {
private:
    std::atomic<bool> is_processing_allowed_{false};

public:
    explicit MySafeNode(const rclcpp.NodeOptions & options)
    : rclcpp_lifecycle::LifecycleNode("my_safe_node", options) {}

    CallbackReturn on_activate(const rclcpp_lifecycle::State &) {
        is_processing_allowed_ = True;
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_deactivate(const rclcpp_lifecycle::State &) {
        is_processing_allowed_ = False;
        // Wait for potential parallel threads to finish or drop data
        return CallbackReturn::SUCCESS;
    }

    void topic_callback(const std_msgs::msg::String::SharedPtr msg) {
        if (!is_processing_allowed_) return; // DROP ZOMBIE DATA
        // Process logic...
    }
};
```

---

## 3. Strict Linter Compliance

Every `CMakeLists.txt` must include the standard linter block within `BUILD_TESTING`. We enforce `-Wall -Wextra -Wpedantic` to catch errors at compile-time.

```cmake
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

if(BUILD_TESTING)
  # Use manual package discovery for linters (instead of ament_lint_auto)
  # to guarantee strict compliance with enterprise QA standards.
  find_package(ament_cmake_cppcheck   REQUIRED)
  find_package(ament_cmake_cpplint    REQUIRED)
  find_package(ament_cmake_uncrustify REQUIRED)
  find_package(ament_cmake_xmllint    REQUIRED)
  find_package(ament_cmake_lint_cmake REQUIRED)
  find_package(ament_cmake_copyright  REQUIRED)
  find_package(ament_cmake_flake8     REQUIRED)

  # Execute Linters
  # C++ static analysis and style formatting
  ament_cppcheck()
  ament_cpplint()
  ament_uncrustify(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../configs/uncrustify.cfg")

  # Copyright verification
  ament_copyright()

  # Python linter with custom configuration
  ament_flake8(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../configs/setup.cfg")

  # XML syntax validation (package, launch, xacro)
  ament_xmllint()

  # CMake syntax formatting
  ament_lint_cmake()
endif()
```

---

## 4. Memory Management
- Avoid `std::shared_ptr` for high-frequency internal data; use `unique_ptr` where possible.
- Strictly no `new` or `delete` operators; use `std::make_shared` or `std::make_unique`.
- Pre-allocate vector capacity to avoid real-time reallocations.