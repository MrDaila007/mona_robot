cmake_minimum_required(VERSION 3.8)
project(mona_core)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)

add_executable(mona_core_node src/mona_core_node.cpp)

ament_target_dependencies(mona_core_node rclcpp)

install(TARGETS
  mona_core_node
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

  # Запускаем flake8 вручную, указывая на общий конфиг в корне
  find_package(ament_cmake_flake8 REQUIRED)
  ament_flake8(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../configs/setup.cfg")

  # Существующий блок для uncrustify (C++)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src")
    find_package(ament_cmake_uncrustify REQUIRED)
    ament_uncrustify(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../configs/uncrustify.cfg")
  endif()
endif()

ament_package()