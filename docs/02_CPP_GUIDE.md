# Руководство по C++ разработке

> **Hard Real-Time & Safety Rules**
> 
> Код управления роботом напрямую влияет на физическую безопасность. Отдаём приоритет предсказуемости памяти, строгой типизации и отсутствию блокирующих операций.

## 1. Создание пакетов
Используется система сборки `ament_cmake`.
```bash
ros2 pkg create mona_new_package --build-type ament_cmake --dependencies rclcpp rclcpp_lifecycle
```

## 2. Шаблон безопасной ноды (Managed Node)
При использовании `MultiThreadedExecutor` в ROS 2 существует риск **"Zombie Callbacks"** — ситуация, когда нода получает сигнал на выключение (`on_deactivate` или `on_shutdown`), но в параллельном потоке продолжает обрабатываться старое сообщение из очереди, которое может переопределить безопасное состояние (например, заново включить моторы).

**Обязательный паттерн:** Использование атомарного флага `is_processing_allowed_`.

**Пример структуры (`src/my_node.cpp`):**
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

    // Гарантируем безопасность при уничтожении объекта ОС
    ~MySafeNode() { disable_hardware(); }

    CallbackReturn on_activate(const rclcpp_lifecycle::State &) override {
        is_processing_allowed_ = true; // Разрешаем работу
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_deactivate(const rclcpp_lifecycle::State &) override {
        is_processing_allowed_ = false; // Мгновенно блокируем все коллбэки
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
        if (!is_processing_allowed_) return; // Защита от Зомби
        
        // ... бизнес логика ...
    }
    
    void disable_hardware() {
        // Код отключения контакторов или отправки 0 скоростей
    }
};
```

## 3. CMake Best Practices
В `CMakeLists.txt` обязательное включение флагов предупреждений (`-Wall -Wextra -Wpedantic`). Линтеры `ament_cppcheck` и `ament_cpplint` должны запускаться в блоке `BUILD_TESTING`.
```cmake
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

if(BUILD_TESTING)
  # 1. Используем ручной поиск конкретных пакетов (вместо ament_lint_auto)
  find_package(ament_cmake_xmllint    REQUIRED)
  find_package(ament_cmake_lint_cmake REQUIRED)
  find_package(ament_cmake_copyright  REQUIRED)
  find_package(ament_cmake_flake8     REQUIRED)

  # 2. Запускаем каждый линтер вручную
  # Проверка лицензий (Copyright)
  ament_copyright()

  # Python линтер с кастомным конфигом
  ament_flake8(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../configs/setup.cfg")

  # XML (package, launch, xacro)
  ament_xmllint()

  # CMake стиль
  ament_lint_cmake()
endif()
```
> На все пакеты указываем `find_package(xxx REQUIRED)`, чтобы на этапе компиляции сразу видеть отсутствующие зависимости

## 4. Логирование
Используем макросы `RCLCPP_INFO`, `RCLCPP_WARN`, `RCLCPP_ERROR`.
Запрещено использование `std::cout` или `printf` в продакшн-коде, так как это нарушает стандартизацию логов. Для критических секций допускается использование ANSI-цветов (например, `\033[34m` для синего).
