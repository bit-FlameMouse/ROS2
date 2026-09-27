# topic_cpp —— 话题通信（发布/订阅）学习包

演示 ROS2 最常用的通信方式：**话题（Topic）**。
发布方按固定频率向话题发消息，订阅方收到消息后自动执行回调；
双方通过"话题名 + 消息类型"匹配，彼此不需要知道对方是谁。

## 节点一览

| 可执行文件 | 节点名 | 作用 |
| --- | --- | --- |
| `demo01_talker_str` | `minimal_publisher` | 发布方：每 0.5s 向话题 `topic` 发布一条 `std_msgs/msg/String`，内容为 `Hello, world! N`（N 自增） |
| `demo02_listener_str` | `minimal_subscriber` | 订阅方：订阅话题 `topic`，把收到的字符串打印到终端 |
| `demo03_talker_student` | `student_publisher` | 发布方：每 0.5s 向话题 `topic_stu` 发布一条自定义消息 `Student`（张三 / 年龄自增 / 身高 1.65） |
| `demo04_listener_student` | `student_subscriber` | 订阅方：订阅话题 `topic_stu`，打印学生姓名、年龄、身高 |

## 启动方式

本包没有 launch 文件，直接用 `ros2 run` 启动（发布方和订阅方分别放在两个终端）。

内置字符串消息示例：

```bash
# 终端 1：发布方
ros2 run topic_cpp demo01_talker_str

# 终端 2：订阅方
ros2 run topic_cpp demo02_listener_str
```

自定义消息示例（需要先编译 `base_interfaces_demo`）：

```bash
# 终端 1：发布方
ros2 run topic_cpp demo03_talker_student

# 终端 2：订阅方
ros2 run topic_cpp demo04_listener_student
```

## 实现效果

- 字符串示例：
  - 发布方终端每 0.5s 打印 `发布的消息：'Hello, world! 0'`、`'Hello, world! 1'` …
  - 订阅方终端同步打印 `订阅的消息： 'Hello, world! 0'` …
- 自定义消息示例：
  - 发布方打印 `学生信息:name=张三,age=0,height=1.65`，年龄每 0.5s 加 1；
  - 订阅方打印 `订阅的学生消息：name=张三,age=0,height=1.65`。

## 常用调试命令

```bash
ros2 topic list                 # 查看所有话题
ros2 topic echo /topic          # 在终端实时显示话题内容
ros2 topic info /topic          # 查看消息类型和发布/订阅数量
ros2 topic hz /topic            # 查看发布频率
```

## 注意事项

- 话题名、消息类型必须完全一致，双方才能通信。
- `demo03/demo04` 依赖 `base_interfaces_demo` 生成的自定义消息，编译顺序上接口包要先编译。
