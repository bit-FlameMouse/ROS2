# base_interfaces_demo —— 自定义接口功能包

本包只定义 ROS2 接口（消息 / 服务 / 动作），**不包含任何可执行节点**。
其他功能包使用本包在编译时自动生成的 C++ 头文件：

- `topic_cpp` 使用 `Student`（消息）
- `service_cpp` 使用 `AddInts`（服务）
- `action_cpp` 使用 `Progress`（动作）
- `exercise_cpp` 使用 `Distance`（服务）、`Nav`（动作）

## 接口一览

| 类型 | 文件 | 接口名 | 字段说明 |
| --- | --- | --- | --- |
| 消息 | `msg/Student.msg` | `base_interfaces_demo/msg/Student` | `string name` 姓名；`int32 age` 年龄；`float64 height` 身高 |
| 服务 | `srv/AddInts.srv` | `base_interfaces_demo/srv/AddInts` | 请求：`int32 num1`、`int32 num2`；响应：`int32 sum` |
| 服务 | `srv/Distance.srv` | `base_interfaces_demo/srv/Distance` | 请求：`float32 x`、`y`、`theta`；响应：`float32 distance` |
| 动作 | `action/Progress.action` | `base_interfaces_demo/action/Progress` | 目标：`int64 num`；结果：`int64 sum`；反馈：`float64 progress` |
| 动作 | `action/Nav.action` | `base_interfaces_demo/action/Nav` | 目标：`goal_x`、`goal_y`、`goal_theta`；结果：`turtle_x`、`turtle_y`、`turtle_theta`；反馈：`distance`（均为 float32） |

> 接口写好后，由 `CMakeLists.txt` 里的 `rosidl_generate_interfaces()` 在编译时生成代码，例如
> `base_interfaces_demo/msg/student.hpp`、`base_interfaces_demo/srv/distance.hpp`、`base_interfaces_demo/action/nav.hpp`。

## 构建

```bash
cd ~/ROS2/ros2_learning_plumbing
colcon build --packages-select base_interfaces_demo
source install/setup.bash
```

## 验证接口是否生成成功

```bash
ros2 interface package base_interfaces_demo   # 列出本包全部接口
ros2 interface show base_interfaces_demo/msg/Student
ros2 interface show base_interfaces_demo/srv/AddInts
ros2 interface show base_interfaces_demo/srv/Distance
ros2 interface show base_interfaces_demo/action/Progress
ros2 interface show base_interfaces_demo/action/Nav
```

## 注意事项

- 修改任何 `.msg/.srv/.action` 文件后，依赖它的功能包（topic_cpp、service_cpp、action_cpp、exercise_cpp）都需要重新编译。
- 只定义接口的包必须声明 `<member_of_group>rosidl_interface_packages</member_of_group>` 并依赖
  `rosidl_default_generators`（编译）和 `rosidl_default_runtime`（运行），这些都已配置。
