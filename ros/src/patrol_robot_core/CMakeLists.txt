cmake_minimum_required(VERSION 3.8)
project(patrol_robot_core)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ---------------- 依赖 ----------------
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(visualization_msgs REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(patrol_interfaces REQUIRED)

# ---------------- 可复用库（纯逻辑 + Action 封装）----------------
add_library(patrol_core_lib SHARED
  src/waypoint_loader.cpp
  src/nav_client.cpp
  src/patrol_state_machine.cpp
  src/safety_evaluator.cpp
)
target_include_directories(patrol_core_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
ament_target_dependencies(patrol_core_lib
  rclcpp rclcpp_action std_msgs geometry_msgs sensor_msgs nav_msgs
  visualization_msgs nav2_msgs tf2 tf2_geometry_msgs patrol_interfaces
)

# ---------------- 可执行文件 ----------------
add_executable(patrol_node src/patrol_node.cpp)
target_link_libraries(patrol_node patrol_core_lib)
ament_target_dependencies(patrol_node
  rclcpp std_msgs visualization_msgs patrol_interfaces)

add_executable(safety_guard_node src/safety_guard_node.cpp)
target_link_libraries(safety_guard_node patrol_core_lib)
ament_target_dependencies(safety_guard_node
  rclcpp sensor_msgs std_msgs geometry_msgs)

install(TARGETS
  patrol_core_lib
  patrol_node
  safety_guard_node
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)
install(DIRECTORY include/
  DESTINATION include
)

# ---------------- 测试 ----------------
if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()

  ament_add_gtest(test_waypoint_loader test/test_waypoint_loader.cpp)
  target_link_libraries(test_waypoint_loader patrol_core_lib)

  ament_add_gtest(test_patrol_state_machine test/test_patrol_state_machine.cpp)
  target_link_libraries(test_patrol_state_machine patrol_core_lib)

  ament_add_gtest(test_safety_evaluator test/test_safety_evaluator.cpp)
  target_link_libraries(test_safety_evaluator patrol_core_lib)
endif()

ament_export_include_directories(include)
ament_export_libraries(patrol_core_lib)
ament_export_dependencies(rclcpp patrol_interfaces)
ament_package()
