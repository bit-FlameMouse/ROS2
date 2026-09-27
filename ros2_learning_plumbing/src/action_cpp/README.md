# action_cpp —— 动作（Action）学习包

动作 = 服务 + 连续反馈，适合"耗时任务"：

- **目标（Goal）**：客户端要做的任务，例如 `num=10`
- **反馈（Feedback）**：执行过程中不断汇报进度，例如 `30%`
- **结果（Result）**：任务结束后的最终结果，例如 `sum=55`
- 支持任务执行过程中**取消**

本包使用自定义动作接口 `Progress`：计算 `1+2+...+num`。

## 节点一览

| 可执行文件 | 节点名 | 作用 |
| --- | --- | --- |
| `demo01_action_server` | `minimal_action_server` | 动作服务端：提供 `get_sum` 动作，以 10Hz 频率累加并连续反馈进度，支持取消；耗时任务放入新线程执行，退出时等待线程回收 |
| `demo02_action_client` | `minimal_action_client` | 动作客户端：向 `get_sum` 发送 `num=10`，打印目标响应、连续进度和最终结果 |

## 启动方式

```bash
# 终端 1：启动动作服务端（保持运行）
ros2 run action_cpp demo01_action_server

# 终端 2：启动动作客户端
ros2 run action_cpp demo02_action_client
```

也可以用命令行直接发目标（`--feedback` 显示连续反馈）：

```bash
ros2 action list
ros2 action send_goal /get_sum base_interfaces_demo/action/Progress "{num: 10}" --feedback
```

## 实现效果

- 服务端：`接收到动作客户端请求，请求数字为 10` → `开始执行任务` →
  每 0.1s 打印 `连续反馈中，进度：0.10` … `1.00` → `任务完成！`
- 客户端：`目标被接收，等待结果中` → 每 0.1s 打印 `当前进度: 10%` … `100%` →
  `任务执行完毕，最终结果: 55`
- 发送目标 `num=0` 会被服务端拒绝；发送取消请求则任务提前结束并返回当前累加结果。

## 注意事项

- 服务端工作线程必须保存到成员变量并在析构时 `join()`，**不要用 `detach()`**：
  否则按 Ctrl+C 退出时线程可能访问已销毁的动作服务端，导致进程崩溃。
- `handle_goal` 中未使用的 `uuid` 用 `(void)uuid;` 消除 `-Wall -Wextra` 告警。
- 本包依赖 `base_interfaces_demo`，需要先编译接口包。
