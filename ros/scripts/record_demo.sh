#!/usr/bin/env bash
# 文件用途：录制演示所需的关键话题（rosbag2），配合屏幕录像使用
# 用法：
#   bash scripts/record_demo.sh            # 输出到 ~/patrol_demo_bag
#   bash scripts/record_demo.sh /path/bag  # 指定输出目录
# 说明：
#   - 本脚本只录话题数据；演示画面/音轨请使用 OBS 等录屏软件（见 12 文档 §8）
#   - Ctrl+C 结束录制；回放： ros2 bag play <bag_dir>
# 版权：2026 patrol_robot developer，Apache-2.0 许可（许可证原文见仓库根目录 LICENSE 文件）
set -euo pipefail

source /opt/ros/humble/setup.bash

OUT_DIR="${1:-$HOME/patrol_demo_bag}"

if [ -e "$OUT_DIR" ]; then
  echo "错误：输出目录已存在：$OUT_DIR（请先改名或删除）" >&2
  exit 1
fi

echo "==> 开始录制到 $OUT_DIR"
echo "==> 话题：/scan /odom /amcl_pose /patrol/status /safety/emergency_stop"
echo "==>       /safety/nearest_obstacle_distance /cmd_vel /tf /tf_static"
echo "==> 按 Ctrl+C 结束"

ros2 bag record -o "$OUT_DIR" \
  /scan \
  /odom \
  /amcl_pose \
  /patrol/status \
  /safety/emergency_stop \
  /safety/nearest_obstacle_distance \
  /cmd_vel \
  /tf \
  /tf_static

echo "==> 录制结束：$OUT_DIR"
