cmake_minimum_required(VERSION 3.8)
project(mona_safety)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake      REQUIRED)
find_package(rclcpp           REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(lifecycle_msgs   REQUIRED)
find_package(geometry_msgs    REQUIRED)

add_executable(mona_safety_node src/safety_node.cpp)

ament_target_dependencies(mona_safety_node
  rclcpp
  rclcpp_lifecycle
  geometry_msgs
  lifecycle_msgs
)

install(
  DIRECTORY   launch
  DESTINATION share/${PROJECT_NAME}
)

install(
  TARGETS     mona_safety_node
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)

  list(APPEND AMENT_LINT_AUTO_EXCLUDE
    ament_cmake_uncrustify
    ament_cmake_copyright
    ament_cmake_cpplint
    ament_cmake_flake8
    ament_copyright
    ament_cpplint
    uncrustify
    cpplint
  )

  ament_lint_auto_find_test_dependencies()

  # Запуск Flake8 (Python)
  find_package(ament_cmake_flake8 REQUIRED)
  ament_flake8(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../configs/setup.cfg")

  # Запуск Uncrustify (C++)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src")
    find_package(ament_cmake_uncrustify REQUIRED)
    ament_uncrustify(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../configs/uncrustify.cfg")
  endif()
endif()

ament_package()