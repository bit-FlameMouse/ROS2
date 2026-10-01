#!/usr/bin/env bash
# 文件用途：仿真测试环境清理——终止上一轮仿真/导航/任务层进程，避免同名节点与 TF 双发布者残留
# 用法： bash scripts/simtest/cleanup.sh
# 注意：① pkill/pgrep 模式使用 [x] 技巧，避免匹配到本脚本自身的命令行；
#       ② 显式排除本脚本的全部祖先进程：当 cleanup 与 ros2 launch 写在同一条命令行
#          （如 `bash cleanup.sh && ros2 launch ... patrol.launch.py`）时，父 shell 的
#          命令行里含有 "<xxx>.launch.py" 字样，会被 "[.]launch.py" 模式误杀
#          （2026-10-01 实测问题）。
# 版权：2026 patrol robot developer，Apache-2.0 许可（许可证原文见仓库根目录 LICENSE 文件）

# 收集自身与全部祖先 PID（逐级向上直到 init），清理时跳过这些进程
ancestors=" $$ "
pid=$$
while true; do
  ppid=$(ps -o ppid= -p "$pid" 2>/dev/null | tr -d ' ')
  [ -z "$ppid" ] || [ "$ppid" -le 1 ] && break
  ancestors="$ancestors$ppid "
  pid=$ppid
done

kill_pattern() {
  local pat="$1" pid
  for pid in $(pgrep -f "$pat" 2>/dev/null); do
    case "$ancestors" in
      *" $pid "*) continue ;;  # 自身或祖先进程：跳过
    esac
    kill -9 "$pid" 2>/dev/null || true
  done
}

for pat in \
  "[g]zserver" "[g]zclient" "[s]lam_toolbox" "[c]omponent_container" \
  "[r]obot_state_publisher" "[s]pawn_entity" "[r]viz2" "[p]atrol_node" \
  "[s]afety_guard_node" "[l]ifecycle_manager" "[.]launch.py" \
  "[m]ock_nav" "[f]ake_scan" "[p]erimeter_driver" "[d]emo_goals" "[n]av_goals"; do
  kill_pattern "$pat"
done
sleep 1
remaining=$(ps -eo args | grep -E "gzserver|slam_toolbox|component_container|robot_state_pub|spawn_entity|patrol_node|safety_guard_node|launch.py" | grep -v grep | wc -l)
if [ "$remaining" -ne 0 ]; then
  echo "警告：仍有 $remaining 个相关进程残留："
  ps -eo pid,args | grep -E "gzserver|slam_toolbox|component_container|robot_state_pub|spawn_entity|patrol_node|safety_guard_node|launch.py" | grep -v grep | head
  exit 1
fi
echo "环境已清理（无残留进程）"
