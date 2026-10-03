# ROS 2 仿真巡逻机器人系统（ros2-patrol-robot-sim）

> 基于 **ROS 2 Humble + Gazebo Classic 11 + TurtleBot3** 的移动机器人仿真系统集成项目。
> 主开发语言 **C++17**，运行平台 **Ubuntu 22.04 LTS**。

本项目把「仿真环境 → 传感器 → SLAM 建图 → AMCL 定位 → Nav2 导航栈 → 自研任务层」串成一条完整链路，
核心自研部分是 **C++ 编写的巡逻任务节点（patrol_node）** 与 **激光安全守护节点（safety_guard_node）**。

---

## 1. 项目定位

| 维度 | 说明 |
| --- | --- |
| 项目类型 | 系统集成型作品（非算法创新型） |
| 交付目标 | 3 个工作日内完成从零到可演示、可讲解、可复现 |
| 核心能力证明 | 多模块联调、TF/坐标系、QoS、生命周期、Action 通信、参数化设计、C++ 工程化 |
| 对标岗位 | 机器人 TPM / 系统工程师 / 具身智能集成工程师 / ROS 开发（入门级） |

**一句话概括**：基于成熟的 SLAM + Nav2 开源栈做系统集成，在其上叠加自研的 C++ 任务层，
让仿真机器人在已知地图中自主循环巡逻，并具备紧急避障停车能力。

---

## 2. 功能一览

| 编号 | 功能 | 实现方式 |
| --- | --- | --- |
| F1 | Gazebo 仿真世界与机器人 | `turtlebot3_gazebo`（TurtleBot3 Burger） |
| F2 | 键盘遥控 | `turtlebot3_teleop` / `teleop_twist_keyboard` |
| F3 | SLAM 建图与地图保存 | `slam_toolbox`（备选 `cartographer`）+ `nav2_map_server` |
| F4 | 定位 | `nav2_amcl`（基于已保存栅格地图） |
| F5 | 单点导航 | Nav2 `navigate_to_pose` Action |
| F6 | **航点循环巡逻（自研）** | `patrol_robot_core/patrol_node`（C++ 事件驱动状态机） |
| F7 | 巡逻模式控制 | 自定义 `SetPatrolMode` 服务（START/PAUSE/RESUME/STOP/RESET） |
| F8 | 巡逻状态发布 | 自定义 `PatrolStatus` 消息 |
| F9 | 航点可视化 | `visualization_msgs/MarkerArray`（RViz2） |
| F10 | **激光安全守护（自研）** | `patrol_robot_core/safety_guard_node`（前向扇区 + 迟滞） |
| F11 | 失败重试与超时保护 | 巡逻节点内部重试计数 + 全局超时 |
| F12 | 全参数化配置 | YAML 参数文件，一键 launch |

### 明确不做的事（Out of Scope）

- ✗ 自己实现 SLAM / 路径规划 / 局部控制器算法
- ✗ 自己写 MPC、强化学习、端到端学习
- ✗ 真实硬件、机械臂 MoveIt2、多机协同
- ✗ 自定义 URDF 机器人模型（列入 v0.2 迭代）

---

## 3. 最终实现效果

> 本节描述**代码实现完成后（v0.2）系统实际呈现的样子**。所有现象都可复现、可验收，
> 对应 [01 需求规格说明书](docs/01-需求规格说明书.md) 的验收标准 AC-01 ~ AC-12，
> 演示脚本见 [12 演示脚本与验收清单](docs/12-演示脚本与验收清单.md)。

### 3.1 一句话效果

**一条命令启动，Gazebo 里的 TurtleBot3 在已建好的地图上自主循环巡逻**：
按顺序前往每个航点、到点停留、走完全部后进入下一轮，全程无需人工干预；
RViz 中航点标记随进度由灰变黄再变绿；机器人正前方出现障碍物时立即停车，
障碍物移开后自动恢复；巡逻过程可随时暂停、继续、停止，状态通过话题实时可观测。

