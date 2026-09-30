#!/usr/bin/env bash
# 文件用途：安装本项目全部运行时与编译依赖（见 docs/03-环境搭建与依赖清单.md §5.2）
# 版权：2026 patrol_robot developer，Apache-2.0 许可（许可证原文见仓库根目录 LICENSE 文件）
set -euo pipefail

if [ "${ROS_DISTRO:-}" != "humble" ]; then
  echo "错误：请先 source /opt/ros/humble/setup.bash" >&2
  exit 1
fi

echo "==> 更新 apt 索引"
sudo apt update

echo "==> 安装 Gazebo 与 TurtleBot3 仿真栈"
sudo apt install -y \
  ros-humble-gazebo-ros-pkgs \
  ros-humble-turtlebot3 \
  ros-humble-turtlebot3-msgs \
  ros-humble-turtlebot3-simulations \
  ros-humble-turtlebot3-navigation2

echo "==> 安装 Nav2 与 SLAM"
sudo apt install -y \
  ros-humble-navigation2 \
  ros-humble-nav2-bringup \
  ros-humble-nav2-msgs \
  ros-humble-slam-toolbox

echo "==> 安装 TF / 可视化 / 中间件"
sudo apt install -y \
  ros-humble-tf2 ros-humble-tf2-ros ros-humble-tf2-geometry-msgs \
  ros-humble-tf2-tools ros-humble-visualization-msgs \
  ros-humble-rmw-cyclonedds-cpp \
  ros-humble-teleop-twist-keyboard

echo "==> 安装测试与规范检查工具"
sudo apt install -y \
  ros-humble-ament-cmake-gtest \
  ros-humble-ament-lint-auto \
  ros-humble-ament-lint-common

echo "==> 用 rosdep 补齐剩余依赖"
sudo rosdep init 2>/dev/null || true
rosdep update || echo "警告：rosdep update 失败，请检查网络"
rosdep install --from-paths src --ignore-src -r -y || true

echo "==> 验证关键依赖"
missing=0
for pkg in gazebo_ros turtlebot3_gazebo nav2_bringup nav2_msgs slam_toolbox rviz2; do
  if ros2 pkg prefix "$pkg" >/dev/null 2>&1; then
    echo "  [OK]   $pkg"
  else
    echo "  [MISS] $pkg"
    missing=1
  fi
done
if [ "$missing" -ne 0 ]; then
  echo "错误：存在缺失依赖，请检查上方输出。" >&2
  exit 1
fi

echo "==> 依赖安装完成（建议写入 ~/.bashrc：export TURTLEBOT3_MODEL=burger）"
