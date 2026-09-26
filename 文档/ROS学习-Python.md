# ROS2 Python 实现笔记

> 本文档由《ROS学习.md》拆分而来，只收录 Python 相关的内容。
> 语言无关的概念、通信机制、接口文件格式、C++ 实现、常用工具等，见《ROS学习.md》。

**目录**

- 一、开发环境（VSCode 插件）
- 二、节点（Python 写法）
- 三、功能包与配置文件（ament_python）
  - 3.1 package.xml 中的 Python 相关项
  - 3.2 新增节点 / 资源时怎么改
- 四、Python 实现（HelloWorld）
- 五、在 Python 中使用自定义接口
- 六、rclpy 常用接口
- 七、重名处理与时间 API（Python 写法）
  - 7.1 编码设置节点名称
  - 7.2 编码设置话题名称
  - 7.3 时间相关 API（Rate / Time / Duration）
- 附录 C：示例工程

---

## 一、开发环境（VSCode 插件）

开发 Python 节点，除通用插件外还需要：

- Python：官方 Python 开发插件

（Chinese、Msg Language Support、vscode-pdf、XML、YAML、URDF 等通用插件见《ROS学习.md》1.2）

## 二、节点（Python 写法）

编码规范：Node 节点必须以继承的方式进行（之前是直接实例化）。这种方式可以在一个进程内组织多个节点，对于提高通信非常有帮助。

示例工程的实际写法（`pkg_helloworld_py/node_helloworld_py.py`，完整说明见四、第 2 步）：

```python
import rclpy
from rclpy.node import Node

class MyNode(Node):
    def __init__(self):
        super().__init__("node_helloworld_py")  # 初始化节点，同时指定节点名

    def get_message(self):  # 定义一个函数，输出对应的信息
        self.get_logger().info("msg: hello world!")

def main():
    rclpy.init()
    node = MyNode()
    node.get_message()  # 调用对象方法，输出信息
    rclpy.shutdown()  # 回收资源

if __name__ == "__main__":
    main()
```

- 类名（`MyNode`）可以随意起，节点名由 `super().__init__()` 的参数决定，两者不必同名。
- 节点名有命名规则：只能包含字母、数字、下划线，且第一个字符必须是字母或下划线（见《ROS学习.md》2.2）。

资源释放相关说明（context 上下文对象）见《ROS学习.md》2.2。

## 三、功能包与配置文件（ament_python）

每个功能包里都有 `package.xml`，但「构建脚本」用哪个文件，取决于包的构建类型：

| 构建类型 | 构建脚本 | 适用场景 |
| --- | --- | --- |
| ament_python | `setup.py` | 纯 Python 节点包（没有 CMakeLists.txt） |

也就是说：**XML 文件（package.xml）两种包都有，CMake 文件只有 ament_cmake 包才有**（ament_cmake 与 CMakeLists.txt 见《ROS学习.md》2.3）。

### 3.1 package.xml 中的 Python 相关项

- 纯 Python 包的构建工具依赖是 `ament_python`，即 `<buildtool_depend>ament_python</buildtool_depend>`。
- 一句话：**代码里 `import` 了哪个包，就要在 `package.xml` 里声明对应的依赖**，否则运行时会找不到。
- 接口包还要加一行 `<member_of_group>rosidl_interface_packages</member_of_group>`（见《ROS学习.md》3.6.4）。

### 3.2 新增节点 / 资源时怎么改

**新增一个节点**

1. 在与包同名的目录下新建 `talker.py`，里面写好 `main()`。
2. 在 `setup.py` 的 `entry_points` 里加一行（各项含义见四、第 3 步）：

```python
entry_points={
    'console_scripts': [
        'node_helloworld_py = pkg_helloworld_py.node_helloworld_py:main',
        'talker = pkg_helloworld_py.talker:main',  # 新增的节点
    ],
},
```

3. 如果有新依赖，`package.xml` 里补 `<depend>...</depend>`。
4. 重新 `colcon build`。

**新增 launch 文件、配置、模型等资源**

这些文件**不会自动被打包**，必须在构建脚本里声明，否则 `ros2 launch` / `ros2 run` 在安装空间里找不到它们：在 `setup.py` 的 `data_files` 里加对应的 `(目标路径, [文件列表])` 项。

**几条通用规则**

- 只要「新增」了可执行文件、接口或资源文件，就必须在构建脚本里**登记一次**，否则新东西不会被打包。
- 改完 `package.xml` 或构建脚本后，**必须重新 `colcon build`** 才会生效。
- `package.xml` 的 `<name>`、`setup.py` 里的包名、文件夹名，三者必须一致。

