Run A2 —— 手册 17 第 7/8 关（TC-S-01 自动巡逻、TC-S-02 服务控制、TC-S-03 航点变色）
+ 第 6 关服务序列 + 性能快测（TC-P-01/02/03）
日期：2026-10-01 晚（无界面服务器，Xshell/SSH）
启动参数：/tmp/patrol_2laps.yaml（默认 3 航点；loop_count=2，waypoint_wait=2s）
仿真日志：/tmp/patrol_run.log（未入库，关键行见 log_summary.txt 等）
备注：本运行在问题 03 修复（nav2_params 膨胀/AMCL 参数）之后进行，属"修复后复验"。

启动计时（TC-P-03）：Nav2 就绪 12 s、扫描链路 16 s（< 60 s 判据）
巡逻流程：START（idle→moving）→ WP0 → WP1 → WP2 → 第 2 轮 → 全部完成

关键结果：
1) 自动巡逻（TC-S-01）：两次 START 之间穿插服务演练；最终
   1790867562.2"巡逻任务全部完成：共 6 个航点，2 轮"；
   status_final：mode=0, completed_waypoints=6, current_loop=2, error_code=0
   （status_stream_2laps.txt 全程 2490 行：sim 88 s mode 0 → sim 252 s completed 6）
2) 服务序列（TC-S-02）：START/重复 START(NO_STATE_CHANGE)/PAUSE→3/RESUME→1/
   STOP→0/RESET OK 全部符合设计（tc_s02_service_replies.txt）
   巡逻中暂停/恢复：PAUSE 触发取消目标（cancel+Goal canceled），RESUME 后重新
   发送同航点目标并恢复（patrol_pause_resume.txt）
3) 航点变色（TC-S-03，文本留证）：巡逻前全部灰 (0.60,0.60,0.60) →
   巡逻中 WP0/WP1 绿、当前目标 WP2 黄 (1.00,0.85,0.00) →
   2 轮后全部绿 (0.20,0.85,0.30)；标签 WP0/WP1/WP2 齐全
   （markers_pre_patrol_gray.txt / markers_mid_patrol.txt / markers_after_2laps_green.txt）
4) 频率与延迟（TC-P-01）：/patrol/status 1.000 Hz（判据 1.0±10%）；
   delay 12 样本平均 0.000 s、最大 0.000 s（< 0.2 s）
5) 资源占用（TC-P-02）：patrol_node CPU 1.3% / RSS 19692 KB，
   safety_guard_node CPU 0.5% / RSS 18412 KB（合计 CPU 1.8% < 10%）
6) 环境链路（TC-I-01）：/scan 4.971 Hz、/clock 9.907 Hz、/odom 29.123 Hz、
   TF map→odom→base_link 完整；AMCL 偏差 0.000 m（20 样本）；
   安全事件计数：0（log_summary.txt）

文件清单：
  startup_time_A2.txt            启动计时（Nav2 12 s / 扫描 16 s）
  log_summary.txt                巡逻关键节点 + Nav2 起终点 + 安全事件=0
  tc_s02_service_replies.txt     6 次服务调用与应答（TC-S-02）
  patrol_pause_resume.txt        巡逻中 PAUSE/RESUME 的取消与恢复日志
  status_stream_2laps.txt        /patrol/status 全程（2490 行，最终 completed=6）
  status_final.txt               最终状态快照（mode=0, loop=2, error=0）
  markers_pre_patrol_gray.txt    巡逻前标记（全灰）
  markers_mid_patrol.txt         巡逻中标记（WP2 黄）
  markers_after_2laps_green.txt  2 轮后标记（全绿）
  hz_status/scan/odom/clock.txt  各话题实测频率
  delay_status.txt               状态话题延迟（12 样本 0.000 s）
  tf_odom.txt                    map→odom 变换（出生点 -2.0,-0.5, z≈0.009）
  amcl.txt                       AMCL 与 odom 位姿偏差（20 样本 0.000 m）
  ps_nodes.txt                   两自研节点 CPU/RSS 快照
  hz_*.txt / delay_*.txt 等      见上
