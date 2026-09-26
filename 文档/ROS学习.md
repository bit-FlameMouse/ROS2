# ROS2 学习笔记

**目录**

- 一、环境搭建
  - 1.1 ROS2 安装
  - 1.2 开发环境（VSCode 插件）
- 二、ROS2 核心概念
  - 2.1 体系框架
  - 2.2 节点（Node）
  - 2.3 功能包与配置文件
- 三、通信机制
  - 3.1 机制简介
  - 3.2 话题通信
  - 3.3 服务通信
  - 3.4 动作通信
  - 3.5 参数服务
  - 3.6 接口文件（msg / srv / action）
- 四、程序实现
  - 4.1 通用流程（五步）
  - 4.2 C++ 实现
- 五、常用工具
  - 5.1 launch 与 rosbag2
  - 5.2 坐标变换（TF）
  - 5.3 可视化（rviz2 / RQT）
  - 5.4 常用命令行
- 六、应用方向
- 七、技术支持与资源

---

## 一、环境搭建

### 1.1 ROS2 安装

1. 需要有个 Ubuntu 系统，版本要和使用的 ROS2 版本对应得上，虚拟机、双系统都可以。这里以 VMware 安装 Ubuntu 22.04、ROS2 版本。

2. 设置所有的镜像源。

3. 安装 ROS2 并测试。
   1. 设置语言环境：检查本地语言环境是否支持 UTF-8 编码，如果不支持，则进行配置。

   ```bash
   locale  # 看输出，是否有对应的 UTF-8 编码

   # 如果不是 UTF-8，则需要单独配置
   sudo apt update && sudo apt install locales
   sudo locale-gen en_US en_US.UTF-8
   sudo update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8
   export LANG=en_US.UTF-8
   ```

4. 启动 Ubuntu universe 存储库（命令行操作）。

   ```bash
   sudo apt-cache policy | grep universe  # 检查是否已经启动了 Ubuntu universe 存储库

   sudo apt install software-properties-common
   sudo add-apt-repository universe
   ```

5. 设置软件源并将存储库添加到列表中：先将 ROS2 apt 存储库添加到系统中，用 apt 授权 GPG。

   ```bash
   sudo apt update && sudo apt install curl gnupg lsb-release

   sudo curl -sSL https://mirrors.tuna.tsinghua.edu.cn/rosdistro/ros.key -o /usr/share/keyrings/ros-archive-keyring.gpg
   ```

   然后将存储库添加到源列表中：

   ```bash
   echo "deb [arch=amd64 signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://mirrors.tuna.tsinghua.edu.cn/ros2/ubuntu jammy main" | sudo tee /etc/apt/sources.list.d/ros2.list
   ```

6. 安装 ROS2。
   1. 更新 apt 存储库缓存并升级已经安装的软件，这一步不要偷懒（ROS2 软件包建立在经常更新的 Ubuntu 系统上，在安装新软件包之前确保系统是最新的）。
   2. 安装桌面版 ROS2，包含 ROS、RViz、示例与教程。

   ```bash
   sudo apt update
   sudo apt upgrade
   sudo apt install ros-humble-desktop
   ```

   也可以改装基础版 ROS2（包含通信库、消息包、命令行工具，但没有 GUI 工具）：

   ```bash
   sudo apt install ros-humble-ros-base
   ```

   3. 配置环境，使终端启动时自动配置环境。

   ```bash
   echo "source /opt/ros/humble/setup.bash" >> ~/.bashrc
   ```

7. 安装 colcon 构建工具：这是一个命令行工具，用于改进编译、测试和使用多个软件包的工作流程。

   ```bash
   sudo apt install python3-colcon-common-extensions
   ```

   安装完成之后，就可以在 ROS 中编写应用程序了。

**补充一：卸载 ROS2（谨慎操作）**

```bash
sudo apt remove ~nros-humble-* && sudo apt autoremove  # 卸载 ROS2 软件包
```

如果想连存储库一起删掉：

```bash
sudo rm /etc/apt/sources.list.d/ros2.list
sudo apt update
sudo apt autoremove
sudo apt upgrade  # 用于恢复此前被 ROS2 遮蔽的软件包
```

**补充二：raw.githubusercontent.com 连接失败的处理**

本笔记用的是清华镜像源，一般不会遇到这个问题；如果改用官方源，可能在「设置软件源」这一步抛出：

```text
curl: (7) Failed to connect to raw.githubusercontent.com port 443: 拒绝连接
```

原因是 DNS 被污染，处理思路是查出该域名的 IP，再改 `/etc/hosts` 添加映射：

1. 访问 `https://www.ipaddress.com/`，输入域名 `raw.githubusercontent.com` 查询 IP 地址（可能查到多个，记下任意一个即可）。
2. 修改 hosts 文件，在末尾添加「IP 域名」映射后保存：

```bash
sudo gedit /etc/hosts   # 若系统没有 gedit，可用 sudo nano /etc/hosts
```

