# Руководство по C++ разработке

> **Hard Real-Time & Safety Rules**
> Код управления роботом напрямую влияет на физическую безопасность. Отдаём приоритет предсказуемости памяти, строгой типизации и отсутствию блокирующих операций.
## 1. Создание пакетов
Используется система сборки `ament_cmake`.
```bash
ros2 pkg create mona_new_package --build-type ament_cmake --dependencies rclcpp rclcpp_lifecycle 
```
## 2. Шаблон безопасной ноды (Managed Node)
Все драйверы и контроллеры должны наследоваться от `rclcpp_lifecycle::LifecycleNode`.

**Пример структуры (`src/my_node.cpp`):**
```c++
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"

class MySafeNode : public rclcpp_lifecycle::LifecycleNode {
public:
    explicit MySafeNode(const rclcpp::NodeOptions & options)
    : rclcpp_lifecycle::LifecycleNode("my_safe_node", options) {}

    // Выделение ресурсов (память, порты)
    CallbackReturn on_configure(const rclcpp_lifecycle::State &) override {
        // ...
        return CallbackReturn::SUCCESS;
    }

    // Активация публикации данных
    CallbackReturn on_activate(const rclcpp_lifecycle::State &) override {
        // ...
        return CallbackReturn::SUCCESS;
    }

    // Деактивация (Safe Stop)
    CallbackReturn on_deactivate(const rclcpp_lifecycle::State &) override {
        // ...
        return CallbackReturn::SUCCESS;
    }
    
    // Очистка ресурсов
    CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override {
        // ...
        return CallbackReturn::SUCCESS;
    }
};
```
## 3. CMake Best Practices
В `CMakeLists.txt` обязательное включение флагов предупреждений:
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
Используйте макросы `RCLCPP_INFO`, `RCLCPP_WARN`, `RCLCPP_ERROR`. Запрещено использование `std::cout` или `printf` в продакшн-коде, так как это нарушает стандартизацию логов.