## 四、Python 实现（HelloWorld）

和 C++ 的实现类似，不过依赖的是 `rclpy`。

**1. 创建功能包**

```bash
mkdir -p ros2_learning_python/src  # 创建一个工作空间，已经有了就不用创建了

cd ros2_learning_python/src  # 进入源码目录

# 调用 ROS2 的创建功能包命令
# --build-type 指定构建类型，--dependencies 添加依赖，--node-name 设置节点名称
# 下面是示例工程实际用的命令（包名 pkg_helloworld_py、节点名 node_helloworld_py）
ros2 pkg create pkg_helloworld_py --build-type ament_python --dependencies rclpy --node-name node_helloworld_py
```

**2. 编辑源文件**

包下面与包同名的目录下面的 node 名的 py 文件，这个就是主文件。编辑代码。

> 代码风格说明：示例工程里采用**自定义节点类**的写法（继承 `Node`），完整文件见附录 C 的 node_helloworld_py.py。

示例工程里的节点（`pkg_helloworld_py/node_helloworld_py.py`）：

```python
import rclpy
from rclpy.node import Node

class Mynode(Node):
    def __init__(self):
        super().__init__("node_helloworld_py")  # 初始化节点，同时指定节点名

    def send_helloworld(self):
        self.get_logger().info("msg: hello world!")


def main():
    rclpy.init()          # 初始化 ROS2
    node = Mynode()       # 创建节点对象
    node.send_helloworld()  # 调用对象方法，输出信息
    rclpy.shutdown()      # 释放资源


if __name__ == "__main__":
    main()
```

- 继承写法的好处见「二、节点（Python 写法）」：可以在一个进程内组织多个节点。
- 这个示例**没有调用 `rclpy.spin()`**，方法执行完程序就退出了；要持续收消息的节点才需要 spin。
- 名字对应关系：类名 `Mynode` 是随意起的，节点名 `node_helloworld_py` 由 `super().__init__()` 的参数决定。

**3. 编辑配置文件**

XML 文件、setup.py 文件。

注册入口：告诉构建系统，把某个 Python 函数包装成一个可以用命令运行的可执行程序。

```python
entry_points={
    'console_scripts': [
        'node_helloworld_py = pkg_helloworld_py.node_helloworld_py:main'
    ],
},
# ros2 run 时使用的名字 = Python包名.文件名:函数名
```

- node_helloworld_py（等号左边）：运行命令时用的可执行名
- pkg_helloworld_py：内层 Python 包目录
- node_helloworld_py（等号右边第一段）：node_helloworld_py.py
- main：node_helloworld_py.py 里的 main 函数

新增节点时在这里再加一行即可，详见 3.2。

**4. 编译**

- 先跳转到工作空间
- 然后进行 build

```bash
colcon build  # 使用 colcon 进行 build
colcon build --packages-select <pkg>  # 编译指定的包
```

**5. 运行**

- 同样跳转到工作空间
- 执行

```bash
. install/setup.bash  # 执行初始化脚本
ros2 run pkg_helloworld_py node_helloworld_py  # ros2 run <包名> <可执行名>
```

## 五、在 Python 中使用自定义接口

接口（msg / srv / action）的语法与注册方式见《ROS学习.md》3.6。使用方需要在 `package.xml` 中声明依赖：

```xml
<depend>base_interfaces_demo</depend>
```

之后在 Python 代码里导入（示例工程的接口包见《ROS学习.md》附录 A）：

```python
from base_interfaces_demo.msg import Student
```

## 六、rclpy 常用接口

`rclpy` 是 ROS2 官方的 Python 客户端库，与 C++ 的 `rclcpp` 一一对应，名字更短。下面按用途列一遍 Python 侧的实际写法（C++ 侧对应的 rclcpp 接口见《ROS学习.md》附录 B）。

### 6.1 生命周期与节点

