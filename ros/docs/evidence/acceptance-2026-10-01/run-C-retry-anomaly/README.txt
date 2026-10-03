Run C —— 手册 17 第 10 关（TC-S-05：坏航点重试与跳过）+ 已知边界（最后一个航点 / 支路风险）
日期：2026-10-01 晚（无界面服务器，Xshell/SSH）
启动参数：/tmp/patrol_retry.yaml（waypoint_y[1]=-2.80 在代价地图覆盖范围之外；
loop_count=0；max_retries=2；skip_failed_waypoint=true；stop/resume=0.30/0.60）
仿真日志：/tmp/patrol_retry.log（C1）、/tmp/patrol_retry2.log（C2 误启动）、
          /tmp/patrol_retry2b.log（C2b）；均未入库，关键行已提取为下列 txt

C1 —— 完整场景（23:19:18 启动）：
1) WP0 (1.75, 0) 正常到达（1790868019.5 "Reached the goal!"）
2) WP1 (0.00, -2.80) 两次尝试均失败：planner 报
   "The goal sent to the planner is off the global costmap"，
   中间正常穿插 spin / wait / backup 恢复行为
3) 重试超限 → 跳过生效：1790868074.6 日志"前往航点 [2] x=-1.75 y=0.00"；
   status 中 current_waypoint_index 1→2、retry_count 归零
4) 跳过后的 WP2 行进失败（异常）：DWB 持续收到空路径——
   "Received plan with zero length" ×303、"Resulting plan has 0 poses in it." ×159，
   持续约 5 分钟，伴随 "Controller patience exceeded"，机器人原地不动；
   WP2 两次尝试均失败，且其为最后一个航点、无下一航点可跳 →
   mode 5 (FAILED)，error_code=4"航点重试次数超限，任务终止"（retry_count=2,
   completed=1, index=2）
5) 异常排查：单独调用 compute_path_to_pose 正常返回完整路径；直接发送独立目标
   （同一 WP2、以及简单的 WP0 坐标，均在巡检已定格约 3.6 分钟后）同样失败；
   "Unable to transform robot pose into global plan's frame" 0 次
   → 排除规划器与 TF，定位为 bt_navigator 内部状态异常，间歇性；
   冷启动重启即恢复（C2 误启动后重跑 C2b 与两次定向复现均未再出现）→ 11 文档坑 23

C2 —— 误启动（23:30:13）：上一轮终态朝向的遗留使机器人在距东墙 0.30 m 处
面向墙启动，START 后立即"前方障碍过近（0.30 m < 0.30 m）"→ 永久 mode 4。
属演示注意事项（非缺陷）：已记入 17 手册 §15.2（面向墙壁的永久 mode 4）。
改从开阔处重跑为 C2b。

C2b —— 完整场景重跑（23:37:57）：
1) WP0 正常到达；WP1 两次失败、恢复行为后按设计跳过
   （1790869186.0"前往航点 [2]"）
2) 跳过支路行进 25 s 后，前方 0.26 m < 0.30 m 触发安全守护 → emergency=true、
   patrol 取消目标 → 永久 mode 4
   （status: index=2, completed=1, distance_remaining=0.83 m）
3) 结论：重试与"跳过"逻辑本身工作正常；"跳过后继续走完"两次运行分别被两个
   已知问题阻塞（空路径异常 / 静态障碍安全死锁），均已登记为 11 文档坑 22、坑 23

通过标准核对（手册 §12）：
- retry 0→1 可见（尝试 2 次）                                  ✅
- 重试超限后跳过生效（日志"前往航点 [2]"、index 1→2）          ✅
- "跳过后续航点继续走完"——受已知边界限制，见上 ⚠️

文件清单：
  runC_status_stream.txt    C1 全程 /patrol/status（含最终 mode 5 快照）
  nav_failure_trace.txt     C1 关键日志摘录（WP1 失败→跳过→空路径刷屏→mode 5）
  runC_setmode_start.txt    C1 开始巡逻服务应答（success=True, mode=1）
  runC2b_status_stream.txt  C2b 全程状态（含最终 mode 4 快照）
  c2b_nav_trace.txt         C2b 关键日志（WP1 失败→跳过→紧急停车→取消）
  c2b_safety_events.txt     C2b 安全守护置位 + patrol 标志变化（各 1 行）
