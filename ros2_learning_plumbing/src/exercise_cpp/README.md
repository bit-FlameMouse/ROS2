# exercise_cpp —— 综合案例练习包

把**话题、服务、动作、参数**四种通信方式综合运用在 turtlesim 乌龟上，每个可执行文件对应一个练习。
自定义接口（`Distance`、`Nav`）来自 `base_interfaces_demo`，所以编译本包前要先编译接口包。

## 节点一览

| 可执行文件 | 节点名 | 练到的通信方式 | 作用 |
| --- | --- | --- | --- |
| `exe01_pub_sub` | `demo01_pub_sub` | 话题 | 订阅窗口1乌龟的位姿 `/turtle1/pose`，处理后把速度指令发布到窗口2乌龟的话题 `/t2/turtle1/cmd_vel`，让两只乌龟同步做镜像运动 |
| `exe02_server` | `exe_distance_server` | 话题 + 服务 | 订阅 `/turtle1/pose` 保存乌龟坐标；提供 `distance` 服务，计算乌龟到目标点 `(x, y)` 的直线距离 |
| `exe03_client` | `exe_distance_client` | 服务 | 从命令行传入目标点 `(x, y, theta)`，调用 `distance` 服务并打印乌龟与目标点的距离 |
| `exe04_action_server` | `exe_nav_action_server` | 话题 + 动作 | 提供 `nav` 动作：订阅乌龟位姿、发布速度指令，控制乌龟向目标点 `(x, y)` 运动，连续反馈剩余距离，支持取消 |
| `exe05_action_client` | `exe_nav_action_client` | 动作 | 从命令行传入目标点 `(x, y, theta)`，向 `nav` 动作发送目标，打印剩余距离反馈和乌龟最终位姿 |
| `exe06_param` | `exe_param_client` | 参数 | 参数客户端：循环修改 turtlesim 的参数 `background_r`，让窗口背景颜色渐变 |

## Launch 文件一览

launch 文件已通过 `install(DIRECTORY launch ...)` 安装，统一用
`ros2 launch exercise_cpp <文件名>` 启动：

| launch 文件 | 启动内容 | 使用说明 |
| --- | --- | --- |
| `exe_pub_sub_launch.py` | 窗口1乌龟 + 窗口2乌龟 + `exe01_pub_sub` | 自动完成"启动两只乌龟 → 窗口2乌龟掉头 180° → 启动模仿节点" |
| `exe_server_launch.py` | 乌龟 + `exe02_server` | 测距服务端，**先运行它** |
| `exe_client_launch.py` | 生成目标乌龟 t2 + `exe03_client` | 测距客户端，需要先运行 `exe_server_launch.py` |
| `exe_action_server_launch.py` | 乌龟 + `exe04_action_server` | 导航动作服务端，**先运行它** |
| `exe_action_client_launch.py` | 生成目标乌龟 t3 + `exe05_action_client` | 导航动作客户端，需要先运行 `exe_action_server_launch.py` |
| `exe_param.py` | 乌龟 + `exe06_param` | 动态修改背景色 |

## 各练习的启动方式与效果

### 练习 1：话题 —— 乌龟镜像跟随（exe01）

```bash
ros2 launch exercise_cpp exe_pub_sub_launch.py
# 另开一个终端遥控窗口1的乌龟（按键前先点一下该终端）：
ros2 run turtlesim turtle_teleop_key
```

效果：窗口2的乌龟会同步模仿窗口1的乌龟运动；方向键控制窗口1，窗口2 做左右镜像动作。

手动启动方式（等价于 launch 的步骤）：

```bash
ros2 run turtlesim turtlesim_node                                     # 窗口1
ros2 run turtlesim turtlesim_node --ros-args -r __ns:=/t2             # 窗口2
ros2 action send_goal /t2/turtle1/rotate_absolute turtlesim/action/RotateAbsolute "{theta: 3.14}"
ros2 run exercise_cpp exe01_pub_sub
```

### 练习 2：服务 —— 计算乌龟到目标点的距离（exe02 + exe03）

```bash
# 终端 1：先启动测距服务端
ros2 launch exercise_cpp exe_server_launch.py
# 终端 2：生成目标乌龟 t2 并发送目标点
ros2 launch exercise_cpp exe_client_launch.py
```

效果：客户端打印 `两只乌龟相距X.XX米。`；服务端打印目标坐标与距离。

手动启动方式：

```bash
ros2 run exercise_cpp exe03_client 8.54 9.54 0.0
# 或者用命令行调用服务：
ros2 service call /distance base_interfaces_demo/srv/Distance "{x: 8.54, y: 9.54, theta: 0.0}"
```

### 练习 3：动作 —— 控制乌龟导航到目标点（exe04 + exe05）

```bash
# 终端 1：先启动动作服务端
ros2 launch exercise_cpp exe_action_server_launch.py
# 终端 2：生成目标乌龟 t3 并发送导航目标
ros2 launch exercise_cpp exe_action_client_launch.py
```

效果：窗口1的乌龟自动驶向 `(8.54, 9.54)`（生成的 t3 只是视觉上的目标标记）；
客户端不断打印 `距离目标点还有 X.XX 米。`，到达后打印乌龟最终坐标和航向。

手动启动方式：

```bash
ros2 run exercise_cpp exe05_action_client 8.54 9.54 0.0
```

### 练习 4：参数 —— 动态修改乌龟背景色（exe06）

```bash
ros2 launch exercise_cpp exe_param.py
```

效果：turtlesim 窗口背景的红色分量在 0~255 之间循环变化，背景颜色不断渐变。

手动启动方式：

```bash
ros2 run exercise_cpp exe06_param
ros2 param get /turtlesim background_r     # 另开终端查看背景红色分量
```

## 编译

```bash
cd ~/ROS2/ros2_learning_plumbing
colcon build --packages-select base_interfaces_demo exercise_cpp
source install/setup.bash
```

## 命令行参数说明

| 节点 | 参数 | 说明 |
| --- | --- | --- |
| `exe03_client` | `<x> <y> <theta>` | 目标点坐标与航向，例如 `8.54 9.54 0.0`（服务端只用到 x、y） |
| `exe05_action_client` | `<x> <y> <theta>` | 目标点坐标与航向，例如 `8.54 9.54 0.0` |

## 注意事项

- 先编译接口包 `base_interfaces_demo`，否则本包找不到 `distance.hpp`、`nav.hpp`。
- 服务/动作类练习都是"先服务端、后客户端"的顺序；客户端启动时会在超时前等待服务端上线。
- 动作服务端的工作线程在退出时会被回收，Ctrl+C 退出不会崩溃。
