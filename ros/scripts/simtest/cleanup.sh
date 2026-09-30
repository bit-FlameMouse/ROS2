#!/usr/bin/env bash
# 文件用途：仿真测试环境清理——终止上一轮仿真/导航/任务层进程，避免同名节点与 TF 双发布者残留
# 用法： bash scripts/simtest/cleanup.sh
# 注意：pkill 模式使用 [x] 技巧，避免匹配到本脚本自身的命令行
# 版权：2026 patrol_robot developer，Apache-2.0 许可（许可证原文见仓库根目录 LICENSE 文件）
for pat in \
  "[g]zserver" "[g]zclient" "[s]lam_toolbox" "[c]omponent_container" \
  "[r]obot_state_publisher" "[s]pawn_entity" "[r]viz2" "[p]atrol_node" \
  "[s]afety_guard_node" "[l]ifecycle_manager" "[.]launch.py" \
  "[m]ock_nav" "[f]ake_scan" "[p]erimeter_driver" "[d]emo_goals" "[n]av_goals"; do
  pkill -9 -f "$pat" 2>/dev/null || true
done
sleep 1
remaining=$(ps -eo args | grep -E "gzserver|slam_toolbox|component_container|robot_state_pub|spawn_entity|patrol_node|safety_guard_node|launch.py" | grep -v grep | wc -l)
if [ "$remaining" -ne 0 ]; then
  echo "警告：仍有 $remaining 个相关进程残留："
  ps -eo pid,args | grep -E "gzserver|slam_toolbox|component_container|robot_state_pub|spawn_entity|patrol_node|safety_guard_node|launch.py" | grep -v grep | head
  exit 1
fi
echo "环境已清理（无残留进程）"
