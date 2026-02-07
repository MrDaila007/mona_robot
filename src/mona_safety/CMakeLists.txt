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

ament_package()