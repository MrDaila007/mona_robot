# Руководство по C++ разработке

## 1. Создание нового пакета
Используется система сборки `ament_cmake`. При создании пакета сразу указываем зависимости.

Внутри контейнера (`/root/mona_ws/src`):
```bash
ros2 pkg create --build-type ament_cmake --dependencies rclcpp mona_new_package
````

---
## 2. Базовая структура Ноды
В проекте принят стиль наследования от `rclcpp::Node`. Это позволяет использовать **Composition** (загрузку нод как компонентов), что критично для производительности на embedded-системах.
### Шаблон кода (Boilerplate)
Используйте этот шаблон для создания новых нод.

**Файл:** `src/mona_new_package/src/example_node.cpp`
```c++
#include <rclcpp/rclcpp.hpp>

class ExampleNode : public rclcpp::Node {
public:
    explicit ExampleNode() : Node("example_node") {
        // Объявление параметров
        this->declare_parameter("loop_rate", 1.0);

        // Логирование
        RCLCPP_INFO(this->get_logger(), "Node initialized successfully.");
    }

private:
    // Сюда добавляем паблишеры, сабскрайберы и таймеры
};

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<ExampleNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
```

---
## 3. Настройка `CMakeLists.txt`
Для корректной сборки и установки ноды добавляем следующие строки в `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.8)
project(example_node)

# 0. Включаем все оповещения о Warnings при компиляции.
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 1. Поиск пакетов
find_package(rclcpp REQUIRED)

# 2. Создание исполняемого файла
add_executable(example_node src/example_node.cpp)
ament_target_dependencies(example_node rclcpp)

# 3. Установка таргета в lib
install(
  TARGETS example_node
  DESTINATION lib/${PROJECT_NAME}
)

# 4. Установка остальных директорий в share
install(
  DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
```

> На все пакеты указываем `find_package(xxx REQUIRED)`, чтобы на этапе компиляции сразу видеть отсутсвующие зависимости

