发现问题：干净终端里 bash scripts/build.sh 直接退出（exit=1），无任何构建输出
时间：2026-10-01 22:59:03
根因：build.sh 第 8 行 set -euo pipefail 后第 14 行 source /opt/ros/humble/setup.bash；
      Humble 的 setup.bash 第 8 行在 set -u 下引用未定义变量 AMENT_TRACE_SETUP_FILES 报错，
      source 返回非零，set -e 令脚本退出。record_demo.sh 存在同一模式。
复现：bash -c 'set -u; source /opt/ros/humble/setup.bash'