3. 回到终端重新执行安装命令即可正常进行。

### 1.2 开发环境（VSCode 插件）

1. Chinese：官方汉化插件
2. Msg Language Support：为 ROS 的 `.action`、`.msg`、`.srv` 文件提供基础语言支持
3. vscode-pdf
4. XML：XML 文件格式化
5. YAML：YAML 文件格式化
6. URDF（smilerobotics）：让 VSCode 把 `.urdf` / `.xacro` 文件关联为 XML 格式，并提供代码片段；需要先装 XML 插件（第 4 项）
7. Robot Developer Extensions for URDF（Ranch Hand Robotics）：URDF / xacro 的完整编辑器，支持 3D 预览、语法高亮、代码补全、Schema 校验，以及连杆 / 关节 / 碰撞体可视化

**VSCode 配置（消除头文件报错）**

在 VSCode 中打开 cpp 文件时，`#include "rclcpp/rclcpp.hpp"` 这类包含语句常常会显示红色波浪线，报错原因是 VSCode 没有配置头文件搜索路径（`includePath`）。解决办法：

1. 把鼠标移到报错的 `#include` 语句上，会弹出提示窗口；
2. 点击弹窗中的「快速修复」，再点「编辑 "includePath" 设置」；
3. 在打开页面的「包含路径」文本域中**换行追加**一行：

```text
/opt/ros/humble/include/**
```

该设置保存在工作区的 `.vscode/c_cpp_properties.json` 中，由 C/C++ 扩展提供。

## 二、ROS2 核心概念

### 2.1 体系框架

- 操作系统层：底层操作系统
- 中间层：由数据分发服务（DDS）与封装的中间件组成，还包括服务质量管理，主要由客户端、DDS 抽象层与进程内通讯 API 构成
- 应用层：开发者构建的应用程序，以功能包为核心，大部分开发者集中在这个层

ROS2 的核心模块：

- 通信模块：最重要的模块，数据交互
- 功能包应用：自己写、通过二进制安装、源码安装

其他：

- 分布式：ROS2 是一个分布式架构，不同 ROS2 设备之间可以方便地实现通信。

### 2.2 节点（Node）

节点（Node）：每个节点对应某一单一的功能模块。ROS2 中的单个可执行文件会包含一个或者多个节点。节点就是 node 或者 node 子类的对象。

节点的命名：只能包含字母（大小写均可）、数字、下划线，并且第一个字符必须是字母或下划线。

编码规范：Node 节点必须以继承的方式进行（之前是直接实例化）。这种方式可以在一个进程内组织多个节点，对于提高通信非常有帮助。

资源释放：

- 使用 context 对象（上下文对象），能实现步骤、阶段、线程之间的通信（可以看成交接文档、档案这种东西）。
- 初始化其中一个工作就是创建 context 对象，资源释放其中一个工作就是销毁 context 对象。

### 2.3 功能包与配置文件

每个功能包里都有 `package.xml`，除此之外，`ament_cmake` 类型的包还有 `CMakeLists.txt` 作为构建脚本：

| 构建类型 | 构建脚本 | 适用场景 |
| --- | --- | --- |
| ament_cmake | `CMakeLists.txt` | C++ 节点、自定义接口包、需要编译的包 |

也就是说：**XML 文件（package.xml）所有包都有，CMake 文件只有 ament_cmake 包才有**。

#### 2.3.1 package.xml：功能包的「清单」

它回答两件事：**我是谁**（元信息）和**我依赖谁**（依赖清单）。

关键标签：

| 标签 | 作用 |
| --- | --- |
| `<name>` | 包名，必须和文件夹名一致 |
| `<version>`、`<description>`、`<maintainer>`、`<license>` | 版本、描述、维护者、许可证等元信息 |
| `<depend>` | 通用依赖，等于同时声明「编译时 + 运行时」依赖，最常用 |
| `<build_depend>` / `<exec_depend>` | 分开声明仅编译时 / 仅运行时的依赖 |
| `<buildtool_depend>` | 构建工具依赖，如 `ament_cmake`、`rosidl_default_generators` |
| `<test_depend>` | 只有测试才用到的依赖 |

接口包还要加一行 `<member_of_group>rosidl_interface_packages</member_of_group>`（见 3.6.4）。

一句话：**代码里 `#include` 了哪个包，就要在这里声明对应的依赖**，否则编译或运行时会找不到。

#### 2.3.2 CMakeLists.txt：ament_cmake 包的「施工图」

它告诉 colcon：编译哪些源文件、生成哪些可执行文件、链接哪些库、把结果装到哪里。

```cmake
cmake_minimum_required(VERSION 3.8)
project(pkg_demo_cpp)                            # 包名，与 package.xml、文件夹名三者一致

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)                    # 查找需要用到的依赖包

add_executable(talker src/talker.cpp)            # 源文件 → 可执行文件
ament_target_dependencies(talker rclcpp)         # 给这个可执行文件链接依赖

install(TARGETS talker                           # 安装可执行文件，ros2 run 才找得到
  DESTINATION lib/${PROJECT_NAME})

ament_package()                                  # 结尾必须调用
```

