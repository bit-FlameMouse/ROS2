#!/usr/bin/env bash
# 文件用途：架构适应度函数自动检查（docs/16-架构规范与设计约束.md §15，AF-01 ~ AF-09）
#   用法： bash scripts/arch_check.sh
#   退出码：0 = 全部通过；1 = 存在失败项
#   豁免：命中行带 AS-EXEMPT 注释的视为已登记的例外（16 §17.1）
# 版权：2026 patrol_robot developer，Apache-2.0 许可（许可证原文见仓库根目录 LICENSE 文件）
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

PASS=0
FAIL=0
WARN=0

pass() { echo "  [PASS] $1"; PASS=$((PASS + 1)); }
fail() { echo "  [FAIL] $1"; FAIL=$((FAIL + 1)); }
warn() { echo "  [WARN] $1"; WARN=$((WARN + 1)); }
info() { echo "  [INFO] $1"; }

# filter_exempt <search_regex> <allow_regex> <path...>
#   输出未豁免的命中行；命中行自身或上下 2 行内含 AS-EXEMPT 视为已登记豁免（16 §17.1）
filter_exempt() {
  python3 - "$@" <<'PY'
import os
import re
import sys

search = re.compile(sys.argv[1])
allow = re.compile(sys.argv[2]) if sys.argv[2] else None


def iter_files(paths):
    for p in paths:
        if os.path.isdir(p):
            for root, _, files in os.walk(p):
                for f in sorted(files):
                    if f.endswith(('.cpp', '.hpp', '.h', '.py', '.xml', '.txt', '.cmake')):
                        yield os.path.join(root, f)
        else:
            yield p


for path in iter_files(sys.argv[3:]):
    try:
        lines = open(path, encoding='utf-8', errors='replace').read().splitlines()
    except OSError:
        continue
    for i, line in enumerate(lines):
        if not search.search(line):
            continue
        if allow and allow.search(line):
            continue
        if 'AS-EXEMPT' in '\n'.join(lines[max(0, i - 2):i + 3]):
            continue
        print(f'{path}:{i + 1}:{line.strip()}')
PY
}

echo "== 架构适应度函数检查（AF-01 ~ AF-09）=="

# ---------------- AF-01 仿真器标识 / cmd_vel / TF 发布 ----------------
echo "AF-01 L4 代码不含仿真器标识、不抢占 cmd_vel、不发布 TF"
AF01_SIM="$(grep -rniE 'gazebo|turtlebot3|burger|waffle|broadcast_transform|sendTransform' \
  src/patrol_robot_core/ 2>/dev/null || true)"
AF01_VEL="$(filter_exempt \
  'create_publisher<geometry_msgs::msg::Twist>|publish\(geometry_msgs::msg::Twist\(\)' \
  '' src/patrol_robot_core/)"
if [ -z "$AF01_SIM" ] && [ -z "$AF01_VEL" ]; then
  pass "无仿真器标识；cmd_vel 接管仅出现在已豁免的应急路径（默认关闭）"
else
  fail "存在未豁免命中："
  echo "$AF01_SIM"
  echo "$AF01_VEL"
fi

# ---------------- AF-02 包依赖方向 ----------------
echo "AF-02 包依赖方向（interfaces 不依赖 core；core 不依赖 bringup/launch_ros/turtlebot3）"
AF02_OK=1
if grep -E '<depend>|<exec_depend>|<build_depend>' src/patrol_interfaces/package.xml \
  | grep -qE 'rclcpp|nav2_msgs|patrol_robot_core|patrol_robot_bringup'; then
  fail "patrol_interfaces 存在违规依赖"
  AF02_OK=0
fi
if grep -E '<depend>|<exec_depend>|<build_depend>' src/patrol_robot_core/package.xml \
  | grep -qE 'launch_ros|turtlebot3|gazebo|patrol_robot_bringup'; then
  fail "patrol_robot_core 存在违规依赖"
  AF02_OK=0
fi
[ "$AF02_OK" -eq 1 ] && pass "依赖方向正确"

# ---------------- AF-03 纯逻辑类零 ROS 依赖 ----------------
echo "AF-03 纯逻辑类（状态机 / 安全判定 / 航点加载）零 ROS 依赖"
AF03_MATCHES="$(grep -rn 'rclcpp\|rosidl\|sensor_msgs' \
  src/patrol_robot_core/src/patrol_state_machine.cpp \
  src/patrol_robot_core/src/safety_evaluator.cpp \
  src/patrol_robot_core/src/waypoint_loader.cpp 2>/dev/null || true)"
if [ -z "$AF03_MATCHES" ]; then
  pass "无 ROS 头文件引用"
else
  fail "纯逻辑类出现 ROS 依赖："
  echo "$AF03_MATCHES"
fi

# ---------------- AF-04 回调内禁止阻塞等待 Action 结果 ----------------
echo "AF-04 无阻塞等待（.get() / spin_until_future_complete / wait_for）"
AF04_MATCHES="$(filter_exempt '\.get\(\)|spin_until_future_complete|wait_for\(' \
  'wait_for_action_server|wait_for_service' src/)"
if [ -z "$AF04_MATCHES" ]; then
  pass "无未豁免命中（已豁免行带 AS-EXEMPT 标注）"
else
  fail "存在未豁免的阻塞等待："
  echo "$AF04_MATCHES"
fi

