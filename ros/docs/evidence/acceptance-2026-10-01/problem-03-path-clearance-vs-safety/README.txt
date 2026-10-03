发现问题：2 圈巡逻在首段即触发安全紧急停车（mode 4）并永久卡住
时间：2026-10-01 23:08:12（sim t≈180 触发，至归档时仍 mode 4）
根因链：
  1) nav2_params.yaml：inflation_radius 0.30 = 安全阈值 0.30，且 cost_scaling_factor 仅 3.0
  2) 0.8 m 宽走廊被全部膨胀后导航代价面近似平坦，NavFn 路径偏向西侧
  3) 路径距圆柱表面 ~0.24 m < 0.30 m → safety_guard 置位 emergency → 取消目标、mode 4
  4) 静态世界中障碍永不清除（resume 需 >0.60 m）→ 巡逻无法恢复
现场数据：
  - 停车点扫描最近障碍 0.238 m @ +27°（scan_at_stop.txt）
  - 反投影校验：地图/AMCL 对齐误差仅 ~3 cm，非定位问题
  - 行驶期间 global costmap 报 'Transform data too old' 288 次（tf_error_count.txt，AMCL update_min_d=0.25 门限所致）
修复方案：inflation_radius 0.55 + cost_scaling 8.0（两代价地图）；amcl update_min_d/a 降至 0.10