三处最容易出错的地方：

- `add_executable(名字 ...)` 里的「名字」决定 `ros2 run <包名> <名字>` 中的可执行名。
- 少了 `ament_target_dependencies`，编译时会报「找不到头文件 / 符号」。
- 少了 `install(TARGETS ...)`，会出现「编译成功但 `ros2 run` 找不到包」。

#### 2.3.3 新增节点 / 功能时，怎么改写这两个文件

**C++ 包新增一个节点**

1. 在 `src/` 下新建源文件，如 `src/talker.cpp`。
2. 在 `CMakeLists.txt` 里补三条：

```cmake
add_executable(talker src/talker.cpp)
ament_target_dependencies(talker rclcpp)
install(TARGETS talker
  DESTINATION lib/${PROJECT_NAME})
```

3. 如果这个节点用到了新的包（比如 `std_msgs`），`package.xml` 里加 `<depend>std_msgs</depend>`，同时 `CMakeLists.txt` 的 `find_package()` 里补上 `std_msgs`。
4. 重新 `colcon build`。

**新增 launch 文件、配置、模型等资源**

这些文件**不会自动被打包**，必须在构建脚本里声明，否则 `ros2 launch` / `ros2 run` 在安装空间里找不到它们：

- ament_cmake 包在 `CMakeLists.txt` 里加：

```cmake
install(DIRECTORY launch config
  DESTINATION share/${PROJECT_NAME})
```

**新增自定义接口（msg / srv / action）**

在 `CMakeLists.txt` 的 `rosidl_generate_interfaces()` 列表里加一行即可，`package.xml` 里保留 `rosidl_default_generators` 等原有声明，详见 3.6.4。

**几条通用规则**

- 只要「新增」了可执行文件、接口或资源文件，就必须在构建脚本里**登记一次**，否则新东西不会被打包。
- 改完 `package.xml` 或构建脚本后，**必须重新 `colcon build`** 才会生效。
- `package.xml` 的 `<name>`、`CMakeLists.txt` 的 `project()`、文件夹名，三者必须一致。

## 三、通信机制

### 3.1 机制简介

每个模块负责一部分任务，他们干活又依赖其他模块所产生的数据，这就牵扯到通信。ROS2 是分布式的系统架构，每个模块高度解耦。

数据载体：指定数据格式。

功能包：

节点（Node）：每个节点对应某一单一的功能模块。ROS2 中的单个可执行文件会包含一个或者多个节点。节点就是 node 或者 node 子类的对象。

话题：一个纽带，具有相同话题的节点可以关联在一起。虽然不同节点使用的语言不同，但是只要两者使用了相同的话题，那么就可以实现数据的交互。

通信模型：

1. 话题通信：单向通信，发方发布数据，订阅方订阅数据（类比看视频、听音乐）
2. 服务通信：基于请求响应的通信模型，客户端发送请求，服务端响应给客户端（类比网页点击）
3. 动作通信：一种带有连续反馈的通信模型。通信双方，客户端发送请求数据到服务端，服务端响应结果给客户端；服务端接收到请求产生最终响应的过程，会发送连续的反馈信息到客户端（类似于汇报工作进度），适用于一些耗时较长的任务。
4. 参数服务：基于共享通信的模型。通信双方，服务端可以设置数据，客户端可以连接服务端并操作服务端数据（可以理解为代理模式），核心是操纵了一个数据池。

接口：就是通信时的通信载体。通信时使用的数据载体一般需要使用接口文件定义，常用的接口文件有三种：msg 文件、srv 文件、action 文件。

- msg 文件：话题通信
- srv 文件：服务通信
- action 文件：动作通信

参数服务无需定义接口文件，参数通信时数据会被封装为参数对象（结构体），参数客户端和服务端操作的都是参数对象。

### 3.2 话题通信

基于**发布订阅**模式，也即：一个节点发布消息，另一个节点订阅该消息（监听-传输）。

- 例子：激光雷达模块进行采集，导航模块订阅并解析雷达数据。

数据订阅对象称之为**订阅方**，数据发布对象称为**发布方**，发布方和订阅方通过话题相关联，发布方将消息发布在话题上，订阅方则从该话题订阅消息。发布方和订阅方实际上是多对多的关系，数据会出现交叉传输的情况。

适用于不断更新、逻辑处理不复杂的情况。

`std_msgs` 包封装了一些原生的数据类型，比如 string、int8 等，这些原生数据类型可以作为话题通信的载体，不过这些数据一般只包含一个 data 字段。同时可以自定义接口消息。

`rclcpp::spin(node)`：让当前线程进入事件循环，阻塞等待节点上的订阅、定时器、服务等谁就绪，就执行对应的回调。也就是让线程停在这里，等待对应的事件发生；哪个事件发生了，就执行对应的回调。抽象模型上类似 epoll 监听。可以只传入节点指针，不传入执行器时，自己创建一个单线程执行器。显式传入执行器（rclcpp 的一个特定类），执行器决定具体怎么执行：单线程还是多线程，回调怎么回调，是否允许回调并发执行。返回值是 void。

