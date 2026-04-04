# Copyright 2026 vladubase
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.8)
project(mona_control)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake      REQUIRED)
find_package(rclcpp           REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(rclcpp_action    REQUIRED)
find_package(geometry_msgs    REQUIRED)
find_package(std_msgs         REQUIRED)
find_package(nav2_msgs        REQUIRED)

include_directories(include)

# Сборка библиотеки компонента
add_library(twist_mux_component SHARED
  src/twist_mux_node.cpp
)

target_include_directories(twist_mux_component PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Линкуем зависимости
ament_target_dependencies(twist_mux_component
  rclcpp
  rclcpp_lifecycle
  rclcpp_components
  rclcpp_action
  geometry_msgs
  std_msgs
  nav2_msgs
)

# Регистрируем плагин в системе ROS 2
rclcpp_components_register_node(twist_mux_component
  PLUGIN "mona_control::TwistMuxNode"
  EXECUTABLE twist_mux_node
)

# Установка (install)
install(
  TARGETS twist_mux_component twist_mux_node
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(
  DIRECTORY   include/
  DESTINATION include
)

# Тестирование и линтеры
if(BUILD_TESTING)
  # 1. Используем ручной поиск конкретных пакетов (вместо ament_lint_auto)
  find_package(ament_cmake_gtest      REQUIRED)
  find_package(ament_cmake_cppcheck   REQUIRED)
  find_package(ament_cmake_cpplint    REQUIRED)
  find_package(ament_cmake_uncrustify REQUIRED)
  find_package(ament_cmake_xmllint    REQUIRED)
  find_package(ament_cmake_lint_cmake REQUIRED)
  find_package(ament_cmake_copyright  REQUIRED)
  find_package(ament_cmake_flake8     REQUIRED)

  # 2. Сборка GTest
  ament_add_gtest(test_control test/test_lifecycle_instantiation.cpp)

  target_link_libraries(test_control twist_mux_component)

  ament_target_dependencies(test_control
    rclcpp
    rclcpp_lifecycle
  )

  # 3. Запускаем линтеры
  # C++ статический анализ и стиль
  ament_cppcheck()
  ament_cpplint()
  # uncrustify с кастомным конфигом
  ament_uncrustify(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../configs/uncrustify.cfg")

  # Проверка лицензий (Copyright)
  ament_copyright()

  # Python линтер с кастомным конфигом
  ament_flake8(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../configs/setup.cfg")

  # XML (package, launch, xacro)
  ament_xmllint()

  # CMake стиль
  ament_lint_cmake()
endif()

ament_package()