### 3.2 操作 → 现象对照（最直观的效果说明）

| 你做什么 | 你会看到什么 |
| --- | --- |
| `ros2 launch patrol_robot_bringup patrol.launch.py` | Gazebo 弹出世界与 TurtleBot3；RViz2 显示地图、激光点云、路径与航点标记；两个自研节点**延迟约 8 s 启动**（等 Nav2 生命周期激活完成） |
| 在 RViz 点 `2D Pose Estimate` 给初始位姿 | 红色激光点云与地图墙线对齐，AMCL 定位收敛（位姿偏差 < 0.3 m） |
| `ros2 service call /patrol/set_mode ... "{mode: 0}"` | 返回 `success=True, current_mode=1`；机器人开始驶向第 1 个航点；RViz 中该航点由**灰变黄** |
| 观察 `ros2 topic echo /patrol/status` | 1 Hz 输出：`mode` 在 1(MOVING)/2(WAITING) 间切换，`distance_remaining` 持续递减，`current_waypoint_index` 递增 |
| 机器人到达第 1 个航点 | 状态转 `WAITING` 并停留 2 s；该航点 Marker 由**黄变绿**，`completed_waypoints` +1；随后自动驶向第 2 个航点 |
| 走完全部 3 个航点 | `current_loop` +1，索引归 0，进入下一轮循环（`loop_count=0` 时为无限循环） |
| `mode: 1`（PAUSE）/ `mode: 2`（RESUME） | 机器人立即停车（取消当前导航目标）并**保留航点索引**；继续后：路上暂停 → 从同一航点重新发起导航；停留期暂停 → 回到停留计时（重计满停留时间、不重复计数） |
| 在 Gazebo 中用 `Insert` 在机器人正前方约 0.2 m 放一个 Box | 约 0.2 s 内 `/safety/emergency_stop` 变 `true`，机器人停车，状态转 `SAFETY_HOLD`(4)，日志出现 WARN |
| 右键删除该 Box | 前方距离恢复超过 0.6 m 后标志复位，机器人**自动继续**原航点巡逻 |
| 把某个航点故意设到墙里（不可达） | 单航点重试 `max_retries` 次，日志记录超时/中止；随后按 `skip_failed_waypoint=true` 跳过该点继续巡逻 |
| 注入非法安全参数（如 `stop_distance=0.8 > resume_distance=0.6`） | 节点**拒绝启动**：`FATAL` 日志 + 非零退出码（不静默降级） |
| 连续运行 30 分钟 | 无崩溃、无内存持续增长、无 TF 中断、无 `/cmd_vel` 抖动 |

### 3.3 交付物形态（最终能拿出什么）

| 类别 | 具体内容 |
| --- | --- |
| 可运行系统 | 3 个功能包（`patrol_interfaces` / `patrol_robot_core` / `patrol_robot_bringup`），`colcon build` 零警告通过 |
| 自研代码 | 2 个 C++ 节点 + 4 个类（`WaypointLoader`、`NavClient`、`PatrolStateMachine`、`SafetyEvaluator`） |
| 演示视频 | ≤ 3 分钟 MP4（含讲解音轨），完整呈现「建图 → 定位 → 导航 → 巡逻 → 安全守护」 |
| 地图资产 | `maps/tb3_world.pgm` + `.yaml`（随仓库提交，是项目输入资产） |
| 测试证据 | `colcon test` 全绿 + 08 文档执行记录表 |
| 文档 | 18 份（README + `docs/00 ~ 17`），含 81 条架构规范条款 |
| 工程化脚本 | `install_deps.sh` / `build.sh` / `record_demo.sh` / `arch_check.sh`（架构约束自动检查）/ `scripts/simtest/`（上机测试脚本集） |

### 3.4 可量化验收指标

