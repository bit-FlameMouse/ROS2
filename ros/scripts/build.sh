#!/usr/bin/env bash
# 文件用途：一键编译工作空间（默认 Release + symlink-install）
# 用法：
#   bash scripts/build.sh                     # 编译全部三个包
#   bash scripts/build.sh --packages-select patrol_robot_core
#   bash scripts/build.sh --clean             # 先清理 build/install/log 再编译
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

# shellcheck disable=SC1091
source /opt/ros/humble/setup.bash

CLEAN=0
ARGS=()
for arg in "$@"; do
  if [ "$arg" = "--clean" ]; then
    CLEAN=1
  else
    ARGS+=("$arg")
  fi
done

if [ "$CLEAN" -eq 1 ]; then
  echo "==> 清理 build/ install/ log/"
  rm -rf build install log
fi

echo "==> colcon build（Release + symlink-install）"
colcon build \
  --symlink-install \
  --cmake-args -DCMAKE_BUILD_TYPE=Release \
  "${ARGS[@]}"

echo "==> 完成。请执行： source install/setup.bash"
