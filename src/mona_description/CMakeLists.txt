cmake_minimum_required(VERSION 3.8)
project(mona_description)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(urdf REQUIRED)
find_package(xacro REQUIRED)

install(
  DIRECTORY launch urdf meshes rviz worlds
  DESTINATION share/${PROJECT_NAME}
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
endif()

ament_package()