| 接口 | 示例 | 作用 |
| --- | --- | --- |
| `import rclpy` | — | 导入客户端库 |
| `from rclpy.node import Node` | — | 导入节点基类 |
| `rclpy.init()` | `rclpy.init()` | 初始化 ROS2，必须最先调用 |
| `rclpy.shutdown()` | `rclpy.shutdown()` | 关闭通信、释放资源 |
| `rclpy.spin(node)` | `rclpy.spin(node)` | 进入事件循环，持续处理回调（示例工程没用，因为它只打印一次就退出） |
| `rclpy.spin_once(node)` | `rclpy.spin_once(node)` | 只处理一轮回调，常用于循环体里 |
| `Node` 子类 | `class Mynode(Node)` | 自定义节点必须继承 `Node` |
| `super().__init__(节点名)` | `super().__init__("node_helloworld_py")` | 初始化节点，参数就是节点名 |
| `rclpy.create_node(节点名)` | `rclpy.create_node("node_demo")` | 不写自定义类时，直接创建节点对象 |
| `node.get_logger()` | `self.get_logger().info("msg: hello world!")` | 打印日志，另有 `.warn()` / `.error()` / `.debug()` |
| `node.get_name()` / `node.get_namespace()` | — | 查询节点名、命名空间 |
| `node.destroy_node()` | `node.destroy_node()` | 显式销毁节点（不是必须，退出时会自动回收） |

### 6.2 话题通信

| 接口 | 示例 | 作用 |
| --- | --- | --- |
| `create_publisher(消息类型, 话题名, 队列长度)` | `self.pub = self.create_publisher(String, "topic", 10)` | 创建发布方 |
| `publish(消息)` | `self.pub.publish(msg)` | 发布消息 |
| `create_subscription(消息类型, 话题名, 回调, 队列长度)` | `self.sub = self.create_subscription(String, "topic", self.callback, 10)` | 创建订阅方 |
| 回调签名 | `def callback(self, msg):` | 参数就是收到的消息 |
| `create_timer(周期秒, 回调)` | `self.timer = self.create_timer(0.5, self.timer_callback)` | 创建定时器（注意 Python 里单位是**秒**，不是 `500ms`） |
| 消息类型导入 | `from std_msgs.msg import String` | 内置消息类型 |
| 自定义消息导入 | `from base_interfaces_demo.msg import Student` | 自定义消息类型 |

### 6.3 服务通信

| 接口 | 示例 | 作用 |
| --- | --- | --- |
| `create_service(接口类型, 服务名, 回调)` | `self.srv = self.create_service(AddInts, "add_ints", self.add_callback)` | 创建服务端 |
| 服务回调签名 | `def add_callback(self, request, response):` … `return response` | 注意 Python 里响应是**返回值**，不是出参 |
| `create_client(接口类型, 服务名)` | `self.cli = self.create_client(AddInts, "add_ints")` | 创建客户端 |
| `wait_for_service()` | `self.cli.wait_for_service()` | 等待服务端上线（可传 timeout_sec） |
| `call_async(请求)` | `self.future = self.cli.call_async(request)` | 异步发送请求，返回 future |
| 请求对象 | `req = AddInts.Request()`；`req.num1 = 3` | 组织请求 |
| 取响应 | `response = self.future.result()`；`response.sum` | 从 future 取响应 |

### 6.4 动作通信

| 接口 | 示例 | 作用 |
| --- | --- | --- |
| 导入 | `from rclpy.action import ActionServer, ActionClient` | 动作的 Python 类 |
| 创建服务端 | `ActionServer(self, Progress, "get_sum", execute_callback=self.execute_cb, ...)` | 创建动作服务端 |
| 目标处理 | `goal_callback` / `cancel_callback` / `handle_accepted_callback` | 三个回调，与 C++ 对应 |
| 创建客户端 | `ActionClient(self, Progress, "get_sum")` | 创建动作客户端 |
| 等待服务端 | `self.client.wait_for_server()` | 等待动作服务端上线 |
| 发送目标 | `self.client.send_goal_async(goal_msg, feedback_callback=...)` | 异步发目标 |
| 取结果 | `self.client.get_result_async(goal_handle)` | 异步取最终结果 |
| 发布反馈 | `goal_handle.publish_feedback(feedback)` | 服务端发布连续反馈 |
| 结束任务 | `goal_handle.succeed()` / `.canceled()` / `.abort()` | 对应 C++ 的同名方法 |

### 6.5 参数服务

| 接口 | 所在侧 | 作用 |
| --- | --- | --- |
| `declare_parameter(名字, 默认值)` | 服务端 | 声明参数 |
| `get_parameter(名字)` | 服务端 | 读取参数，返回 Parameter 对象，用 `.value` 取值 |
| `set_parameters([Parameter(名字, 值)])` | 服务端 | 修改自己的参数 |
| `add_on_set_parameters_callback(回调)` | 服务端 | 参数被改时触发 |
| `get_parameters([名字...])` 或 `AsyncParametersClient.get_parameters` | 客户端 | 读取目标节点的参数 |
| `AsyncParametersClient(self, "目标节点名")` | 客户端 | 创建参数客户端 |

