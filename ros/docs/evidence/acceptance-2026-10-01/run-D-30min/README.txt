Run D 记录：30 分钟长稳 + 异常深挖（TC-S-06 主证据）
==========================================================================
环境：headless 启动（gui:=false use_rviz:=false）、ROS_DOMAIN_ID=30、rmw_cyclonedds_cpp。
时间：2026-10-01 深夜 ~10-02 凌晨（D1 段约 23:46–23:58；D2 段 00:01:42 启动）。
说明：D2 原始日志与状态流已随本目录归档（runD2_full_log.txt、runD2_status_stream.txt）；
      其余为本目录按主题抽取的证据切片。全部为文本留证（无界面环境）。

--------------------------------------------------------------------------
D1 段（约 23:46–23:58）：首次异常 —— 空路径 → 控制器中止风暴
--------------------------------------------------------------------------
- 现象起始：1790869874.96（23:51:14.96）某航点下发后 controller_server 持续报
  "Resulting plan has 0 poses in it" / "Received plan with zero length" /
  "Controller patience exceeded" → 目标被中止；patrol 侧最终 mode=5 / error_code=4。
- 处置验证：RESET + START 无法恢复（runD_reset_start_no_recovery.txt）；
  杀掉整个 launch 重新拉起后恢复正常（runD_restart_reply.txt）。
- 计数快照 @23:58:36：zero-length 358、0-poses 87、too-old 152、patience 56、
  安全报警 0（runD_log_counts_pre_recovery.txt）。
- 内存部分采样（采样方法同下方"采样小坑"，仅作参考）：23:47 18.7/17.8 MB →
  23:52 19.5/18.3 MB → 23:57 19.1/18.9 MB（runD_mem_samples_partial.txt）。
- 归属：11 文档坑 23 / 14 文档 TD-12（Nav2 行为树间歇性空路径异常；未修复，整体重启可恢复）。
- 证据文件：runD_anomaly_onset.txt、runD_patrol_events.txt、
  runD_status_failed_snapshot.txt、runD_reset_reply.txt、runD_reset_start_no_recovery.txt、
  runD_restart_reply.txt、runD_log_counts_pre_recovery.txt、runD_mem_samples_partial.txt

--------------------------------------------------------------------------
D2 段（00:01:42 启动 → 00:31:42 满 30 min）：正式长稳窗口
--------------------------------------------------------------------------
巡逻 START：00:02:05（runD2_setmode_start.txt）

1) 正常段（lap 1–3，约 00:02–00:12）
   - 9 次 "Goal succeeded"（3 航点 × 3 圈），每段约 20 s，无安全事件。
   - 判据锚点：completed_waypoints=9、current_loop=3。

2) 异常段（lap 4 起，约第 10–11 次进近柱区后）
   00:05:42.6   map→odom 冻结起点（末次滤波更新 sim 234.165 s）
   00:05:48.33  第 1 次安全夹停 0.28 m（柱区窄缝，mode 4 锁存）
   00:09:50–00:10:27  人工施救①（转向 48° + 倒车）：near 0.267→0.615 m 解除，
                patrol 自动重发 WP0、恢复巡线
   00:10:30.39  第 2 次夹停（施救后 2.8 s 回夹）→ 00:10:36.22 施救②解除
   00:12:35.45  WP0 两次 60 s 超时 → 跳过（idx 0→1、retry 重置、err=3）
   00:13:17.55  WP1 下发 → "Failed to make progress" 中止 → 重发
   00:14:16.0   WP1 60 s 超时 → 跳过（idx 1→2）
   00:14:17.95  WP2 下发（bt 播报起点 (1.73, -0.22)）
   00:14:22.88  第 3 次夹停 0.27 m：机器人在 tf 冻结、控制器失效状态下发生
                ~0.5 m 非巡线位移，最终位于东墙前（odom (2.149, 0.117)、
                amcl (2.104, 0.065)、yaw≈-74°，车头距墙约 0.22 m）→ 静态障碍
                永久 mode 4（按设计不复位；按手册 §13 决策不再施救，保持原状记录）
   00:14:23.00  controller "Goal was canceled. Stopping the robot."
   → 此后至窗口结束零推进（终态：mode 4、idx 2、completed 9、loop 3、err 3）