**发布方 / 订阅方的关键接口**（以 rclcpp 为例）

- 发布方：用 `create_publisher()` 创建发布方，指定话题名和消息类型；用 `publish()` 把消息发布到话题上，可以配合定时器周期发送，也可以在事件触发时发送。
- 订阅方：用 `create_subscription()` 创建订阅方并绑定回调函数；话题上每来一条消息就触发一次回调，回调的输入就是收到的消息。
- 和 `rclcpp::spin` 的关系（见本节上文）：发布方是**主动**的，想发就发；订阅方是**被动**的，不进入事件循环，订阅回调就不会被执行。

**案例一：发布内置消息类型**

发布一个 String 类型的信息，属于内置消息类型。内置的类型 String 首字母必须大写，无需自己定义接口，直接用 `std_msgs/msg/String` 即可。

**案例二：发布自定义消息类型**

发布的信息是一个复合类型的，需要自定义。以「学生信息」为例，完整流程如下（接口语法见 3.6.1，注册方法见 3.6.4）。

**1. 创建接口功能包**

接口包用 `ament_cmake` 构建。接口包建议独立成一个包，不要和节点包混在一起。

创建时**不加** `--dependencies`：该参数生成的是 `<depend>` 标签，而接口包需要的是 `<buildtool_depend>rosidl_default_generators</buildtool_depend>`（见 3.6.4），两者一起用会重复声明同一个包，所以按 3.6.4 手动添加即可。

```bash
# 在工作空间的 src 目录下执行
ros2 pkg create pkg_demo_msg --build-type ament_cmake

cd pkg_demo_msg
mkdir msg  # msg 目录不会自动生成，需要手动建
```

**2. 编写接口文件**

在 `msg/` 下新建 `Student.msg`（文件名首字母必须大写）：

```text
string name
int32 age
float32 height
```

**3. 注册接口**

在 `package.xml` 和 `CMakeLists.txt` 中补上注册配置（完整内容见 3.6.4），其中 `CMakeLists.txt` 里要写上：

```cmake
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/Student.msg"
)
```

**4. 编译**

```bash
cd ..  # 回到工作空间根目录
colcon build
. install/setup.bash
```

**5. 验证接口**

```bash
ros2 interface show pkg_demo_msg/msg/Student  # 查看接口定义，确认注册成功
```

**6. 在发布方 / 订阅方使用**

发布方、订阅方的功能包需要先声明依赖，在使用方的 `package.xml` 中加上：

```xml
<depend>pkg_demo_msg</depend>
```

之后在使用方的代码里 `#include "pkg_demo_msg/msg/student.hpp"`。

常用命令：

```bash
ros2 topic list                                         # 列出当前所有话题
ros2 topic list -t                                      # 列出话题并附带接口类型
ros2 topic info /chatter                                # 查看话题的发布方 / 订阅方数量
ros2 topic echo /chatter                                # 打印话题上流动的消息内容
ros2 topic hz /chatter                                  # 查看话题的发布频率
ros2 topic bw /chatter                                  # 查看话题占用的带宽
ros2 topic find std_msgs/msg/String                     # 按接口类型查找话题
ros2 topic pub /chatter std_msgs/msg/String "{data: hello}"  # 命令行手动发布消息（默认按 1 Hz 持续发布，加 --once 只发一条）
ros2 interface show std_msgs/msg/String                 # 查看接口定义
```

### 3.3 服务通信

基于**请求-响应**模式，也即：客户端发送一次请求，服务端处理并返回一次响应（一问一答）。

一个服务（service）上服务端只能有一个，客户端能有很多个。

- 例子：上位机作为客户端向机器人控制器（服务端）请求「查询电量」，服务端返回当前电量值。

请求方称之为**客户端（Client）**，提供方称之为**服务端（Server）**，双方通过**服务名**（service name）关联。和话题的「发布-订阅」持续数据流不同，服务是「调用一次、响应一次」：

- 服务是**被动**的：没有客户端请求时，服务端不会主动做任何事；只有收到请求，回调才会执行并返回响应。
- 服务端必须**已经存在**，客户端才能调用成功；服务端不存在时，调用会失败或一直等待。
- 请求和响应**成对出现**，一次调用对应一次响应，调用结束关系就结束，不存在话题那种持续的订阅关系。

适用于**偶尔调用、需要立刻拿到结果、处理时间不长**的场景，例如查询状态、开关设备、设置参数、简单计算。

- 与话题的区别：话题是单向、持续、多对多（见 3.2）；服务是双向、一问一答、多个客户端对一个服务端。
- 不适用：高频调用或耗时很长的任务——服务端在处理请求期间会被占用，长耗时任务应当用动作通信（3.4）。