几点说明：

- Python 的定时器单位是**秒**（`0.5`），别和 C++ 的时长对象写法混淆。
- 传回调时直接写 `self.方法名` 即可，回调所需的参数（订阅回调的消息、服务回调的请求与响应）由 ROS2 按位置自动传入，不需要额外做参数绑定。
- 服务回调必须 `return response`，这一步就是「发响应」。
- 打印浮点数、字符串直接用 f-string 即可，不需要 `%d`、`%s` 这类占位符。

## 七、重名处理与时间 API（Python 写法）

> 本节是《ROS学习.md》3.10.3、3.11.3、3.12 的 Python 对应写法。概念、三种重名处理方式的划分、以及 C++（rclcpp）写法见那几节，本节只整理 rclpy 的代码。

### 7.1 编码设置节点名称

在 rclpy 中，节点类的构造函数同样提供了设置节点名称（`node_name`）与命名空间（`namespace`）的参数（对应《ROS学习.md》3.10.3）：

```python
Node(node_name, *,
   context=None,
   cli_args=None,
   namespace=None,
   use_global_arguments=True,
   enable_rosout=True,
   start_parameter_services=True,
   parameter_overrides=None,
   allow_undeclared_parameters=False,
   automatically_declare_parameters_from_overrides=False)
```

构造函数中可以使用 `node_name` 设置节点名称，`namespace` 设置命名空间。

### 7.2 编码设置话题名称

话题分为**全局话题**、**相对话题**、**私有话题**三种类型（划分规则见《ROS学习.md》3.11.3），在 rclpy 中通过 `create_publisher` 的第二个参数直接给出话题名即可：

**1. 全局话题**（以 `/` 开头，与命名空间、节点名称无关）：

```python
self.publisher_ = self.create_publisher(String, '/topic/chatter', 10)
```

话题名为 `/topic/chatter`，与命名空间 `xxx`、节点名称 `yyy` 都无关。

**2. 相对话题**（非 `/` 开头，参考命名空间）：

```python
self.publisher_ = self.create_publisher(String, 'topic/chatter', 10)
```

话题名为 `/xxx/topic/chatter`，与命名空间 `xxx` 有关，与节点名称 `yyy` 无关。

**3. 私有话题**（以 `~/` 开头，与命名空间、节点名称都有关）：

```python
self.publisher_ = self.create_publisher(String, '~/topic/chatter', 10)
```

话题名为 `/xxx/yyy/topic/chatter`，以命名空间 `xxx` 与节点名称 `yyy` 作为前缀。

### 7.3 时间相关 API（Rate / Time / Duration）

> 概念与 C++ 写法见《ROS学习.md》3.12。

**1. Rate**

rclpy 中的 Rate 对象通过节点创建，其 `sleep()` 需要在子线程中执行，否则会阻塞程序。示例：周期性输出一段文本。

```python
import rclpy
import threading
from rclpy.timer import Rate

rate = None
node = None

def do_some():
    global rate
    global node
    while rclpy.ok():
        node.get_logger().info("hello ---------")
        # 休眠
        rate.sleep()

def main():
    global rate
    global node
    rclpy.init()
    node = rclpy.create_node("rate_demo")
    # 创建 Rate 对象
    rate = node.create_rate(1.0)

    # 创建子线程
    thread = threading.Thread(target=do_some)
    thread.start()

    rclpy.shutdown()

if __name__ == "__main__":
    main()
```

**2. Time**

示例：创建 Time 对象，并调用其函数。

```python
import rclpy
from rclpy.time import Time
def main():
    rclpy.init()
    node = rclpy.create_node("time_demo")
    # 创建 Time 对象
    right_now = node.get_clock().now()
    t1 = Time(seconds=10,nanoseconds=500000000)

    node.get_logger().info("s = %.2f, ns = %d" % (right_now.seconds_nanoseconds()[0], right_now.seconds_nanoseconds()[1]))
    node.get_logger().info("s = %.2f, ns = %d" % (t1.seconds_nanoseconds()[0], t1.seconds_nanoseconds()[1]))
    node.get_logger().info("ns = %d" % right_now.nanoseconds)
    node.get_logger().info("ns = %d" % t1.nanoseconds)
    rclpy.shutdown()

if __name__ == "__main__":
    main()
```

**3. Duration**

示例：创建 Duration 对象，并调用其函数。

