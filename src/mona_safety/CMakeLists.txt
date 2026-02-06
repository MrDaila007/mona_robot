cmake_minimum_required(VERSION 3.8)
project(mona_safety)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(geometry_msgs REQUIRED)

add_executable(safety_node src/safety_node.cpp)

ament_target_dependencies(safety_node
  rclcpp
  rclcpp_lifecycle
  geometry_msgs
  lifecycle_msgs
)

install(TARGETS
  safety_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()