服务通信的接口必须用 `.srv` 文件定义（请求和响应两段，详见 3.6.2），不能直接用 `.msg`。

**服务端 / 客户端的关键接口**（以 rclcpp 为例）

- 服务端：用 `create_service()` 注册服务并绑定回调，回调的输入是「请求」，输出是「响应」。
- 客户端：用 `create_client()` 创建客户端，用 `async_send_request()` 发送请求，它返回一个 future（未来值），代表「还没拿到、稍后会有」的响应。
- 和 `rclcpp::spin` 一样，响应不会立刻返回：需要让线程进入事件循环去等，可以用 `spin_until_future_complete(node, future)` 阻塞等待结果，也可以在回调里异步处理，期间继续做别的事。

**案例：两数相加**

一个简单的案例：服务通信，客户端可以提交**两个整数**到服务端，服务端接受请求并解析两个整数求和，然后**将结果响应回客户端**。

对应的 srv 接口（`srv/AddTwoInts.srv`，注册方式见 3.6.4）：

```text
int64 a
int64 b
---
int64 sum
```

- 服务端：提供 `/add_two_ints` 服务，回调里计算 `sum = a + b` 并返回。
- 客户端：向 `/add_two_ints` 发送请求 `{a: 3, b: 5}`，拿到响应 `{sum: 8}`。

常用命令：

```bash
ros2 service list                                       # 列出当前所有服务
ros2 service list -t                                    # 列出服务并附带接口类型
ros2 service type /add_two_ints                         # 查看指定服务的接口类型
ros2 service find pkg_demo_msg/srv/AddTwoInts           # 按接口类型查找服务
ros2 service call /add_two_ints pkg_demo_msg/srv/AddTwoInts "{a: 3, b: 5}"  # 命令行调用服务
ros2 interface show pkg_demo_msg/srv/AddTwoInts         # 查看接口定义
```

### 3.4 动作通信

基于**请求-响应 + 连续反馈**模式，也即：客户端发送一次目标请求，服务端在执行过程中周期性返回进度反馈，最终返回一次结果（发一次、连续反馈、最后给结论）。

一个动作（action）上服务端只能有一个，客户端能有很多个。

- 例子：机器人导航到某个目标点，节点A发布目标信息，节点B收到请求并控制移动，最终响应目标达成状态信息。

导航是一个过程，一个长时间的耗时操作，如果使用服务通信，导航结束的时候，才会产生响应结果，在导航过程中，节点A不会获得任何反馈，可能出现程序“假死”的现象，过程不可控制，这会导致不好的用户体验和逻辑处理的缺陷（导航终止的需求无法实现）。

请求方称之为**动作客户端（Action Client）**，提供方称之为**动作服务端（Action Server）**，双方通过**动作名**（action name）关联。动作是在服务通信的基础上加了一层反馈：

- **目标（goal）**：客户端发出的一次任务请求，相当于服务里的「请求」。
- **结果（result）**：任务结束时返回的最终结果（如「导航成功」）。
- **反馈（feedback）**：执行过程中周期性发回的中间状态（如「已完成 60%」），这是动作独有的，也是它和服务最大的差别。
- 执行过程**可干预**：客户端可以中途取消（cancel）目标，也能查询当前进度，不像服务那样发出去就只能干等。
- 目标有**状态**：一个目标会经历 accepted / executing / succeeded 等状态，客户端和命令行都能查到它当前处于哪一步。

适用于**耗时较长、需要反馈进度或中途可以取消**的场景，例如导航到目标点、机械臂抓取、长时间扫描建图。

- 与话题的区别：话题是单向持续的数据流，没有「完成」的概念；动作有明确的开始与结束，而且是双向的。
- 与服务的区别：服务是「一问一答」，发出请求后只能等结果；动作是「一问、连续反馈、最后一答」，过程中能知道进度、能取消。
- 再深一层：动作底层就是「服务 + 话题」的组合，goal / result 走服务、feedback 走话题（见 3.6.3）。

动作通信的接口必须用 `.action` 文件定义（goal / result / feedback 三段，详见 3.6.3）。

**服务端 / 客户端的关键接口**（以 rclcpp 为例）

- 服务端：用 `rclcpp_action::create_server()` 创建动作服务端，需要提供几个回调——目标处理（接受还是拒绝）、执行（算完填 result）、取消处理。
- 客户端：用 `rclcpp_action::create_client()` 创建动作客户端，用 `async_send_goal()` 发送目标，再用 `async_get_result()` 取最终结果。
- 反馈：发送目标时可以注册反馈回调，服务端每发一次 feedback 就触发一次，用来更新进度显示。
- 和 `rclcpp::spin` 一样，这几个异步接口返回的都是 future，需要用 `rclcpp::spin_until_future_complete()` 去等结果。

**案例：计算斐波那契数列**

一个简单的案例：动作通信，客户端提交**要计算的项数**，服务端依次计算斐波那契数列；每算出一项就**反馈一次当前结果**，全部算完后**把完整数列作为最终结果返回**。

