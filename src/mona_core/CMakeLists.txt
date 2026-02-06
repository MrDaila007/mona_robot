cmake_minimum_required(VERSION 3.8)
project(mona_core)

# Опции компилятора: включаем предупреждения для качественного кода
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 1. Находим зависимости
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)

# 2. Объявляем исполняемый файл
add_executable(mona_core_node src/mona_core_node.cpp)

# 3. Линкуем зависимости
ament_target_dependencies(mona_core_node rclcpp)

# 4. Устанавливаем (куда копировать после сборки)
install(TARGETS
  mona_core_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()