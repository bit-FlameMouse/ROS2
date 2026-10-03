Run B —— 手册 17 第 9 关（TC-S-04：安全急刹与自动恢复）+ §11.3 方式 B（手动注入）
日期：2026-10-01 晚（无界面服务器，Xshell/SSH）
启动参数：/tmp/patrol_safety_demo.yaml（loop_count: 2，waypoint_wait: [15,15,15]）
仿真日志：/tmp/patrol_safety.log（关键行见 startup_B.txt / safety_events.txt）

过程与结果（时间取自日志头，与墙钟同步）：
1) 23:16:01 机器人到 WP1 停站（/patrol/status mode=2，15 秒操作窗口打开）
   front_point.py 0.25 -> 机器人位姿 (0.19,1.87) yaw=103.2°，正前方 0.25 m = (0.13, 2.11)
2) 23:16:12 spawn safety_box 成功（0.2x0.2x0.4 静态方块，中心距机器人 0.25 m）
   +0.36 s：safety_guard "前方障碍过近（0.16 m < 0.30 m），置位紧急标志"
   +0.36 s：patrol_node "安全标志变化: emergency=true"
   随后 /patrol/status 持续 mode=4（遇险停车；完整消息见 runB_mode4_status.txt）
3) 锁存值读取（TRANSIENT_LOCAL 订阅脚本 check_emergency.py）：data=True
   注：本机 Humble 的 `ros2 topic echo --once --qos-durability transient_local`
   启动后未及时返回锁存样本（挂起>30 s）；持续运行的 echo 能正常收到后续变化
   （见 runB_emergency_echo.txt），因此锁存值改用等价订阅脚本读取。
4) 23:17:18 delete_entity safety_box 成功
   +0.02 s：safety_guard "前方畅通（0.72 m > 0.60 m），解除紧急标志"
   +0.02 s：patrol_node "安全标志变化: emergency=false"
   /patrol/status：mode 4 -> 2（继续歇完剩余停站）-> 1（23:17:34 恢复航行）
5) 锁存值复查：data=False
6) §11.3 方式 B（手动注入）：23:17:57 pub true -> emergency=true、mode=4；
   23:18:20 pub false -> emergency=false、mode 2 -> 1（runB_emergency_echo.txt 为 echo 全程输出）

通过标志核对（手册 §11.2 第 6/7 步）：
- 放箱子 -> data:true + mode=4                      ✅
- 拿开   -> data:false + mode 回到 2/1              ✅
- 日志成对出现 "前方障碍过近…置位紧急标志/安全标志变化: emergency=true"
  与 "前方畅通…解除紧急标志/安全标志变化: emergency=false"  ✅

文件清单：
  startup_B.txt                  就绪关键行（spawn/Nav2/巡逻/雷达）
  safety_events.txt              置位/解除 + 标志变化 全部日志行
  runB_front_point.txt           前方 0.25 m 坐标计算输出
  runB_spawn_box.txt             放箱子命令输出（Successfully spawned）
  runB_mode4_status.txt          完整 mode=4 状态消息（含 completed/index）
  runB_emergency_true.txt        锁存值 data=True
  runB_delete_box.txt            删箱子命令输出（Successfully deleted）
  runB_emergency_false.txt       锁存值 data=False
  runB_emergency_echo.txt        持续 echo 收到的 data:true / data:false
  runB_status_stream.txt         /patrol/status 全程（含 2->4->2->1 各阶段）
  status_around_emergency.txt    状态流中紧急事件前后切片
  runB_setmode_start.txt         开始巡逻服务应答
  check_emergency.py             读锁存值的等价订阅脚本
  /tmp/patrol_safety.log         完整仿真日志（未入库，关键行已提取）