对应的 action 接口（`action/Fibonacci.action`，注册方式见 3.6.4）：

```text
int32 order
---
int32[] sequence
---
int32[] partial_sequence
```

- 第一段（目标）：客户端要算的项数 `order`。
- 第二段（结果）：任务结束后返回的完整数列 `sequence`。
- 第三段（反馈）：执行过程中每算出一项就发回的当前数列 `partial_sequence`。

- 服务端：提供 `/fibonacci` 动作，收到 `order` 后循环计算，每算一项发一次 feedback，全部算完返回 result。
- 客户端：向 `/fibonacci` 发送目标 `{order: 10}`，过程中连续收到 feedback（`{partial_sequence: [0, 1]}`、`{partial_sequence: [0, 1, 1]}` ……），最后收到 result `{sequence: [0, 1, 1, 2, 3, 5, 8, 13, 21, 34]}`。

常用命令：

```bash
ros2 action list                                        # 列出当前所有动作
ros2 action list -t                                     # 列出动作并附带接口类型
ros2 action info /fibonacci                             # 查看动作的服务端 / 客户端数量
ros2 action send_goal /fibonacci pkg_demo_msg/action/Fibonacci "{order: 10}" --feedback  # 发送目标并显示反馈
ros2 interface show pkg_demo_msg/action/Fibonacci       # 查看接口定义
```

### 3.5 参数服务

基于**共享数据**模型，也即：参数集中放在某一方（服务端）维护，其他方（客户端）连接上来读取或修改，双方操作的是同一份数据（可以理解为代理模式）。

一个参数服务上服务端只能有一个（参数始终由服务端一方持有并维护），**客户端能有很多个**。

- 例子：机器人控制器把「最大速度」「低电量阈值」这类配置作为参数暴露出来，上位机作为客户端读取当前值，或者在运行中直接改掉它，而不用重启节点。

持有参数的一方称之为**服务端（Server）**，连接并操作参数的一方称之为**客户端（Client）**，双方通过**参数名**关联：

- 参数是**键值对**：有名字、有值，值的类型在声明的时候就定下来了（bool / 整数 / 浮点 / 字符串，以及它们的数组）。
- 参数可以**随时改**：客户端改完立即生效，服务端不用重启，适合需要在线调参的场景。
- 参数**变化频率低**：和「实时发布自身状态」的话题不同，参数平时放着不动，有需要的节点来取一下就行，不必像话题那样持续发布，能显著降低通讯带宽压力。
- ROS2 里**每个节点维护自己的参数**，所以「服务端」就是持有参数的那个节点。
- 参数读写的**底层走的是服务**：`get_parameters`、`set_parameters`、`list_parameters` 这些服务由参数所在的节点提供，`ros2 param` 命令只是这些服务调用的封装。

适用于**配置项、可调参数、运行状态量**的读写，例如速度上限、PID 增益、是否开启某个功能。

- 与话题的区别：话题是持续的数据流；参数是静态数据，改了才变、不改就没有流量。
- 与服务的区别：服务是「一次调用一次响应」，用完就结束；参数是「一份共享数据一直摆在那里」，谁都能来读、来改。

参数服务**不需要接口文件**：参数会被封装成参数对象（结构体）传递，客户端和服务端操作的都是参数对象，所以它没有对应的 `.msg` / `.srv` / `.action`（见 3.1）。

**服务端 / 客户端的关键接口**（以 rclcpp 为例）

- 服务端：用 `declare_parameter()` 声明参数（声明时给出名字和默认值），用 `get_parameter()` 读、`set_parameter()` 改；想让参数被改时做点事，可以注册回调 `add_on_set_parameters_callback()`。
- 客户端：用 `get_parameters()` / `set_parameters()` 对目标节点的参数做读写，传进去的是参数名的列表。
- 和 `rclcpp::spin` 一样，参数服务的调用也是异步的，需要让线程进入事件循环去等结果。

**案例：修改机器人的最大速度**

一个简单的案例：参数服务，客户端把目标节点上的参数 `max_speed` 从默认的 `1.0`（m/s）改成 `2.0`，再把值读回来确认修改已经生效。

- 服务端（机器人控制器节点）：用 `declare_parameter("max_speed", 1.0)` 声明参数，并注册回调，参数被修改时打印一条日志。
- 客户端：向该节点写入参数 `max_speed = 2.0`，再读取一次，拿到 `2.0` 就说明写入成功。

常用命令：

```bash
ros2 param list                                         # 列出所有节点的参数
ros2 param list /controller                             # 只看指定节点的参数
ros2 param get /controller max_speed                    # 读取参数值
ros2 param set /controller max_speed 2.0                # 修改参数值（值按 YAML 语法解析）
ros2 param describe /controller max_speed               # 查看参数的类型和含义
ros2 param delete /controller max_speed                 # 删除动态参数（已声明的参数删不掉）
ros2 param dump /controller                             # 把节点的全部参数导出成 YAML
ros2 param load /controller params.yaml                 # 从 YAML 文件导入参数
```

