# service_cpp —— 服务通信（请求/响应）学习包

服务用于"一问一答"：客户端发一次请求，服务端处理后返回一次响应，
适合不需要持续数据流的场景（例如查询、计算）。

本包使用自定义服务接口 `AddInts`（两数求和）。

## 节点一览

| 可执行文件 | 节点名 | 作用 |
| --- | --- | --- |
| `demo01_server` | `minimal_service` | 服务端：提供 `add_ints` 服务，收到两个整数后返回它们的和 |
| `demo02_client` | `minimal_client` | 客户端：从命令行读取两个整数，请求 `add_ints` 服务并打印结果 |

## 启动方式

```bash
# 终端 1：启动服务端（保持运行）
ros2 run service_cpp demo01_server

# 终端 2：启动客户端并传入两个整数
ros2 run service_cpp demo02_client 3 5
```

也可以不用客户端，直接用命令行调用服务：

```bash
ros2 service list                                   # 查看所有服务
ros2 service type /add_ints                         # 查看服务类型
ros2 service call /add_ints base_interfaces_demo/srv/AddInts "{num1: 3, num2: 5}"
```

## 实现效果

- 服务端启动打印：`add_ints 服务端启动完毕，等待请求提交...`
- 客户端发送请求后：
  - 服务端打印：`请求数据:(3,5),响应结果:8`
  - 客户端打印：`请求正常处理`、`响应结果:8!`
- 若服务端还没启动，客户端会循环打印 `服务连接中，请稍候...`，直到连上或按 Ctrl+C。

## 注意事项

- 客户端参数个数是 3 个（程序名 + 两个整数），参数不对会提示用法并以非 0 退出。
- 服务名 `add_ints` 必须客户端与服务端一致。
- 本包依赖 `base_interfaces_demo`，需要先编译接口包。