```python
import rclpy
from rclpy.duration import Duration

def main():
    rclpy.init()

    node = rclpy.create_node("duration_demo")
    du1 = Duration(seconds = 2,nanoseconds = 500000000)
    node.get_logger().info("ns = %d" % du1.nanoseconds)

    rclpy.shutdown()

if __name__ == "__main__":

    main()
```

**4. Time 与 Duration 运算**

示例：Time 以及 Duration 的相关运算（运算规则小结见《ROS学习.md》3.12.4）。

```python
import rclpy
from rclpy.time import Time
from rclpy.duration import Duration

def main():
    rclpy.init()
    node = rclpy.create_node("time_opt_node")
    t1 = Time(seconds=10)
    t2 = Time(seconds=4)

    du1 = Duration(seconds=3)
    du2 = Duration(seconds=5)

    # 比较
    node.get_logger().info("t1 >= t2 ? %d" % (t1 >= t2))
    node.get_logger().info("t1 < t2 ? %d" % (t1 < t2))
    # 数学运算
    t3 = t1 + du1
    t4 = t1 - t2
    t5 = t1 - du1

    node.get_logger().info("t3 = %d" % t3.nanoseconds)
    node.get_logger().info("t4 = %d" % t4.nanoseconds)
    node.get_logger().info("t5 = %d" % t5.nanoseconds)

    # 比较
    node.get_logger().info("-" * 80)
    node.get_logger().info("du1 >= du2 ? %d" % (du1 >= du2))
    node.get_logger().info("du1 < du2 ? %d" % (du1 < du2))

    rclpy.shutdown()

if __name__ == "__main__":
    main()
```

## 附录 C：示例工程

`ros2_learning_python/` 是本笔记配套的可运行代码，四、HelloWorld 的案例由它而来。

### C.1 目录结构

```text
ros2_learning_python/
└── src/
    └── pkg_helloworld_py/                  # ament_python 包
        ├── pkg_helloworld_py/              # 与包同名的 Python 包目录
        │   ├── __init__.py                 # 空文件，仅用于标记这是一个 Python 包
        │   └── node_helloworld_py.py       # 节点源文件
        ├── resource/
        │   └── pkg_helloworld_py           # 空文件，供 ROS2 的资源索引识别本包
        ├── test/                           # ament 自带的三个代码风格测试
        │   ├── test_copyright.py
        │   ├── test_flake8.py
        │   └── test_pep257.py
        ├── package.xml
        ├── setup.py                        # 构建脚本，入口在 entry_points 里登记
        └── setup.cfg                       # 把可执行程序装到 lib/<包名> 下
```

### C.2 各文件的实际内容要点

- **`node_helloworld_py.py`**：用继承写法，类名 `Mynode`，`super().__init__("node_helloworld_py")` 里给的字符串就是节点名；`main()` 里 `rclpy.init()` → 创建节点 → 调用自己的方法 → `rclpy.shutdown()`。注意这个示例**没有调用 spin**，方法执行完就退出了。
- **`setup.py` 的 `entry_points`**：

```python
entry_points={
    'console_scripts': [
        'node_helloworld_py = pkg_helloworld_py.node_helloworld_py:main'
    ],
},
```

四项含义见四、第 3 步；这里可执行名、节点名、文件名三者恰好同名，容易混淆，实际项目中可以不同名。

- **`setup.py` 的 `data_files`**：登记 `resource/` 和 `package.xml` 的安装位置，缺了 ROS2 就找不到这个包。
- **`setup.cfg`**：`[develop]` 和 `[install]` 两段都写着 `script_dir=$base/lib/pkg_helloworld_py`，作用是把入口脚本装进 `lib/<包名>/`，正是 `ros2 run` 查找可执行程序的地方。
- **`package.xml`**：`<buildtool_depend>ament_python`、`<depend>rclpy</depend>`，测试依赖 `ament_copyright`、`ament_flake8`、`ament_pep257`、`python3-pytest`（对应 `test/` 下三个测试文件）。
- **`test/` 三个文件**：ament 创建包时自动生成，分别检查版权头、flake8 代码风格、PEP257 文档字符串。`test_copyright.py` 默认带 `@pytest.mark.skip`，因为生成的源文件没有版权头。

### C.3 编译与运行

```bash
cd ros2_learning_python   # 进入工作空间根目录
colcon build              # 编译
. install/setup.bash      # 让当前终端能找到刚编译出来的包
ros2 run pkg_helloworld_py node_helloworld_py   # ros2 run <包名> <可执行名>
```