| 维度 | 指标 | 对应 |
| --- | --- | --- |
| 环境复现 | 按 03 文档搭建一次成功（允许 1 次排查） | AC-01 |
| 建图 | 保存地图与 Gazebo 世界障碍布局一致 | AC-03 |
| 定位 | AMCL 位姿偏差 < 0.3 m | AC-04 |
| 单点导航 | 5/5 目标点成功到达 | AC-05 |
| 自动巡逻 | 3 航点 × 2 轮全自动完成，无人工干预 | AC-06 |
| 安全守护 | 障碍触发停车，移除后自动恢复 | AC-08 |
| 实时性 | tick 100 ms；状态发布 ≥ 1 Hz；`/scan` 处理延迟 < 50 ms | NFR-02 |
| 资源占用 | 巡逻节点 CPU < 5%（不含仿真）；内存增量 < 50 MB | NFR-03 |
| 稳定性 | 30 min 连续运行无崩溃、无内存泄漏增长 | NFR-05 / AC-10 |
| 代码质量 | 零编译警告；`ament_lint` 通过；单元测试覆盖率 ≥ 60% | NFR-06 / NFR-10 |

### 3.5 能力边界（诚实说明）

**能做**：仿真环境下的完整「建图 → 定位 → 导航 → 任务编排 → 安全监控」链路集成，以及自研 C++ 任务层。

**不做 / 做不到**：

- ✗ 不做算法自研——SLAM、定位、规划、控制全部使用成熟开源栈（这是**有意的工程选择**，见 02 文档 ADR-01）
- ✗ 未接真实硬件——上述所有指标均为 **Gazebo 仿真结果**，不是实车实测数据
- ✗ 未做视觉感知（列入 v0.3）、多机协同（列入 v0.4）；机械臂等本就不在本项目范围（见 §2）
- ✗ 参数为启动期参数，暂不支持运行时热改；地图更新仍需重新建图（技术债见 [14 文档](docs/14-变更记录与迭代规划.md)）

> 上述边界在 [13 答辩讲稿](docs/13-答辩讲稿与常见问答.md) 中有对应的话术处理：
> **如实说明集成范围与自研比例，比夸大能力更可信。**

---

## 4. 系统架构（速览）

```
┌──────────────────────────────────────────────────────────────┐
│ 交互层      RViz2 可视化 / 键盘遥控 / ros2 CLI / 自定义服务     │
├──────────────────────────────────────────────────────────────┤
│ 任务层(自研) patrol_node(巡逻状态机)  safety_guard_node(安全守护)│
├──────────────────────────────────────────────────────────────┤
│ 导航层      Nav2 (bt_navigator / planner / controller / AMCL) │
│ 建图层      slam_toolbox（离线建图） → 栅格地图 .pgm/.yaml      │
├──────────────────────────────────────────────────────────────┤
│ 抽象层      /scan  /odom  /tf  /tf_static  /cmd_vel           │
├──────────────────────────────────────────────────────────────┤
│ 仿真层      Gazebo Classic 11 + turtlebot3_gazebo + TB3 Burger│
└──────────────────────────────────────────────────────────────┘
```

详细设计见 [`docs/02-系统架构设计说明书.md`](docs/02-系统架构设计说明书.md)，
架构上的强制约束见 [`docs/16-架构规范与设计约束.md`](docs/16-架构规范与设计约束.md)。

---

## 5. 工作空间结构

```
ros2-patrol-robot-sim/
├── README.md
├── docs/                              # 全套开发文档（见下表）
├── scripts/
│   ├── install_deps.sh                # 依赖安装
│   ├── build.sh                       # 一键编译
│   └── record_demo.sh                 # 演示录屏辅助
└── src/
    ├── patrol_interfaces/             # 自定义 msg / srv
    ├── patrol_robot_core/             # C++ 核心节点（自研）
    │   ├── include/patrol_robot_core/
    │   │   ├── waypoint_loader.hpp
    │   │   ├── nav_client.hpp
    │   │   └── patrol_state_machine.hpp
    │   └── src/
    │       ├── patrol_node.cpp
    │       ├── safety_guard_node.cpp
    │       ├── waypoint_loader.cpp
    │       └── nav_client.cpp
    └── patrol_robot_bringup/          # launch / config / maps / rviz
        ├── launch/  config/  maps/  rviz/
```

