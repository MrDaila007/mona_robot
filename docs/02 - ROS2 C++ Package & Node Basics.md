# Цель
Создать архитектурную основу для C++ кода робота: пакет `mona_core` и базовую ноду, использующую современный подход (наследование от `rclcpp::Node`).

# 1. Создание пакета
Мы используем тип сборки `ament_cmake` (стандарт для C++ в ROS 2) и сразу добавляем зависимость от библиотеки `rclcpp`.

**Команда (выполнять в `~/mona_ws/src`):**
```bash
ros2 pkg create --build-type ament_cmake --dependencies rclcpp mona_core
```

# 2. Исходный код Ноды
Создаем ноду как класс. Это позволяет в будущем использовать Composition (загрузку нод как компонентов в один процесс), что критично для производительности на embedded-системах.

**Файл:** `src/mona_core/src/mona_core_node.cpp`
```c++
#include <rclcpp/rclcpp.hpp>

// Наследуемся от rclcpp::Node — это стандарт для C++ нод
class MonaCoreNode : public rclcpp::Node {
public:
	// Конструктор: задаем имя ноды ("mona_core_node")
	MonaCoreNode() : Node("mona_core_node") {
		// RCLCPP_INFO — это аналог printf/cout, но с метками времени и уровнями логирования
		RCLCPP_INFO(this->get_logger(), "Hello, MONA! C++ Node is running.");
	}
};

int main(int argc, char **argv) {
	// 1. Инициализация ROS 2 (парсинг аргументов)
	rclcpp::init(argc, argv);

	// 2. Создание ноды и передача её в "исполнитель" (spin)
	// make_shared создает умный указатель (smart pointer)
	auto node = std::make_shared<MonaCoreNode>();

	// spin запускает бесконечный цикл обработки событий (таймеры, топики)
	// Пока событий нет, нода просто "висит" и ждет (Hello выведется в конструкторе)
	rclcpp::spin(node);

	// 3. Корректное завершение
	rclcpp::shutdown();
	return 0;
}
```

# 3. Настройка CMakeLists.txt
Очищенный и настроенный файл сборки. Мы включаем флаги предупреждений компилятора для контроля качества кода.

**Файл:** `src/mona_core/CMakeLists.txt`
```cmake
cmake_minimum_required(VERSION 3.8)
project(mona_core)

# Опции компилятора: Strict Mode
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 1. Поиск зависимостей
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)

# 2. Создание исполняемого файла
add_executable(mona_core_node src/mona_core_node.cpp)

# 3. Линковка библиотек ROS
ament_target_dependencies(mona_core_node rclcpp)

# 4. Установка (куда копировать бинарник)
install(TARGETS
  mona_core_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
```

# 4. Сборка и Запуск
Цикл компиляции и проверки работоспособности.

**Команды (в корне воркспейса `~/mona_ws`):**
```bash
# 1. Сборка только нашего пакета
colcon build --packages-select mona_core

# 2. Обновление путей (Source)
source install/setup.bash

# 3. Запуск ноды
ros2 run mona_core mona_core_node
```

**Ожидаемый вывод:**
```
[INFO] [timestamp] [mona_core_node]: Hello, MONA! C++ Node is running.
```