### 3.6 接口文件（msg / srv / action）

三种接口文件与通信模型一一对应：msg 对应话题通信、srv 对应服务通信、action 对应动作通信。参数服务不需要接口文件。

#### 3.6.1 msg 文件（话题通信）

**文件本身**

- 位置：放在功能包的 `msg/` 目录下，文件名就是消息类型名，首字母必须大写（如 `Student.msg`），一个文件定义一种消息类型。
- 接口全名：`包名/消息名`（如 `pkg_demo_msg/Student`），中间不含 `msg`。
- 纯文本、UTF-8 编码。

**每一行的写法**

| 种类 | 格式 | 例子 |
| --- | --- | --- |
| 字段 | `类型 字段名` | `int32 age` |
| 注释 | `#` 开头，可整行，也可行尾 | `int32 age  # 年龄` |
| 常量 | `类型 常量名=值`，常量名**必须全大写**（官方硬性要求，不是习惯） | `int32 MAX_AGE=150` |

- 一行只能写一个字段；字段之间没有逗号、分号之类的分隔符，靠「类型」和「字段名」中间的空格区分，空格至少一个（写成 `int32age` 会被当成非法类型）。
- 字段名建议 snake_case，只能包含字母、数字、下划线；**必须以字母开头**，不能以下划线结尾，也不能出现连续两个下划线。
- 字段名不能重复，也不能和常量名重名。
- 常量是编译期常量，不参与数据传输，访问方式为 `消息类型.常量名`。
- 空行和缩进会被忽略。
- msg **支持**给字段设置默认值（官方文档把它列为 ROS2 相对 ROS1 的新增特性）：在「类型 字段名」后面再追加一个值即可，例如 `int32 X 123`、`string full_name "John Doe"`、`int32[] samples [-200, -100, 0, 100, 200]`。唯一的例外是**字符串数组和嵌套的复杂类型**，它们不支持默认值。

**基础类型**（必须写全名，没有简写）

```text
bool
int8 uint8 int16 uint16 int32 uint32 int64 uint64
byte char         # 与 uint8 并列的两种 1 字节整数类型，不是 uint8 的别名
float32 float64   # 没有 float / double，写成 float、double、int 都会报错
string            # 另有 wstring，极少用
```

**数组 / 序列**（在类型后加方括号）

- 变长：`int32[] scores`
- 定长：`float64[36] data`
- 有界变长（ROS2 支持，`<=` 表示上限）：`int32[<=100] data`
- 元素也可以是复合类型：`geometry_msgs/Point[] points`

**有界字符串**（给字符串加长度上限，同样是 ROS2 支持）

- `string<=10 name`：字符串最长 10 个字符。
- 有界字符串和有界数组可以叠加：`string<=10[<=5] names`，表示「最多 5 个字符串，每个最长 10 个字符」。
- 不加修饰的 `string` 即不限长度。

**复合类型（嵌套）**

- 使用其他消息类型时写成 `包名/消息名`，如 `geometry_msgs/Point position`；同一个包内可以省略包名，直接写 `Point position`。
- msg 只能嵌套 msg，不能直接嵌套 srv。
- ROS2 没有内置 Header：需要时间戳或坐标系时显式写 `std_msgs/Header header`，惯例放在第一个字段；纯时间用 `builtin_interfaces/Time`。

#### 3.6.2 srv 文件（服务通信）

**文件本身**

- 位置：放在功能包的 `srv/` 目录下，文件名就是服务类型名，首字母必须大写（如 `AddTwoInts.srv`）。
- 接口全名：`包名/服务名`（如 `pkg_demo_msg/AddTwoInts`），中间不含 `srv`。

**结构：两段式**

用**一行 `---`** 把文件分成两段，上段是**请求（request）**，下段是**响应（response）**。两段各自都遵循 msg 的字段语法（字段、注释、常量、类型、数组、复合类型完全一样）。

| 段 | 名称 | 含义 |
| --- | --- | --- |
| 第一段 | 请求（request） | 客户端发送给服务端的数据 |
| 第二段 | 响应（response） | 服务端返回给客户端的数据 |

```text
# 请求（request）
int64 a
int64 b
---
# 响应（response）
int64 sum
```

- `---` 必须有且只有一行。
- 请求段或响应段都可以为空（表示没有请求参数、或没有返回值），但 `---` 不能省。
- 生成代码里两段分别对应 `AddTwoInts.Request` 和 `AddTwoInts.Response`。
- 一个服务只能有一个服务端，客户端可以有多个。

#### 3.6.3 action 文件（动作通信）

**文件本身**

- 位置：放在功能包的 `action/` 目录下，文件名就是动作类型名，首字母必须大写（如 `Fibonacci.action`）。
- 接口全名：`包名/动作名`（如 `pkg_demo_msg/Fibonacci`），中间不含 `action`。

**结构：三段式**

用**两行 `---`** 把文件分成三段，顺序固定，依次是：