---

## 6. 快速开始

```bash
# 0. 前提：Ubuntu 22.04 + ROS 2 Humble（安装见 docs/03）
source /opt/ros/humble/setup.bash

# 1. 安装依赖
bash scripts/install_deps.sh

# 2. 编译
cd ~/ros2_ws && colcon build --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=Release
source install/setup.bash

# 3. 建图（第一次需要做一次，得到 maps/tb3_world）
ros2 launch patrol_robot_bringup mapping.launch.py      # 另开终端跑 teleop 遥控走一圈
ros2 run nav2_map_server map_saver_cli -f ~/ros2_ws/src/patrol_robot_bringup/maps/tb3_world

# 4. 一键巡逻演示
ros2 launch patrol_robot_bringup patrol.launch.py
```

完整流程与排错见 [`docs/09-构建部署与运行手册.md`](docs/09-构建部署与运行手册.md)，
上机测试步骤见 [`docs/17-仿真验收测试操作手册.md`](docs/17-仿真验收测试操作手册.md)。

> **路径说明**：以上命令在“工作空间根目录”（含 `src/` 的那一层）执行。
> 09 文档用 `~/ros2_ws` 描述推荐布局；若仓库目录直接作为工作空间（本机为 `/home/lich/ROS2/ros`），
> 请把 `~/ros2_ws` 替换为该目录（`scripts/` 与 `docs/` 位于仓库根）。

---

## 7. 开发文档清单

| 编号 | 文档 | 内容概要 |
| --- | --- | --- |
| 00 | [文档索引与阅读指南](docs/00-文档索引与阅读指南.md) | 文档地图、阅读路径、术语表入口 |
| 01 | [需求规格说明书](docs/01-需求规格说明书.md) | 范围、功能/非功能需求、验收标准、需求追踪矩阵 |
| 02 | [系统架构设计说明书](docs/02-系统架构设计说明书.md) | 分层架构、包划分、技术选型决策记录、时序图 |
| 03 | [环境搭建与依赖清单](docs/03-环境搭建与依赖清单.md) | 虚拟机方案、Ubuntu 22.04、ROS 2 Humble、依赖包表 |
| 04 | [详细设计说明书](docs/04-详细设计说明书.md) | 每个 C++ 节点/类的设计、状态机、完整代码清单 |
| 05 | [接口设计说明书](docs/05-接口设计说明书.md) | 话题/服务/动作/参数定义表、QoS、错误码 |
| 06 | [坐标变换与 TF 设计](docs/06-坐标变换与TF设计.md) | 坐标系定义、TF 树、时间戳规范、TF 故障排查 |
| 07 | [C++ 开发规范与编码指南](docs/07-C++开发规范与编码指南.md) | 命名、内存、参数、日志、格式、Git 规范 |
| 08 | [测试计划与用例设计](docs/08-测试计划与用例设计.md) | 单元/集成/系统/性能测试用例与缺陷管理 |
| 09 | [构建部署与运行手册](docs/09-构建部署与运行手册.md) | 编译、四条启动链路、命令速查、排错手册 |
| 10 | [项目计划与三日排期](docs/10-项目计划与三日排期.md) | WBS、三日时间盒、里程碑、降级预案 |
| 11 | [风险登记册与避坑指南](docs/11-风险登记册与避坑指南.md) | 风险表、20 条技术坑、应急方案 |
| 12 | [演示脚本与验收清单](docs/12-演示脚本与验收清单.md) | 3 分钟演示分镜、验收勾选表、录屏规范 |
| 13 | [答辩讲稿与常见问答](docs/13-答辩讲稿与常见问答.md) | 电梯演讲、技术讲解、高频问题标准答案 |
| 14 | [变更记录与迭代规划](docs/14-变更记录与迭代规划.md) | 版本历史、Roadmap、技术债清单 |
| 15 | [术语表与参考资料](docs/15-术语表与参考资料.md) | 中英术语对照、官方文档与参考实现 |
| 16 | [架构规范与设计约束](docs/16-架构规范与设计约束.md) | 81 条强制架构条款（AS-01~AS-81）、架构适应度函数、架构评审清单 |
| 17 | [仿真验收测试操作手册](docs/17-仿真验收测试操作手册.md) | 无界面（headless 服务器）环境的一步步上机测试步骤、文本留证清单、环境注意事项（RMW/daemon/离线加固）、实测记录 |