3) TC-S-06 四项判定（窗口 00:01:42–00:31:42）
   ① 无崩溃                      ✅  patrol/safety 进程全程存活至窗口终点（PID 37527/37525；
                                  00:31:42 随后台任务限时正常收档，全程无 crash 日志）
   ② 内存无持续增长               ⚠️  见"内存采样"节：4 点 18.96→19.56→20.14→20.69 MB
                                  线性缓升（≈+0.57 MB/5 min，无平台期）；幅度远低于
                                  1.5× 上限；按字面判据更近 ✗，保守记 ⚠️ 并列观察项
   ③ completed_waypoints 持续增长  ✗  9 后停滞（lap4 WP0/WP1 超时跳过、WP2 夹停）
   ④ 无 TF/QoS 刷屏               ✗  too-old 报错 4179 条，Transform time 恒 234.165 s
   总结论：⚠️ 部分通过 —— 无崩溃成立；③④为已登记缺陷（坑 22/23、TD-12）的伴生表现；
          ②存在线性缓升（4 点复核定性）：登记为观察项。

4) tf 冻结证据（坑 23 机制实证）
   - 4179 条 "Transform data too old"，Transform time 恒为 sim 234.165 s
     （= 首次夹停前 1.1 s 的最后一次 AMCL 滤波时间）；最后一条止于第 3 次夹停后 72 ms、
     goal cancel 前 45 ms。分段：743.75→1150.20 密集（~10 Hz）；中间 107 s 空窗；
     WP2 段 1258.00→1262.955 再度密集（~20 Hz）。
   - mode 4 停驻实测（00:21:55）：/tf 中 map→odom 以 5 Hz 重发、时间戳在走、
     数值冻结在 (-0.047, 0.004)；静止时 /amcl_pose 6 s 内 0 条。
   - 旁证：/patrol/status 的 distance_remaining 冻结在 3.3583 m 恒定 >400 s；
     bt 播报起点 (1.73,-0.22) 与 odom/amcl 实测终点差 0.47 m（定位账本失稳；
     当次未录包，微观动作序列无法进一步复原）。
   - 恢复条件：真实移动 ≥0.10 m 后滤波恢复推进（前段夹停解除后自动恢复巡线即实证）。
   - 证据：runD2_tf_freeze.txt（本文件上方口径的完整样本与统计）。

5) 人工施救记录（坑 22 处置手法手册化验证）
   - 脚本：runD2_recover_script.py（原地转向 48° + 倒车退出前向锥；全程监控
     360° 扫描原始最近点，<0.17 m 立即中止）。
   - 施救①CLEARED：travel=0.340 m、near 0.267→0.615 m；施救②复跑同样成功。
   - 第 3 次为东墙静态顶死：按手册决策"放着不管"并记录（两种处置路径均有留证）。
   - 施救前快照：runD2_safety_stop_pre_recovery.txt（mode4/near=0.267）；
     第 3 次现场读数：runD2_third_stop.txt。

6) 内存采样（修正采样器；runD2_mem_fixed.txt）
   采样时刻     patrol_rss   safety_rss   备注
   00:15:41     18960 KB     17680 KB
   00:20:41     19564 KB     18312 KB
   00:25:41     20136 KB     18920 KB
   00:30:41     20692 KB     19404 KB     （第 4 点；此后仿真于 00:31:42 满 30 min
                                           被任务时限终止，窗后第 5 点未采）
   对照：Run A2 基准 19692/18412 KB（短程运行）
   ⚠️ 采样小坑：最初用 `pgrep -f patrol_node` 会匹配到采样脚本自身，采得恒定 ~3.6 MB
   假值——前 3 行已作废并在 runD2_mem.txt 内注明；修正写法见 17 手册 §13.2 小坑说明。

7) 收官计数（00:31:42；仿真被后台任务 30 min 时限自动终止，恰满窗口）
   - too-old 最终条数：4179（确认停止增长）
   - 终态快照：runD2_status_final.txt（mode 4 / idx 2 / completed 9 / loop 3 /
     retry 0 / err 3 "航点导航超时" / distance_remaining 3.3583；最后一条 sim 1787.6）
   - 计数总表：Goal succeeded=9、紧急置位=3、航点超时告警=3（WP0×2、WP1×1）、
     跳过事件=2（以状态流 idx 跳变为准；日志无"跳过"字样）、Failed to make progress=19、
     状态流 1789 条（26835 行）、日志 9301 行、mode5/err4=0
   - 采集口径：runD2_final_counts.txt（00:31:45 自动采集；终态 status 一条
     因进程已随窗口终止而超时无输出，终态另见上条快照）

--------------------------------------------------------------------------
结论与去向
--------------------------------------------------------------------------
- TC-S-06 判定 ⚠️ 部分通过（明细已写入 08 文档 §9 TC-S-06 行）。
- 坑 22 施救手法两度实证有效（0.34 m 倒车解除）；坑 23 机理证据链完整
  （值冻结/5 Hz 重发/too-old 报错/真实移动后恢复）。
- 长跑内存 4 点线性缓升（≈+0.57 MB/5 min）：登记为观察项并在 TC-S-06 行注明。
- 文档同步：08 §9、11 坑 22/23、12 清单、14 v0.2.3 已知问题、17 §14/§15（2026-10-02）。
