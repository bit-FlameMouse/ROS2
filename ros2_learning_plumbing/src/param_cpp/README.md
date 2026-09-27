# param_cpp —— 参数（Parameter）学习包

参数是节点持有的"键值对"配置，可随时查询和修改，常用于运行中调参。
本包演示参数服务端声明参数、参数客户端增/查/改/删的完整流程。

## 节点一览

| 可执行文件 | 节点名 | 作用 |
| --- | --- | --- |
| `demo01_param_server` | `minimal_param_server` | 参数服务端：声明 `car_type`、`height`、`wheels` 三个普通参数和动态参数 `temp_param`；注册修改回调校验数据（`height` 必须 > 0）；启动时打印全部参数 |
| `demo02_param_client` | `minimal_param_client` | 参数客户端：依次演示 增（会被拒绝）、查、改、删 |

## 启动方式

```bash
# 终端 1：启动参数服务端（保持运行）
ros2 run param_cpp demo01_param_server

# 终端 2：启动参数客户端，自动完成整套演示
ros2 run param_cpp demo02_param_client
```

## 实现效果

服务端启动打印：

```
参数声明完毕，当前值如下：
car_type = Tiger
height = 1.50
wheels = 4
temp_param = 100
```

客户端依次执行：

1. **增**：设置服务端未声明的 `width` → 失败，服务端拒绝新参数；
2. **查**：用 `has_parameter`、`list_parameters`、`get_parameter`、`describe_parameters`
   列出参数名、参数值、参数类型和说明；
3. **改**：把 `car_type` 改成 `Mouse`、`height` 改成 `1.75`、`wheels` 改成 `6`；
   再把 `height` 改成 `-1.0` → 被服务端回调拒绝并返回原因 `height 必须大于 0`；
4. **删**：删除动态参数 `temp_param` → 成功；删除普通参数 `car_type` → 失败（普通参数不可删除）。

## 命令行调试

```bash
ros2 param list                                          # 列出所有节点的参数
ros2 param get /minimal_param_server height              # 查询参数
ros2 param set /minimal_param_server height 1.8          # 修改参数
ros2 param describe /minimal_param_server height         # 查看参数类型和说明
ros2 param delete /minimal_param_server temp_param       # 删除动态参数
```

## 注意事项

- 只有服务端 `declare_parameter` 声明过的参数才能被客户端操作，客户端不能新增参数。
- 普通参数类型固定、声明后不可删除；要删除必须声明为动态参数（`dynamic_typing = true`）。
- 注册修改回调时返回的句柄（`OnSetParametersCallbackHandle`）必须保存为成员变量，否则回调会被注销。