> **02 / 16 / 07 的区别**：02 讲「架构**长什么样**、为什么这么设计」，16 讲「架构上**必须**怎么做、不许怎么搭」，
> 07 讲「每一行 C++ **怎么写**」。改设计回 02，搭结构看 16，写代码看 07。

---

## 8. 环境与版本基线

| 组件 | 版本 | 备注 |
| --- | --- | --- |
| Ubuntu | 22.04.5 LTS (Jammy) | 唯一支持 Humble 的 LTS |
| ROS 2 | Humble Hawksbill | LTS，2027-05 EOL |
| Gazebo | Classic 11 | Humble 下 TurtleBot3 仿真使用 Classic，**不要混用 Ignition/gz-sim** |
| TurtleBot3 | Burger | `TURTLEBOT3_MODEL=burger` |
| C++ 标准 | C++17 | `-std=c++17` |
| 构建系统 | colcon + ament_cmake | `--symlink-install` |
| 中间件 | Fast DDS（默认）/ CycloneDDS（推荐） | `RMW_IMPLEMENTATION=rmw_cyclonedds_cpp` |

---

## 9. 状态

当前阶段：**v0.2.3 —— 无界面验收走查（17 手册）进行中；走查发现的世界地面缺失问题已修复（04 §14.3）**。

| 项 | 状态 |
| --- | --- |
| 三个功能包源码（2 个 C++ 节点 + 4 个类） | ✅ 完成 |
| 自定义接口（1 msg + 1 srv） | ✅ 完成 |
| 单元测试（36 个用例，覆盖 08 文档 TC-U-01~06） | ✅ `colcon test` 全绿 |
| 编译零警告 + `ament_lint`（cpplint/uncrustify/xmllint/flake8…） | ✅ 通过 |
| 5 条 launch 链路 + 3 个参数文件 + 2 个 RViz 配置 | ✅ 完成 |
| 地图资产 `maps/tb3_world.{pgm,yaml}` | ✅ 随仓库提交（SLAM 实采，见下） |
| 工程化脚本（依赖/构建/录制/架构检查/地图生成/上机测试） | ✅ 完成 |
| 仿真全链路实测（Gazebo + Nav2） | ✅ 已完成一轮 headless 验证（TC-I-01/02/03/04/05、TC-P-03）；2026-10-01 离线加固后启动复验通过（~20 s 全链路就绪）；TC-S/TC-P 其余项按 17 文档执行 |

> **地图说明**：`maps/tb3_world.pgm` 由 **SLAM Toolbox 实采**（`mapping.launch.py` +
> `scripts/simtest/drive_perimeter.py` 外环闭环巡线 13/13 段，2026-09-30），与仿真世界障碍布局一致；
> `scripts/generate_map_from_world.py` 保留为无仿真环境下的离线重建工具（10 文档 §6 降级预案 P2）。
> 上机测试步骤与环境注意事项见 [17 仿真验收测试操作手册](docs/17-仿真验收测试操作手册.md)。

> 文档与代码不同步时，以文档为准——先改文档再改代码。