| 段 | 名称 | 含义 |
| --- | --- | --- |
| 第一段 | 目标（goal） | 客户端发送的请求数据 |
| 第二段 | 结果（result） | 任务完成后返回的最终结果 |
| 第三段 | 反馈（feedback） | 任务执行过程中周期性发送的中间状态 |

三段都遵循 msg 的字段语法。

```text
# 目标（goal）
int32 order
---
# 结果（result）
int32[] sequence
---
# 反馈（feedback）
int32[] partial_sequence
```

- 两行 `---` 必须都存在，即使某一段为空也要保留分隔线，否则解析报错。
- 生成代码里三段分别对应 `Fibonacci.Goal`、`Fibonacci.Result`、`Fibonacci.Feedback`。
- action 底层是「服务 + 话题」的组合：goal / result 走服务，feedback 走话题，所以它既能请求响应、又能连续反馈；适用于耗时较长、需要反馈进度的任务。

#### 3.6.4 功能包注册方法

msg、srv、action 三类接口的注册方式完全一样，都靠功能包里的 `rosidl_generate_interfaces()` 统一注册。只写接口文件还不够，不注册的话编译时不会生成对应代码。

`package.xml` 中添加：

```xml
<buildtool_depend>rosidl_default_generators</buildtool_depend>
<exec_depend>rosidl_default_runtime</exec_depend>
<member_of_group>rosidl_interface_packages</member_of_group>
```

`CMakeLists.txt` 中添加（三类接口文件写在同一个 `rosidl_generate_interfaces()` 里）：

```cmake
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/Student.msg"
  "srv/AddTwoInts.srv"
  "action/Fibonacci.action"
)
```

编译并验证：

```bash
colcon build
. install/setup.bash
ros2 interface show pkg_demo_msg/msg/Student       # 验证 msg
ros2 interface show pkg_demo_msg/srv/AddTwoInts    # 验证 srv
ros2 interface show pkg_demo_msg/action/Fibonacci  # 验证 action
```

如果别的功能包要使用这些接口，需要在使用方的 `package.xml` 中声明依赖：`<depend>接口包名</depend>`。

## 四、程序实现

### 4.1 通用流程（五步）

始终就这五步：

- 创建功能包
- 编辑源文件
- 编辑配置文件
- 编译
- 运行

### 4.2 C++ 实现

#### HelloWorld

**1. 创建功能包**

`rclcpp` 是 ROS2 官方的 C++ 客户端库（ROS Client Library for C++），用于编写 ROS2 的 C++ 节点和应用程序。

```bash
mkdir -p ws00_helloworld/src  # 创建一个工作空间

cd ws00_helloworld/src  # 进入源码目录

# 调用 ROS2 的创建功能包命令
# --build-type 指定构建系统（ament_cmake），--dependencies 声明依赖（rclcpp），--node-name 设置节点名称
ros2 pkg create pkg01_helloworld_cpp --build-type ament_cmake --dependencies rclcpp --node-name helloworld
```

**2. 编辑源文件**

编辑生成的包下面的 src 目录下的 helloworld.cpp 文件，这个是对应的执行文件。

```cpp
#include "rclcpp/rclcpp.hpp"

int main(int argc, char **argv){
    rclcpp::init(argc, argv);  // 初始化 ROS2
    auto node = rclcpp::Node::make_shared("helloworld_node");  // 创建节点
    RCLCPP_INFO(node->get_logger(), "hello world!");  // 输出文本
    rclcpp::shutdown();  // 释放资源

    return 0;
}
```

**3. 编辑配置文件**

不用完全自己写，以后只需要知道关键的地方是什么意思、哪些地方需要修改即可。CMAKE 文件、XML 文件（两者的作用与改法见 2.3）。

**4. 编译**

使用 colcon 进行 build。

```bash
cd ..  # 进入工作空间目录
colcon build  # 使用 colcon 进行 build
```

**5. 运行**

```bash
. install/setup.bash
ros2 run pkg01_helloworld_cpp helloworld
```

## 五、常用工具

既有 ROS2 的命令行工具，也有图形化工具 RQT。

### 5.1 launch 与 rosbag2

launch 文件：通过 launch 文件，可以批量启动 ROS2 节点，这是在构建大型项目时启动多节点的常用方式。

### 5.2 坐标变换（TF）

TF 坐标变换：能实现不同机器人、或者机器人的不同部件之间相对关系的转换。

### 5.3 可视化（rviz2 / RQT）

可视化：内置三维可视化 rviz2，以图形化的方式显示机器人模型或显示机器人系统中的一些抽象数据。

### 5.4 常用命令行

ros2 的查找指令：

```bash
ros2 pkg executables [包名]  # 输出所有功能包或指定功能包下的可执行程序
ros2 pkg list  # 列出所有功能包，包括自己写的和系统自带的
ros2 pkg prefix [包名]  # 列出功能包路径
ros2 pkg xml [包名]  # 输出功能包的 package.xml 内容
```