# ---------------- AF-05 时间统一使用 ROS 时钟 ----------------
echo "AF-05 不使用挂钟/系统时钟参与业务计时"
AF05_MATCHES="$(grep -rn 'system_clock\|steady_clock\|RCL_SYSTEM_TIME' src/ 2>/dev/null || true)"
if [ -z "$AF05_MATCHES" ]; then
  pass "无挂钟时间调用"
else
  fail "存在挂钟时间调用："
  echo "$AF05_MATCHES"
fi

# ---------------- AF-06 QoS 显式声明（括号配对检查） ----------------
echo "AF-06 所有 create_publisher / create_subscription / create_service 显式传入 QoS"
AF06_RESULT="$(python3 - <<'EOF'
import glob
import re
import sys

bad = []
checked = 0
for path in glob.glob('src/*/src/*.cpp') + glob.glob('src/*/src/*.hpp'):
    text = open(path, encoding='utf-8').read()
    for m in re.finditer(r'create_(publisher|subscription|service|wall_timer)[<(]', text):
        if 'wall_timer' in m.group(0):
            continue
        # 从匹配处向后扫描，找到配对的右括号
        i = text.index('(', m.start())
        depth = 0
        while i < len(text):
            if text[i] == '(':
                depth += 1
            elif text[i] == ')':
                depth -= 1
                if depth == 0:
                    break
            i += 1
        args = text[m.start():i]
        checked += 1
        if not re.search(r'qos|QoS|rmw_qos_profile', args):
            line = text[:m.start()].count('\n') + 1
            bad.append(f'{path}:{line}')
print(f'checked={checked}')
for b in bad:
    print(b)
EOF
)"
AF06_CHECKED="$(echo "$AF06_RESULT" | sed -n 's/^checked=//p')"
AF06_BAD="$(echo "$AF06_RESULT" | grep -v '^checked=' || true)"
if [ -z "$AF06_BAD" ]; then
  pass "检查了 $AF06_CHECKED 处创建调用，QoS 均显式传入"
else
  fail "以下创建调用未显式传入 QoS："
  echo "$AF06_BAD"
fi

# ---------------- AF-07 话题名参数化（默认值允许，调用点硬编码不允许） ----------------
echo "AF-07 话题/坐标系名参数化（调用点无硬编码）"
AF07_OK=1
AF07_A="$(grep -rEn 'declare_parameter(<[^>]*>)?\(\s*"/' src/patrol_robot_core/ 2>/dev/null || true)"
if [ -n "$AF07_A" ]; then
  fail "参数名以 / 开头："
  echo "$AF07_A"
  AF07_OK=0
fi
AF07_B="$(filter_exempt '"map"' '' src/patrol_robot_core/src src/patrol_robot_core/include \
  | grep -v 'declare_parameter' \
  | grep -v 'frame_id{"map"}' \
  | grep -v '^[^:]*:[0-9]*: *//' \
  | grep -v '^[^:]*:[0-9]*:.*///' || true)"
if [ -n "$AF07_B" ]; then
  fail "坐标系名字面量出现在默认值/结构体默认值之外："
  echo "$AF07_B"
  AF07_OK=0
fi
[ "$AF07_OK" -eq 1 ] && pass "命名均已参数化"

# ---------------- AF-08 魔法数字（人工核对项，输出供评审） ----------------
echo "AF-08 浮点字面量人工核对（非参数默认值的数值需有明确语义）"
AF08_MATCHES="$(grep -rnE '[^a-zA-Z_](0\.[0-9]+|[0-9]+\.[0-9]+f)' \
  src/patrol_robot_core/src/ 2>/dev/null \
  | grep -v 'declare_parameter' || true)"
if [ -z "$AF08_MATCHES" ]; then
  pass "无非参数浮点字面量"
else
  warn "以下数值需人工核对（展示常量/算法常量应带注释；可调量必须参数化，共 $(echo "$AF08_MATCHES" | wc -l) 行）："
  echo "$AF08_MATCHES" | head -40
fi

# ---------------- AF-09 非法参数拒绝启动 ----------------
echo "AF-09 非法参数导致非零退出（需已 source install/setup.bash）"
if ros2 pkg prefix patrol_robot_core >/dev/null 2>&1; then
  AF09_OK=0
  if timeout 20 ros2 run patrol_robot_core safety_guard_node \
    --ros-args -p stop_distance:=0.8 -p resume_distance:=0.6 >/dev/null 2>&1; then
    fail "safety_guard_node 在 stop>resume 时未拒绝启动"
  else
    AF09_OK=$((AF09_OK + 1))
  fi
  if timeout 20 ros2 run patrol_robot_core patrol_node \
    --ros-args -p tick_period_ms:=5 >/dev/null 2>&1; then
    fail "patrol_node 在 tick_period_ms 越界时未拒绝启动"
  else
    AF09_OK=$((AF09_OK + 1))
  fi
  [ "$AF09_OK" -eq 2 ] && pass "两个节点均在非法参数下非零退出"
else
  warn "未找到 patrol_robot_core（请先 colcon build 并 source install/setup.bash），跳过"
fi

echo
echo "== 结果：PASS=$PASS FAIL=$FAIL WARN=$WARN =="
if [ "$FAIL" -gt 0 ]; then
  echo "存在失败项，请按 16 文档 §17 处理（整改或登记 AS-EXEMPT）。"
  exit 1
fi
echo "全部检查通过。"
exit 0
