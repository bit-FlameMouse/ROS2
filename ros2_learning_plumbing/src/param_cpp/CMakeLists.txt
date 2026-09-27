cmake_minimum_required(VERSION 3.8)
project(param_cpp)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rcl_interfaces REQUIRED)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

add_executable(demo01_param_server src/demo01_param_server.cpp)  # 参数服务端
ament_target_dependencies(demo01_param_server rclcpp rcl_interfaces)            # 依赖 rclcpp

add_executable(demo02_param_client src/demo02_param_client.cpp)  # 参数客户端
ament_target_dependencies(demo02_param_client rclcpp rcl_interfaces)            # 依赖 rclcpp

install(TARGETS
  demo01_param_server
  demo02_param_client
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
