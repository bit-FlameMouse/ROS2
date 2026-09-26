cmake_minimum_required(VERSION 3.8)
project(topic_cpp)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(base_interfaces_demo REQUIRED)

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

add_executable(demo01_talker_str src/demo01_talker_str.cpp)  # 添加可执行文件
ament_target_dependencies(demo01_talker_str rclcpp std_msgs) # 添加依赖

add_executable(demo02_listener_str src/demo02_listener_str.cpp)
ament_target_dependencies(demo02_listener_str rclcpp std_msgs)

add_executable(demo03_talker_student src/demo03_talker_student.cpp)
ament_target_dependencies(demo03_talker_student rclcpp base_interfaces_demo)

add_executable(demo04_listener_student src/demo04_listener_student.cpp)
ament_target_dependencies(demo04_listener_student rclcpp base_interfaces_demo)

install(TARGETS
  demo01_talker_str
  demo02_listener_str
  demo03_talker_student
  demo04_listener_student
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()