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
  - 3.7 分布式通信（DDS 域）
  - 3.8 工作空间覆盖
  - 3.9 元功能包
  - 3.10 节点重名
  - 3.11 话题重名
  - 3.12 时间相关 API
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
- 附录 A：示例工程
- 附录 B：ROS2 接口速查

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

#### 2.3.1 XML 与 YAML：两种配置文件格式

**它们是什么**

XML 和 YAML 本质是**同一类东西**：都是「标记语言」，用来把**结构化数据**（一层套一层的配置信息）写成纯文本，让程序能读、人也能看懂。它们只负责**描述数据**，不负责逻辑。二者的差别只在「怎么表达层级」：

| 维度 | XML | YAML |
| --- | --- | --- |
| 层级表达 | 用**尖括号标签**嵌套 | 用**缩进**层级 |
| 写法 | `<标签 属性="值">内容</标签>`，必须成对闭合 | `键: 值`，`-` 表示列表项，**无闭合概念** |
| 冗长度 | 啰嗦（闭合标签、引号都要写全） | 简洁（省掉大量括号引号） |
| 容错性 | 较宽容，空格随意 | **对缩进极敏感**，缩错就报错 |
| 典型用途 | 配置、数据交换（老牌标准） | 配置（现代工具偏爱） |

一句话记：**XML 靠标签，YAML 靠缩进；XML 啰嗦但严谨，YAML 简洁但娇气。**

**同一个意思的两种写法**

下面这组配置描述「有一个叫 turtle1 的节点」，两种写法表达的内容完全一样：

```xml
<launch>
    <node pkg="topic_cpp" exec="demo01_talker_str" name="turtle1" />
</launch>
```

```yaml
launch:
- node:
    pkg: topic_cpp
    exec: demo01_talker_str
    name: turtle1
```

可以看到：XML 用 `<node>` 标签加上 `属性="值"`，YAML 用缩进的 `键: 值`。`launch` 下面那个 `-` 表示「这是一个列表项」，因为 launch 文件里可以有多个节点。

**在 ROS2 中的作用**

这两种格式在 ROS2 里都是**配置文件**（描述数据、不含程序逻辑），主要出现在三处：

1. **`package.xml`** —— **XML 的主场**。功能包的「清单」（见 2.3.2），文件名和格式都是固定的，构建工具 `colcon` 靠它识别包的元信息与依赖。
2. **launch 文件** —— **XML / YAML / Python 三选一**（见 5.1）。三者功能等价，XML、YAML 胜在「不用写代码」，Python 胜在灵活（能写条件、循环）。
3. **其它配置** —— 如 rviz2 的视图配置、地图参数等，大多也是 YAML 格式。

#### 2.3.2 package.xml：功能包的「清单」

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

#### 2.3.3 CMakeLists.txt：ament_cmake 包的「施工图」

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

#### 2.3.4 新增节点 / 功能时，怎么改写这两个文件

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

**四种通信方式是怎么「联系」在一起的**

不管是话题、服务、动作还是参数，通信双方都是靠**同一套机制**配对的：

- **按名字**：话题名、服务名、动作名、参数名。名字差一个字就永远连不上。
- **按类型**：两端声明的接口类型必须一致（消息类型 / 服务类型 / 动作类型）。
- **靠 DDS 自动发现**：不需要指定对方的 IP 或进程，底层中间件（DDS）在后台自动扫描并匹配，跨机器也行。这是 ROS2 相对 ROS1（靠 master 集中注册）最大的变化。

也就是说，「客户端怎么找到服务端」这个问题，四种通信方式的答案是一样的：**按名字和类型，由 DDS 自动配对。**

**但「底层由什么拼成」各不相同**——ROS2 真正原子的传输模型只有**话题**和**服务**两种，另外两种都是在它们之上封装出来的：

| 通信方式 | 底层由什么组成 | 展开后的命名接口 |
| --- | --- | --- |
| 话题 | 就是话题本身 | `/话题名`（一条话题，多对多） |
| 服务 | 就是服务本身 | `/服务名`（一条服务，一请求一应答） |
| 动作 | 服务 + 话题 | `/动作名/_action/` 下的一组接口：3 条服务 + 2 条话题（详见 3.4） |
| 参数 | 服务 + 话题 | `/节点名/` 下的一组服务（读写参数）+ `parameter_events` 话题（通知变更，详见 3.5.2） |

规律：**话题、服务是「原子」的**，本身就是最底层的传输模型，直接对外暴露一条命名接口；**动作、参数是「组合」的**，都是从「服务 + 话题」拼出来的，只是在上面又包了一层语义。

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

示例工程（`topic_cpp`）里实际用到的接口：

| 接口 | 参数（含义 / 要传什么） | 声明位置 | 作用 |
| --- | --- | --- | --- |
| `rclcpp::init()` | `argc`、`argv`（类型 `int`、`char const *[]`）：`main()` 的命令行参数，直接把 `main` 的入参原样传进来即可，如 `rclcpp::init(argc, argv)` | `main()` | 初始化 ROS2，必须最先调用 |
| `this->create_publisher<消息类型>(话题名, 队列长度)` | `话题名`（字符串）：话题名称字符串，如 `"topic"`，发布方和订阅方必须一致；`队列长度`（整数）：QoS 队列深度，如 `10`，消息积压时最多缓存这么多条 | 构造函数 | 创建发布方，模板参数是消息类型 |
| `publisher_->publish(消息)` | `消息`（消息类型对象，只读引用）：要发布出去的消息对象，如 `msg` | 定时器回调 | 把消息发到话题上 |
| `this->create_subscription<消息类型>(话题名, 队列长度, 回调)` | `话题名`（字符串）：要订阅的话题名，如 `"topic"`；`队列长度`（整数）：QoS 队列深度，如 `10`；`回调`（函数对象）：收到消息时触发的成员函数，需和当前节点对象一起传入 | 构造函数 | 创建订阅方并绑定回调 |
| `this->create_wall_timer(周期, 回调)` | `周期`（时长）：触发间隔，如 `500ms` 或 `1s`；`回调`（函数对象）：到点触发的成员函数，需绑定当前节点对象（无入参） | 构造函数 | 创建「墙钟定时器」，按固定周期触发回调（发布方靠它周期发消息） |
| `rclcpp::spin(节点指针)` | `节点指针`（节点对象的共享指针）：要交给事件循环托管的节点对象，如 `node` | `main()` | 进入事件循环，处理定时器和订阅回调 |
| `rclcpp::shutdown()` | 无参数 | `main()` | 关闭通信、释放资源 |
| `RCLCPP_INFO(日志器, "格式", 参数...)` | `日志器`（日志器对象）：当前节点的日志器，传 `this->get_logger()`；`"格式"`（字符串）：`printf` 风格格式串，如 `"发布的消息：%s"`；`参数...`（可变）：按格式串依次填入的值，如 `msg.data.c_str()` | 任意位置 | 打印日志（`RCLCPP_WARN` / `RCLCPP_ERROR` 同理） |
| `this->get_logger()` | 无参数 | 任意位置 | 取当前节点的日志器，传给日志宏的第一个参数 |
| `rclcpp::TimerBase::SharedPtr` | 无参数（这是类型，不是函数） | 成员声明 | 定时器对象句柄 |
| `rclcpp::Publisher<消息类型>::SharedPtr` | 无参数（这是类型，不是函数） | 成员声明 | 发布方对象句柄 |
| `rclcpp::Subscription<消息类型>::SharedPtr` | 无参数（这是类型，不是函数） | 成员声明 | 订阅方对象句柄 |

几点说明：

- `create_publisher()` / `create_subscription()` 的队列长度就是 QoS 的队列深度：消息来不及处理时最多缓存这么多条，超出的旧消息会被丢弃。
- `create_wall_timer()` 的周期可以写成 `500ms` 这种时长字面量。
- 传回调时要注意：回调是定义在节点类里的成员函数，必须和「当前这个节点对象」一起传进去，否则 ROS2 不知道该调用哪个对象上的方法。写法上就是把成员函数和当前对象绑定在一起，需要接收入参的回调再按顺序预留出参数位置（订阅回调留 1 个，用来接收消息；服务回调留 2 个，用来接收请求和响应；定时器回调不需要留）。
- 订阅回调的参数用 `const` 修饰，表示回调里不该修改收到的消息。
- `RCLCPP_INFO()` 的格式串沿用 `printf` 风格：`%s` 字符串、`%d` 整数、`%ld` 长整数、`%.2f` 保留两位小数。

**案例一：发布内置消息类型（`topic_cpp` 的 demo01 / demo02）**

发布一个 String 类型的信息，属于内置消息类型。内置的类型 String 首字母必须大写，无需自己定义接口，直接用 `std_msgs/msg/String` 即可。

示例工程的对应文件：发布方 `topic_cpp/src/demo01_talker_str.cpp`、订阅方 `topic_cpp/src/demo02_listener_str.cpp`。

```bash
ros2 run topic_cpp demo01_talker_str    # 发布方：每 0.5 秒发一条
ros2 run topic_cpp demo02_listener_str  # 订阅方：另一个终端运行
```

- 节点名分别是 `minimal_publisher` 和 `minimal_subscriber`，话题名统一用 `topic`（双方必须一致才能通信），队列长度 10。
- 发布方用一个 500ms 的定时器周期触发，每次把字符串 `"Hello, world! "` 拼上自增计数器后 `publish()` 出去；订阅方只负责在回调里打印收到的 `data`。
- 两者的消息类型都是 `std_msgs::msg::String`，代码里对应 `#include "std_msgs/msg/string.hpp"`。

**案例二：发布自定义消息类型（`topic_cpp` 的 demo03 / demo04）**

发布的信息是一个复合类型的，需要自定义。以「学生信息」为例，完整流程如下（接口语法见 3.6.1，注册方法见 3.6.4）。

**1. 创建接口功能包**

接口包用 `ament_cmake` 构建，独立成一个包，不要和节点包混在一起。示例工程里的接口包叫 `base_interfaces_demo`。

创建时**不加** `--dependencies`：该参数生成的是 `<depend>` 标签，而接口包需要的是 `rosidl_default_generators`（见 3.6.4），两者一起用会重复声明同一个包，所以手动添加即可。

```bash
# 在工作空间的 src 目录下执行
ros2 pkg create base_interfaces_demo --build-type ament_cmake

cd base_interfaces_demo
mkdir msg  # msg 目录不会自动生成，需要手动建
```

**2. 编写接口文件**

在 `msg/` 下新建 `Student.msg`（文件名首字母必须大写）。示例工程里的内容（与 `Student.msg` 原文一致）：

```text
string name # 学生姓名，使用string，而不是String，否则会被当成ROS2内置的消息类型
int32 age # 学生年龄
float64 height # 学生身高
```

- 三行分别对应姓名字段（`string`）、年龄字段（`int32`）、身高字段（`float64`）。
- 注释里的提醒很关键：字段类型要写小写的 `string`，写成大写的 `String` 会被当成 ROS2 内置的消息类型。

**3. 注册接口**

在 `package.xml` 和 `CMakeLists.txt` 中补上注册配置（完整内容见 3.6.4），其中 `CMakeLists.txt` 里要写上（示例工程里三个接口文件写在同一个 `rosidl_generate_interfaces()` 中）：

```cmake
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/Student.msg"
  "srv/AddInts.srv"
  "action/Progress.action"
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
ros2 interface show base_interfaces_demo/msg/Student  # 查看接口定义，确认注册成功
```

**6. 在发布方 / 订阅方使用**

发布方、订阅方的功能包需要先声明依赖。示例工程里 `topic_cpp` 的 `package.xml` 加的是：

```xml
<depend>base_interfaces_demo</depend>
```

并且在 `CMakeLists.txt` 里 `find_package(base_interfaces_demo REQUIRED)`，给用到的可执行文件补上 `ament_target_dependencies(... base_interfaces_demo)`。

之后在使用方的代码里包含头文件、并用 `using` 简化类型名：

```cpp
#include "base_interfaces_demo/msg/student.hpp"  // 编译接口包时自动生成

using base_interfaces_demo::msg::Student;        // 简化类型名，后面直接写 Student
```

对应文件：发布方 `topic_cpp/src/demo03_talker_student.cpp`、订阅方 `topic_cpp/src/demo04_listener_student.cpp`，话题名 `topic_stu`。

```bash
ros2 run topic_cpp demo03_talker_student    # 发布方：每 0.5 秒发一条 Student
ros2 run topic_cpp demo04_listener_student  # 订阅方：另一个终端运行
```

- 节点名分别是 `student_publisher` 和 `student_subscriber`。
- 发布方每次新建一个 `Student` 对象，把 `name` 固定为「张三」、`age` 递增、`height` 固定 1.65，打印后 `publish()`。
- 订阅方在回调里依次打印 `name`、`age`、`height` 三个字段。

常用命令：

```bash
ros2 topic list                                         # 列出当前所有话题
ros2 topic list -t                                      # 列出话题并附带接口类型
ros2 topic info /topic                                  # 查看话题的发布方 / 订阅方数量
ros2 topic echo /topic                                  # 打印话题上流动的消息内容
ros2 topic hz /topic                                    # 查看话题的发布频率
ros2 topic bw /topic                                    # 查看话题占用的带宽
ros2 topic find std_msgs/msg/String                     # 按接口类型查找话题
ros2 topic pub /topic std_msgs/msg/String "{data: hello}"   # 命令行手动发布消息（默认按 1 Hz 持续发布，加 --once 只发一条）
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

示例工程（`service_cpp`）里实际用到的接口：

| 接口 | 参数（含义 / 要传什么） | 声明位置 | 作用 |
| --- | --- | --- | --- |
| `this->create_service<接口类型>(服务名, 回调)` | `服务名`（字符串）：服务名称字符串，如 `"add_ints"`，服务端和客户端必须一致；`回调`（函数对象）：收到请求时触发的成员函数，需绑定当前节点对象，签名固定为（请求、响应）两个参数 | 服务端构造函数 | 创建服务端并绑定回调，回调的第 1 个参数是请求、第 2 个是响应 |
| `this->create_client<接口类型>(服务名)` | `服务名`（字符串）：要连接的服务名，如 `"add_ints"`，必须与服务端一致 | 客户端构造函数 | 创建客户端，参数是服务名 |
| `client->wait_for_service(超时)` | `超时`（时长）：最长等待时长，如 `1s`；返回 `true` 表示服务端已上线 | 客户端 | 等待服务端上线；返回 `true` 表示已连上，超时返回 `false`，常配合 `while` 循环重试 |
| `client->async_send_request(请求)` | `请求`（请求对象的共享指针）：要发给服务端的请求对象，先把 `num1`、`num2` 填好 | 客户端 | **异步**发送请求，立刻返回，不等结果 |
| `.future.share()` | 无参数（在 `async_send_request` 的返回值上调用） | 客户端 | 从返回值里取出可共享的 future，供后面等待结果用 |
| `rclcpp::spin_until_future_complete(节点, future)` | `节点`（节点对象的共享指针）：等结果期间负责处理回调的节点；`future`（上一步的 future）：要等待的异步结果 | 客户端 `main()` | 一边处理回调一边等 future 完成，返回 `FutureReturnCode::SUCCESS` 表示成功 |
| `rclcpp::FutureReturnCode::SUCCESS` | 无参数（枚举值，用于比较） | 客户端 | 判断 future 是否成功完成 |
| `rclcpp::ok()` | 无参数 | 客户端 | 判断程序是否仍应继续运行（Ctrl+C 后变 `false`） |
| `<接口类型>::Request` / `<接口类型>::Response` | 无参数（这是类型，不是函数） | 服务端、客户端 | 编译接口包时生成的请求类 / 响应类 |
| `rclcpp::Service<接口类型>::SharedPtr` | 无参数（这是类型，不是函数） | 成员声明 | 服务端对象句柄 |
| `rclcpp::Client<接口类型>::SharedPtr` | 无参数（这是类型，不是函数） | 成员声明 | 客户端对象句柄 |

几点说明：

- 服务端回调的签名是「请求对象 + 响应对象」两个入参：**请求只读**（从中取 `num1`、`num2`），**响应可写**（往 `sum` 赋值后就自动回给客户端，不需要手动「发送响应」）。
- 客户端「等待服务端」是个绕不开的步骤：服务端没启动时请求会失败，所以代码里用 `while (!client->wait_for_service(1s))` 循环等，循环体里检查 `rclcpp::ok()`，这样用户按 Ctrl+C 能立刻退出而不是死等。
- 要把响应取出来用，得先从 future 里「解引用」拿到响应对象，再访问它的字段。
- 一个服务只能有一个服务端，客户端可以有多个；服务端在处理请求期间被占用，所以耗时很长的任务应当改用动作通信（3.4）。

**案例：两数相加（`service_cpp`）**

一个简单的案例：服务通信，客户端可以提交**两个整数**到服务端，服务端接受请求并解析两个整数求和，然后**将结果响应回客户端**。

对应的 srv 接口是示例工程里的 `base_interfaces_demo/srv/AddInts.srv`（注册方式见 3.6.4），原文：

```text
# 请求部分
int32 num1
int32 num2
---
# 响应部分
int32 sum
```

- 第一段（请求）：客户端要相加的两个整数 `num1`、`num2`。
- 第二段（响应）：服务端算出的和 `sum`。
- 生成代码里两段分别对应 `AddInts::Request` 和 `AddInts::Response`。

对应文件：服务端 `service_cpp/src/demo01_server.cpp`、客户端 `service_cpp/src/demo02_client.cpp`，服务名 `add_ints`。

```bash
ros2 run service_cpp demo01_server         # 先启动服务端
ros2 run service_cpp demo02_client 3 5     # 再启动客户端，把 3 和 5 作为命令行参数传入
```

- 服务端：节点名 `minimal_service`，用 `create_service<AddInts>("add_ints", ...)` 注册回调；回调拿到的 `req` 是只读的请求、`res` 是可写的响应，里面做 `res->sum = req->num1 + req->num2`，并打印 `请求数据:(3,5),响应结果:8`。
- 客户端：节点名 `minimal_client`，用 `create_client<AddInts>("add_ints")` 创建客户端后，先 `wait_for_service(1s)` 循环等待服务端上线（期间检查 `rclcpp::ok()`，万一被 Ctrl+C 能立刻退出）；连上后 `async_send_request()` 异步发出请求，最后用 `rclcpp::spin_until_future_complete()` 等待响应，成功则打印 `响应结果:8!`。
- 客户端要接收两个命令行参数，所以启动前会先检查参数个数，不足两个就提示「请提交两个整型数据！」并退出。

常用命令：

```bash
ros2 service list                                       # 列出当前所有服务
ros2 service list -t                                    # 列出服务并附带接口类型
ros2 service type /add_ints                             # 查看指定服务的接口类型
ros2 service find base_interfaces_demo/srv/AddInts      # 按接口类型查找服务
ros2 service call /add_ints base_interfaces_demo/srv/AddInts "{num1: 3, num2: 5}"   # 命令行调用服务
ros2 interface show base_interfaces_demo/srv/AddInts    # 查看接口定义
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

#### 3.4.1 动作客户端和服务端是怎么联系上的

动作不是一种新的底层传输，它是由**服务 + 话题**拼出来的封装。当你用 `rclcpp_action::create_server(节点, "动作名", ...)` 创建动作服务端时，ROS2 会**自动**在 `/动作名` 这个命名空间下生成一组固定的底层接口。以示例工程的动作名 `get_sum` 为例：

| 底层接口 | 类型 | 方向 | 作用 |
| --- | --- | --- | --- |
| `/get_sum/_action/send_goal` | 服务 | 客户端 → 服务端 | 客户端提交目标（goal） |
| `/get_sum/_action/cancel_goal` | 服务 | 客户端 → 服务端 | 客户端请求取消目标 |
| `/get_sum/_action/get_result` | 服务 | 客户端 → 服务端 | 客户端索取最终结果（result） |
| `/get_sum/_action/feedback` | 话题 | 服务端 → 客户端 | 服务端周期性推送进度（feedback） |
| `/get_sum/_action/status` | 话题 | 服务端 → 客户端 | 服务端广播每个目标的状态变化 |

前三条是服务（有请求有应答），后两条是话题（服务端单向广播）——这正是「动作 = 服务 + 话题」的具体落点。

**双方是怎么配对的**：客户端用 `rclcpp_action::create_client(节点, "动作名")` 同样展开成上面这 5 条接口，名字和类型必须和服务端**完全一致**；然后由 DDS 自动发现、按「名字 + 类型」匹配上。客户端**不需要知道**服务端在哪台机器、哪个进程，也不需要任何手写的握手步骤——这 5 条接口在 `create_server` / `create_client` 时就已经建好了。

**一次完整交互的时序**（以客户端发一个 `num = 10` 的目标为例）：

1. 客户端调 `/get_sum/_action/send_goal`，把 goal 发给服务端。
2. 服务端立刻应答「接受 / 拒绝」，并把这次 goal 打包成一个目标句柄（goal handle）。客户端在 `SendGoalOptions` 的 `goal_response_callback` 里收到这个应答。
3. 服务端开始执行，每算一步就往 `/get_sum/_action/feedback` 发一条 `progress`。客户端通过 `feedback_callback` 收到（多次、可丢）。
4. 若中途取消，客户端调 `/get_sum/_action/cancel_goal`，触发服务端的 `handle_cancel()`。
5. 客户端调 `/get_sum/_action/get_result` 要最终结果，这一步会**阻塞等待**直到结果就绪。
6. 服务端把 `result` 返回，客户端在 `result_callback`（或 future）里拿到 `result.code`，判断是 `SUCCEEDED` / `ABORTED` / `CANCELED`。

`/get_sum/_action/status` 话题则在这几步之间持续广播每个目标的状态变化，供需要同时跟踪多个目标的场景使用。

**服务端 / 客户端的关键接口**（以 rclcpp 为例）

- 服务端：用 `rclcpp_action::create_server()` 创建动作服务端，需要提供几个回调——目标处理（接受还是拒绝）、执行（算完填 result）、取消处理。
- 客户端：用 `rclcpp_action::create_client()` 创建动作客户端，用 `async_send_goal()` 发送目标，再用 `async_get_result()` 取最终结果。
- 反馈：发送目标时可以注册反馈回调，服务端每发一次 feedback 就触发一次，用来更新进度显示。
- 和 `rclcpp::spin` 一样，这几个异步接口返回的都是 future，需要用 `rclcpp::spin_until_future_complete()` 去等结果。

示例工程（`action_cpp`）里实际用到的接口，分服务端和客户端两张表。

**服务端**：

| 接口                                                               | 参数（含义 / 要传什么）                                                                                                                                  | 作用                                 |
| ---------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------- |
| `rclcpp_action::create_server<接口类型>(节点, 动作名, 处理目标, 处理取消, 接受后执行)` | `节点`（节点对象的共享指针）：承载动作服务端的节点；`动作名`（字符串）：动作名称，如 `"get_sum"`，与客户端一致；`处理目标`：`handle_goal` 回调；`处理取消`：`handle_cancel` 回调；`接受后执行`：`handle_accepted` 回调 | 创建动作服务端，参数是节点、动作名、三个回调             |
| `handle_goal(目标编号, 目标对象)`                                        | `目标编号`（目标编号）：本次目标的唯一编号；`目标对象`（目标对象的共享指针）：客户端发来的目标，如取其 `num` 字段                                                                                 | 收到目标时调用，返回接受或拒绝                    |
| `rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE` / `::REJECT`   | 无参数（枚举值，用作 `handle_goal` 的返回值）                                                                                                                 | 目标处理回调的两个返回值：接受并执行 / 拒绝            |
| `handle_cancel(目标句柄)`                                            | `目标句柄`（目标句柄）：要取消的那个目标                                                                                                                          | 收到取消请求时调用                          |
| `rclcpp_action::CancelResponse::ACCEPT`                          | 无参数（枚举值，用作 `handle_cancel` 的返回值）                                                                                                               | 取消处理回调的返回值：同意取消                    |
| `handle_accepted(目标句柄)`                                          | `目标句柄`（目标句柄）：已被接受的那个目标，交给执行函数用                                                                                                                 | 目标被接受后调用，这里负责真正启动执行                |
| `rclcpp_action::ServerGoalHandle<接口类型>`                          | 无参数（这是类型，不是函数）                                                                                                                                 | 管理「某一个目标」的句柄，用它反馈进度、结束任务           |
| `目标句柄->get_goal()`                                               | 无参数；返回目标对象的共享指针                                                                                                                                | 取出客户端发来的目标（取其字段作为输入）               |
| `目标句柄->publish_feedback(反馈对象)`                                   | `反馈对象`（反馈对象的共享指针）：要发回的进度对象，如把 `progress` 字段填好后传入                                                                                               | 发布一次连续反馈                           |
| `目标句柄->is_canceling()`                                           | 无参数；返回 `bool`                                                                                                                                  | 查询客户端是否请求了取消，循环里要检查                |
| `目标句柄->succeed(结果对象)`                                            | `结果对象`（结果对象的共享指针）：最终结果对象，如把 `sum` 填好后传入                                                                                                        | 任务正常完成，返回最终结果                      |
| `目标句柄->canceled(结果对象)` / `abort(结果对象)`                           | `结果对象`（结果对象的共享指针）：当前已算出的结果对象                                                                                                                   | 任务被取消 / 被中止（Ctrl+C 中断时用），返回当前结果    |
| `rclcpp::Rate 频率对象(Hz)` + `.sleep()`                             | `Hz`（浮点）：循环频率，如 `10.0` 表示 10 Hz；`.sleep()` 无参数                                                                                                 | 控制循环频率（如 10 Hz，即每轮睡 0.1 秒），让反馈呈周期性 |
| `rclcpp_action::Server<接口类型>::SharedPtr`                         | 无参数（这是类型，不是函数）                                                                                                                                 | 动作服务端对象句柄                          |

**客户端**：

| 接口 | 参数（含义 / 要传什么） | 作用 |
| --- | --- | --- |
| `rclcpp_action::create_client<接口类型>(节点, 动作名)` | `节点`（节点对象的共享指针）：承载动作客户端的节点；`动作名`（字符串）：要连接的动作名，如 `"get_sum"`，与服务端一致 | 创建动作客户端 |
| `client->wait_for_action_server(超时)` | `超时`（时长）：最长等待时长，如 `10s`；返回 `bool` | 等待动作服务端上线，超时返回 `false` |
| `rclcpp_action::Client<接口类型>::SendGoalOptions` | 无参数（这是结构体类型，先定义一个对象再往三个 `callback` 成员里填回调） | 承载三个回调的结构体，发目标前先填好 |
| `send_goal_options.goal_response_callback` | 赋值为一个回调函数，入参是「目标句柄」的 future（为空表示被拒绝） | 服务端回应「接受 / 拒绝」时触发的回调 |
| `send_goal_options.feedback_callback` | 赋值为一个回调函数，入参是（目标句柄、反馈对象） | 每收到一次连续反馈就触发的回调 |
| `send_goal_options.result_callback` | 赋值为一个回调函数，入参是「结果包装」（含 `code` 与 `result`） | 任务结束时触发一次的回调 |
| `client->async_send_goal(目标对象, 选项)` | `目标对象`（目标对象的共享指针）：要提交的目标，如把 `num` 填好后传入；`选项`（`SendGoalOptions`）：承载三个回调的结构体 | **异步**发送目标，立刻返回，结果全交给回调处理 |
| `rclcpp_action::ResultCode::SUCCEEDED` / `ABORTED` / `CANCELED` | 无参数（枚举值，与 `result.code` 比较） | 结果里的状态码，用来判断任务最终怎么了 |
| `rclcpp_action::Client<接口类型>::SharedPtr` | 无参数（这是类型，不是函数） | 动作客户端对象句柄 |

几点说明：

- **动作服务端的回调不是「一个」而是「三个」**：处理目标（要不要接）、处理取消（同不同意取消）、接受后执行（接了之后干什么）。这是和服务端最大的写法差别。
- **耗时任务必须另开线程**：接受后执行的函数里不能直接做循环计算，否则会卡住主线程的 `spin`，导致收不到新的目标和取消指令。示例工程的做法是把执行函数交给子线程跑，线程句柄存进成员列表，并在析构函数里等它们结束回收（**不要让线程脱离管理**，否则节点销毁时子线程可能访问已析构的对象导致崩溃）。
- **中止必须显式调用**：程序被 Ctrl+C 中断时，必须显式调用 `abort()` 把目标标成「中止」，否则目标句柄析构时会自动尝试取消并发布结果，而那一步会访问已在关闭中的服务端，导致进程崩溃。
- 客户端的「目标回应回调」参数为空时表示**目标被拒绝**，正常接受则能拿到目标句柄。
- 客户端的三个回调是「谁先到谁先跑」，结果回调一定是最后触发的那个；所以打印最终结果要放在结果回调里，而不是发送目标之后就立刻读。
- 用命令行 `ros2 action send_goal` 时加 `--feedback` 才能在终端看到连续反馈。

**案例：累加求和并连续反馈进度（`action_cpp`）**

一个简单的案例：动作通信，客户端提交**一个整数 num**，服务端计算 `1+2+…+num`；每加一个数就**反馈一次当前进度**，全部加完后**把累加和作为最终结果返回**。

对应的 action 接口是示例工程里的 `base_interfaces_demo/action/Progress.action`（注册方式见 3.6.4），原文：

```text
#目标字段：客户端发送的请求
int64 num
---
# 结果字段：任务完成后返回的最终结果
int64 sum
---
# 任务执行过程中周期性发送的中间状态
float64 progress
```

- 第一段（目标）：客户端要累加到的数字 `num`。
- 第二段（结果）：任务结束后返回的累加和 `sum`。
- 第三段（反馈）：执行过程中**周期性**发回的进度 `progress`，取值 0.0 ~ 1.0（用小数表示百分比）。

对应文件：动作服务端 `action_cpp/src/demo01_action_server.cpp`、动作客户端 `action_cpp/src/demo02_action_client.cpp`，动作名 `get_sum`。

```bash
ros2 run action_cpp demo01_action_server  # 先启动动作服务端
ros2 run action_cpp demo02_action_client  # 再启动动作客户端（固定发送 num=10）
```

**服务端**（节点名 `minimal_action_server`）用 `rclcpp_action::create_server<Progress>()` 创建，注册三个回调：

- `handle_goal()`：判断目标要不要接受。这里 `num < 1` 就返回 `GoalResponse::REJECT`，否则返回 `ACCEPT_AND_EXECUTE`。
- `handle_cancel()`：客户端请求取消时调用，这里一律返回 `CancelResponse::ACCEPT`。
- `handle_accepted()`：目标被接受后调用，**另开一个子线程**去跑 `execute()`，把线程存进 `worker_threads_`，这样耗时计算不会卡住主线程的 spin（否则收不到新的请求和取消指令）。代码注释特别提醒**不要用 `detach()`**——线程脱离管理后可能在节点销毁时还在访问服务端，导致崩溃；析构函数里统一 `join()` 收尾。

`execute()` 里是真正的耗时逻辑：用 `rclcpp::Rate loop_rate(10.0)` 把循环压到 10 Hz（每轮睡 0.1 秒，让进度看起来是连续变化的），循环中每加一个数就用 `goal_handle->publish_feedback()` 汇报 `feedback->progress = i / num`；循环里检查 `goal_handle->is_canceling()`，被取消就 `canceled(result)` 返回；正常跑完用 `succeed(result)` 返回结果；如果是因为 Ctrl+C（`rclcpp::ok()` 变 false）而中断，则用 `abort(result)` 显式中止，并用 try/catch 兜底——因为此时通信正在关闭，发布结果本身可能失败抛异常。

**客户端**（节点名 `minimal_action_client`）用 `rclcpp_action::create_client<Progress>()` 创建，`send_goal()` 里先 `wait_for_action_server(10s)` 等服务端上线，再通过 `SendGoalOptions` 注册三个回调：

- `goal_response_callback()`：服务端回应接受或拒绝。回调参数是空的说明目标被拒绝。
- `feedback_callback()`：每收到一次反馈就调用一次，把 0.3 这种小数乘 100 后打印 `当前进度: 30%`。
- `result_callback()`：任务结束时调用一次，先用 `switch (result.code)` 区分 `SUCCEEDED` / `ABORTED` / `CANCELED`，是成功才打印最终结果 `result.result->sum`。

常用命令：

```bash
ros2 action list                                        # 列出当前所有动作
ros2 action list -t                                     # 列出动作并附带接口类型
ros2 action info /get_sum                               # 查看动作的服务端 / 客户端数量
ros2 action send_goal /get_sum base_interfaces_demo/action/Progress "{num: 10}" --feedback   # 发送目标并显示反馈
ros2 interface show base_interfaces_demo/action/Progress    # 查看接口定义
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

#### 3.5.1 为什么只有一个服务端

「一个参数服务上服务端只能有一个」这句话要**分层理解**，否则容易和「参数在系统里只有一份」搞混：

- **论节点**：每个节点**自带**一个参数服务端，由 `rclcpp` / `rclpy` 在节点创建时自动生成，不用自己写；同一个节点上不可能再建出第二个。所以系统里有几个节点，就有几套**各自独立**的参数服务（每套以该节点名为命名空间，如 `/node_a`、`/node_b`）。
- **论某个具体参数**：比如节点 A 上的 `max_speed`，它的提供方（服务端）**只有节点 A 一个**，而使用方（客户端）**可以有任意多个**——命令行 `ros2 param get/set`、其他节点、`rqt_reconfigure` 全都是客户端。
- **论归属**：参数的「归属」是**节点**，不是全局。所以两个节点各自有一个叫 `max_speed` 的参数，它们**互不干扰**，也不构成「多个服务端提供同一个参数」。
- **不要误解**：「不能有多个服务端」≠「参数在系统里只能有一份」。准确说法是：**每个节点有且只有一个参数服务端（就是它自己），而不同节点各有各的一套参数。**

一个边角：ROS2 **没有**「多个节点共享同一个参数服务」的机制。想让多个节点用同一份配置，常见做法是——用参数文件（`.yaml`）在启动时给多个节点各灌一份相同的初值，或者由一个节点把配置通过话题 / 服务广播给别的节点。

#### 3.5.2 参数服务的底层是服务，不是话题

参数服务和话题（topic）是**两套并行的机制**，参数服务的底层**不是话题**：

- **读写走服务**：节点启动时会自动创建一组标准服务，如 `/节点名/get_parameters`、`/节点名/set_parameters`、`/节点名/list_parameters`、`/节点名/describe_parameters`、`/节点名/get_parameter_types`、`/节点名/set_parameters_atomically`。它们都是「请求—应答」，接口定义在 `rcl_interfaces` 包里（如 `rcl_interfaces/srv/SetParameters`）。判断依据很简单：`set` 是有**返回值**的、成功失败会告诉你，这是典型的**服务**特征，而话题是「发了就算、无应答」。
- **变更通知走话题**：唯一和话题沾边的是**参数变更事件**——节点会发布 `/节点名/parameter_events` 话题（类型 `rcl_interfaces/msg/ParameterEvent`），每当参数被创建、修改、删除就发一条。别的程序（如 `rqt_reconfigure`、监控工具）只要订阅这个话题就能感知参数何时变了，而不用轮询。但它只是**通知频道**，不是参数读写的通道。

所以更准确的说法是：**参数服务 ≈ 一组内置服务 + 一个事件话题**。它借话题来「广播变更」，但组织参数读写的主干是服务。

| 机制 | 底层组织方式 |
| --- | --- |
| 话题 topic | 发布 / 订阅（多对多） |
| 服务 service | 请求 / 应答（一对多） |
| 动作 action | 服务 + 话题（goal / result 走服务，feedback 走话题） |
| **参数服务** | **本质是服务**（get / set / list / describe…）+ 一个 `parameter_events` **话题**做变更通知 |

#### 3.5.3 是客户端直接改数据，还是服务端执行？

**是「客户端向服务端发请求，由服务端执行增删改查」，不是客户端直接操作数据。** 这一点容易和「共享数据」这个说法混起来，分开看：

- **客户端只能提要求**：客户端调 `set_parameters` / `get_parameters`，本质是一次服务调用；它拿到的是值对象的**拷贝**，不是数据本身。
- **服务端是唯一持有者 + 唯一执行者**：数据只在服务端进程里，真正的读写在服务端完成。
- **「共享数据」指语义层面**：「共享」是说双方认同一份逻辑数据（客户端 set 成 2.0，服务端读出来也是 2.0），**不是**内存层面的共享、更不是客户端能直接访问那块内存。
- **铁证——服务端可以拒绝**：服务端能注册 `add_on_set_parameters_callback()` 做校验。若客户端能直接改，就根本拦不住；而实际上非法值会被拒绝、数据原样不变，说明修改必须经服务端这一手。

**类比 Web 的前后端分离**（这个比喻很贴切）：

| Web 前后端 | ROS2 参数服务 |
| --- | --- |
| 前端（浏览器）：只发请求、收响应，不碰数据库 | 客户端：只调 `get / set_parameters`，不碰参数数据 |
| 后端（服务进程）：真正读写数据库 | 服务端（节点）：真正读写自己那份参数 |
| 数据在数据库里，前端拿到的是副本 | 数据在服务端进程内存，客户端拿到的是拷贝 |
| 后端有校验、可以拒绝（400 / 403） | 服务端有 `add_on_set_parameters_callback`，可以拒绝（`successful = false`） |
| 前后端用 HTTP 约定接口 | 两端用固定的服务接口约定（`rcl_interfaces` 那套） |

也就是：**客户端 ≈ 前端，服务端 ≈ 后端，参数 ≈ 数据库，「代理模式」说的就是这件事**（呼应 3.5 开头）。

与 Web 不同的三点：①「前后端」不是把一个应用拆两半，而是**每个节点都自带一个「后端」**，任何节点都能当客户端去访问别的节点；② 通信走 **DDS 上的服务调用**，不是 HTTP 的 URL / 方法 / 状态码那一套；③ 能当客户端的角色更多——`ros2 param` 命令、`rqt_reconfigure`、别的节点、甚至**节点自己**（代码里 `this->set_parameter(...)`），而且同进程调用也仍然走「服务端自己处理、触发回调、可能拒绝」这条路。

#### 3.5.4 参数数据存在哪里？

参数数据存在**声明它的那个节点进程的内存里**，形态是节点对象内部的一张「参数表」（name → value 的映射，带类型和描述）：

| 名字（name） | 值（value） | 类型（type） | 描述（description） |
| --- | --- | --- | --- |
| `max_speed` | `2.0` | double | 最大速度 |
| `use_sim_time` | `false` | bool | 是否使用仿真时间 |

- **位置**：参数挂在节点对象内部的参数表上（C++ 里就是 `rclcpp::Node` 内部那张表）；`/node_a` 的参数就存在 `node_a` 那个进程的地址空间里。**节点在哪，数据就在哪。**
- **类型**：声明时就定下来（由默认值决定），之后只能改值、不能改类型。
- **生命周期**：`declare_parameter()` 执行后参数才存在；节点关闭 / 进程退出，这张表**随之消失**；下次重启又回到声明时的默认值。**默认行为下参数是「易失」的——节点一关，改过的值全丢。**

**想持久化只能靠参数文件（YAML）**，ROS2 不提供自动落盘：

```bash
ros2 param dump /节点名           # 把内存里的全部参数导出成 YAML（写盘）
ros2 param load /节点名 xxx.yaml  # 启动后从 YAML 导回内存
```

launch 文件里也可以直接给节点指定参数 YAML，启动时自动加载。注意方向：**YAML 是「外部来源」，不是「存储后端」**——节点跑起来后真身仍在内存里，YAML 只是启动时的初值来源、退出时手动导出的快照。

#### 3.5.5 能不能引入数据库（Redis / SQLite）？

**能引，ROS2 不限制你在节点里接任何存储；但不要拿参数服务当数据库用。** 两者要分清：

| 做的事 | 参数服务 | 数据库 |
| --- | --- | --- |
| 「最大速度」「PID 增益」这类**少量配置** | 正合适 | 杀鸡用牛刀 |
| 「几千条历史记录」「高频写入」「多表关联」 | 别扭 | 正合适 |

参数服务的定位是**「少量、低频、配置类」数据**（强项是节点自带、随取随用、有类型校验和回调、能在线调；弱项是**无事务、无索引、无查询语言、默认不落盘、数据量一大就难受**）。注意——**它只是一份「节点自带的少量配置」背后的接口，不是数据库。**

实际工程里常见的两种组合：

- **方式 A：参数为主，数据库只做「持久化后端」**（推荐）——节点启动时从 Redis / SQLite 读上次的值当作 `declare_parameter()` 的初值，`set_parameter` 时再写回去。对外仍是标准参数服务，数据库只负责「记住上次改成啥了」，解决「节点一关参数全丢」的问题。
- **方式 B：数据库负责数据，参数只做「连接配置」**——把 `db_host`、`db_port`、`redis_url` 这类连接信息作为参数暴露，真正的业务数据读写走数据库客户端，**根本不经过参数服务**。

反面场景：别用参数存「最近 1000 条传感器读数」——每次 `set` 都会触发服务调用 + 变更事件广播，开销远大于直接写内存，参数表越来越大还会让 `list_parameters` 卡顿。正确做法是直接上 SQLite / Redis / 时序库。

**工程实践里的分工**：

- **参数放什么**：启动配置（设备路径、话题名映射、坐标系名）、算法可调量（PID、速度上限、阈值）、连接信息（数据库地址、服务器 IP）。
- **怎么管**：写在 `.yaml` 里，launch 时加载（`parameters=['config/params.yaml']`）；`ros2 param dump` / `load` 导出导入；`rqt_reconfigure` 在线调完再存回 YAML 固化。
- **持久化怎么做**：少量配置跨重启保留 → 还是 YAML（简单、可见、可进 Git）；节点内部业务状态 → 直接在代码里接 SQLite / 文件（如建图写 `.pgm`、标定写 SQLite、日志写 rosbag2）；大量数据 / 多机共享 / 需要查询 → 上 Redis / 数据库 / 时序库，参数只留「怎么连上它」。
- **常见坑**：用参数存大量数据（开销大、`list_parameters` 卡顿）；参数里放密码 / API Key（`ros2 param dump` 会明文导出，**别进参数**）；以为参数自动存盘（重启全丢）；参数名各节点不统一（多机部署配置混乱，要定命名规范）；运行中乱改关键参数（用 `add_on_set_parameters_callback` 做范围校验、拒绝非法值）。

**一句话总结**：参数 = 「少量、低频、配置类」的小盒子，主流用 YAML 管、launch 加载；需要持久化就 YAML 存盘或自己接 SQLite / Redis；大量数据、多机共享、需要查询的直接上数据库，参数只留连接信息。**ROS2 故意不替你做持久化，就是把存储选择权留给工程。**

参数服务**不需要接口文件**：参数会被封装成参数对象（结构体）传递，客户端和服务端操作的都是参数对象，所以它没有对应的 `.msg` / `.srv` / `.action`（见 3.1）。

**服务端 / 客户端的关键接口**（以 rclcpp 为例）

- 服务端：用 `declare_parameter()` 声明参数（声明时给出名字和默认值），用 `get_parameter()` 读、`set_parameter()` 改；想让参数被改时做点事，可以注册回调 `add_on_set_parameters_callback()`。
- 客户端：用 `get_parameters()` / `set_parameters()` 对目标节点的参数做读写，传进去的是参数名的列表。
- 和 `rclcpp::spin` 一样，参数服务的调用也是异步的，需要让线程进入事件循环去等结果。

示例工程里没有自己写参数代码，但它编译出的每个节点都自带一组默认参数（`use_sim_time` 等），所以把节点跑起来就能练习命令行操作。下面是节点「服务端一侧」和「客户端一侧」各自会用到的接口：

| 接口 | 参数（含义 / 要传什么） | 所在侧 | 作用 |
| --- | --- | --- | --- |
| `this->declare_parameter("参数名", 默认值)` | `"参数名"`（字符串）：参数字符串，如 `"max_speed"`；`默认值`（任意受支持类型：布尔 / 整数 / 浮点 / 字符串）：初值，同时决定参数类型，如 `1.0`（double）、`true`（bool）、`"abc"`（string） | 服务端 | 声明一个参数，声明后它才存在于节点上 |
| `this->get_parameter("参数名", 变量)` | `"参数名"`（字符串）：要读的参数名；`变量`（按类型的引用）：用来接收值的变量，如 `double speed;` | 服务端 | 读取参数值到变量里 |
| `this->set_parameter(rclcpp::Parameter("参数名", 值))` | `rclcpp::Parameter("参数名", 值)`：一个参数对象，名字用字符串、值用对应类型，如 `rclcpp::Parameter("max_speed", 2.0)` | 服务端 | 在代码里修改参数值 |
| `this->add_on_set_parameters_callback(回调)` | `回调`（函数对象）：参数被改时触发的函数，入参是被请求修改的参数列表（参数对象列表），返回 `rcl_interfaces::msg::SetParametersResult`（用 `successful` 字段表示是否允许） | 服务端 | 注册回调，参数被修改前后做校验或响应 |
| `this->get_parameters({"名1", "名2"}, 结果)` | `{"名1", "名2"}`（字符串列表）：要读的参数名列表；`结果`（参数对象列表的引用）：接收读取结果的容器 | 客户端 | 一次读取多个参数 |
| `this->set_parameters({rclcpp::Parameter(...), ...})` | `{...}`（参数对象列表）：要修改的参数对象列表，如 `{rclcpp::Parameter("max_speed", 2.0)}` | 客户端 | 一次修改多个参数 |
| `this->list_parameters({}, 深度)` | `{}`（字符串列表）：名字前缀过滤，空表示不过滤；`深度`（整数）：递归深度，如 `1` | 客户端 | 列出目标节点上有哪些参数 |

几点说明：

- `declare_parameter()` **必须先用**：没有声明的参数在默认配置下不允许动态创建，客户端写一个不存在的参数会失败。
- 参数的类型由声明时的默认值决定（`1.0` 是 double、`true` 是 bool、`"abc"` 是 string），之后 `set_parameter` 传错类型会被拒绝。
- 参数读写底层是服务调用，所以示例工程那套「等 service 上线」的思路在这里同样适用——目标节点没运行时，`ros2 param` 命令会报找不到节点。
- 命令行是最省事的用法：`ros2 param list / 节点名`、`ros2 param get / 节点名 参数名`、`ros2 param set / 节点名 参数名 值`。值按 YAML 语法解析，所以 `true` 是布尔、`2.0` 是浮点，写成 `"2.0"` 就成了字符串。

**案例：读取并修改节点上的参数**

一个简单的案例：参数服务，客户端把目标节点上的参数从默认值改成另一个值，再把值读回来确认修改已经生效。

示例工程里没有单独的参数节点，直接用命令行即可复现（示例工程的话题节点 `topic_cpp/demo01_talker_str`、动作节点 `action_cpp/demo01_action_server` 都自带一组默认参数，可以拿来当「被读写的节点」）：

```bash
ros2 run topic_cpp demo01_talker_str &        # 启动一个节点作为「服务端」放在后台
ros2 param list /minimal_publisher            # 看它自己维护了哪些参数（use_sim_time 等）
ros2 param describe /minimal_publisher use_sim_time   # 看参数的类型和默认值
ros2 param get /minimal_publisher use_sim_time        # 读取参数值
ros2 param set /minimal_publisher use_sim_time true   # 修改参数值（值按 YAML 语法解析）
ros2 param get /minimal_publisher use_sim_time        # 再读一次，确认已变成 true
```

常用命令：

```bash
ros2 param list                                         # 列出所有参数
ros2 param list /minimal_publisher                      # 只看指定节点的参数
ros2 param get /minimal_publisher use_sim_time          # 读取参数值
ros2 param set /minimal_publisher use_sim_time true     # 修改参数值（值按 YAML 语法解析）
ros2 param describe /minimal_publisher use_sim_time     # 查看参数的类型和含义
ros2 param delete /minimal_publisher use_sim_time       # 删除动态参数（已声明的参数删不掉）
ros2 param dump /minimal_publisher                      # 把节点的全部参数导出成 YAML
ros2 param load /minimal_publisher params.yaml          # 从 YAML 文件导入参数
```

### 3.6 接口文件（msg / srv / action）

三种接口文件与通信模型一一对应：msg 对应话题通信、srv 对应服务通信、action 对应动作通信。参数服务不需要接口文件。

#### 3.6.1 msg 文件（话题通信）

**文件本身**

- 位置：放在功能包的 `msg/` 目录下，文件名就是消息类型名，首字母必须大写（如 `Student.msg`），一个文件定义一种消息类型。
- 接口全名：`包名/msg/消息名`（如 `base_interfaces_demo/msg/Student`），用 `ros2 interface show` 查看时要带上中间的 `msg`。
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

- 使用其他消息类型时写成 `包名/消息名`，如 `geometry_msgs/Point position`；同一个包内**必须省略**包名，直接写 `Point position`（官方原话：if you want to refer to a message from the same package you **must not** mention the package name）——多写了包名反而会被判为错误。
- msg 只能嵌套 msg，不能直接嵌套 srv。
- ROS2 没有内置 Header：需要时间戳或坐标系时显式写 `std_msgs/Header header`，惯例放在第一个字段；纯时间用 `builtin_interfaces/Time`。

代码里用到的类型名（示例工程的 `Student.msg` 见 3.2 案例二）：

```text
string name # 学生姓名，使用string，而不是String，否则会被当成ROS2内置的消息类型
int32 age # 学生年龄
float64 height # 学生身高
```

#### 3.6.2 srv 文件（服务通信）

**文件本身**

- 位置：放在功能包的 `srv/` 目录下，文件名就是服务类型名，首字母必须大写（如 `AddInts.srv`）。
- 接口全名：`包名/srv/服务名`（如 `base_interfaces_demo/srv/AddInts`），用 `ros2 interface show` 查看时要带上中间的 `srv`。

**结构：两段式**

用**一行 `---`** 把文件分成两段，上段是**请求（request）**，下段是**响应（response）**。两段各自都遵循 msg 的字段语法（字段、注释、常量、类型、数组、复合类型完全一样）。

| 段 | 名称 | 含义 |
| --- | --- | --- |
| 第一段 | 请求（request） | 客户端发送给服务端的数据 |
| 第二段 | 响应（response） | 服务端返回给客户端的数据 |

示例工程里的 `base_interfaces_demo/srv/AddInts.srv` 原文：

```text
# 请求部分
int32 num1
int32 num2
---
# 响应部分
int32 sum
```

- `---` 必须有且只有一行。
- 请求段或响应段都可以为空（表示没有请求参数、或没有返回值），但 `---` 不能省。
- 生成代码里两段分别对应 `AddInts.Request` 和 `AddInts.Response`（写成头文件就是 `AddInts::Request` / `AddInts::Response`）。
- 一个服务只能有一个服务端，客户端可以有多个。

#### 3.6.3 action 文件（动作通信）

**文件本身**

- 位置：放在功能包的 `action/` 目录下，文件名就是动作类型名，首字母必须大写（如 `Progress.action`）。
- 接口全名：`包名/action/动作名`（如 `base_interfaces_demo/action/Progress`），用 `ros2 interface show` 查看时要带上中间的 `action`。

**结构：三段式**

用**两行 `---`** 把文件分成三段，顺序固定，依次是：

| 段 | 名称 | 含义 |
| --- | --- | --- |
| 第一段 | 目标（goal） | 客户端发送的请求数据 |
| 第二段 | 结果（result） | 任务完成后返回的最终结果 |
| 第三段 | 反馈（feedback） | 任务执行过程中周期性发送的中间状态 |

三段都遵循 msg 的字段语法。

示例工程里的 `base_interfaces_demo/action/Progress.action` 原文：

```text
#目标字段：客户端发送的请求
int64 num
---
# 结果字段：任务完成后返回的最终结果
int64 sum
---
# 任务执行过程中周期性发送的中间状态
float64 progress
```

- 两行 `---` 必须都存在，即使某一段为空也要保留分隔线，否则解析报错。
- 生成代码里三段分别对应 `Progress.Goal`、`Progress.Result`、`Progress.Feedback`。
- 用了 action，功能包里除了 `rosidl_default_generators` 还要依赖 `action_msgs`（示例工程的 `base_interfaces_demo/package.xml` 里就有 `<depend>action_msgs</depend>`）。
- action 底层是「服务 + 话题」的组合：goal / result 走服务，feedback 走话题，所以它既能请求响应、又能连续反馈；适用于耗时较长、需要反馈进度的任务。

#### 3.6.4 功能包注册方法

msg、srv、action 三类接口的注册方式完全一样，都靠功能包里的 `rosidl_generate_interfaces()` 统一注册。只写接口文件还不够，不注册的话编译时不会生成对应代码。

`package.xml` 中添加：

```xml
<buildtool_depend>rosidl_default_generators</buildtool_depend>
<exec_depend>rosidl_default_runtime</exec_depend>
<member_of_group>rosidl_interface_packages</member_of_group>
```

示例工程的 `base_interfaces_demo/package.xml` 用的是下面这组（`<build_depend>` 与官方现行教程的 `<buildtool_depend>` 略有差别，但功能上都能用；官方推荐写成 `<buildtool_depend>`）：

```xml
<build_depend>rosidl_default_generators</build_depend>
<exec_depend>rosidl_default_runtime</exec_depend>
<depend>action_msgs</depend>
<member_of_group>rosidl_interface_packages</member_of_group>
```

`CMakeLists.txt` 中添加（三类接口文件写在同一个 `rosidl_generate_interfaces()` 里，示例工程就是这个写法）：

```cmake
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/Student.msg"
  "srv/AddInts.srv"
  "action/Progress.action"
)
```

编译并验证：

```bash
colcon build
. install/setup.bash
ros2 interface show base_interfaces_demo/msg/Student        # 验证 msg
ros2 interface show base_interfaces_demo/srv/AddInts        # 验证 srv
ros2 interface show base_interfaces_demo/action/Progress    # 验证 action
```

如果别的功能包要使用这些接口，需要在使用方的 `package.xml` 中声明依赖：`<depend>接口包名</depend>`（示例工程里 `topic_cpp` / `service_cpp` / `action_cpp` 的 `package.xml` 都加了 `<depend>base_interfaces_demo</depend>`）。

### 3.7 分布式通信（DDS 域）

分布式通信是指可以通过网络在不同主机之间实现数据交互的一种通信策略。

ROS2 所基于的中间件是 DDS（数据分发服务，采用以数据为中心的发布/订阅模型），通过 DDS 的域机制，可以实现分布式通信。

默认的域 ID 是 0，所有的节点都在同一个域中。

设置域 ID（在 run 启动节点之前执行）：

```bash
export ROS_DOMAIN_ID=6
```

ID 不是随意设置的，在 `[0, 101]` 之间。

- 每个域的节点个数要小于 120 个。
- 如果域 ID 为 101，则域节点总数要小于 54 个。

域 ID 值的计算规则（了解即可）：

1. DDS 是基于 TCP/IP 或 UDP/IP 网络通信协议的，网络通信时需要指定端口号，端口号由 2 个字节的无符号整数表示，其取值范围在 `[0, 65535]` 之间；
2. 端口号的分配也是有其规则的，并非可以任意使用的，根据 DDS 协议规定以 7400 作为起始端口，也即可用端口为 `[7400, 65535]`，又已知按照 DDS 协议默认情况下，每个域 ID 占用 250 个端口，那么域 ID 的个数为：(65535-7400)/250 = 232(个)，对应的其取值范围为 `[0, 231]`；
3. 操作系统还会设置一些预留端口，在 DDS 中使用端口时，还需要避开这些预留端口，以免使用中产生冲突，不同的操作系统预留端口又有所差异，其最终结果是，在 Linux 下，可用的域 ID 为 `[0, 101]` 与 `[215-231]`，在 Windows 和 Mac 中可用的域 ID 为 `[0, 166]`，综上，为了兼容多平台，建议域 ID 在 `[0, 101]` 范围内取值。
4. 每个域 ID 默认占用 250 个端口，且每个 ROS2 节点需要占用两个端口，另外，按照 DDS 协议每个域 ID 的端口段内，第 1、2 个端口是 Discovery Multicast 端口与 User Multicast 端口，从第 11、12 个端口开始是域内第一个节点的 Discovery Unicast 端口与 User Unicast，后续节点所占用端口依次顺延，那么一个域 ID 中的最大节点个数为：(250-10)/2 = 120(个)；
5. 特殊情况：域 ID 值为 101 时，其后半段端口属于操作系统的预留端口，其节点最大个数为 54 个。


### 3.8 工作空间覆盖

**场景**

同一工作空间下不允许出现功能包重名的情况，但是当存在多个工作空间时，不同工作空间下的功能包是可以重名的，那么当功能包重名时，会调用哪一个呢？

> 比如：自定义工作空间 A 存在功能包 `turtlesim`，自定义工作空间 B 也存在功能包 `turtlesim`，当然系统自带工作空间也存在 `turtlesim`，如果调用 `turtlesim` 包，会调用哪个工作空间中的呢？

**概念**

存在多个工作空间，不同的工作空间下面出现重名的功能包，重名功能包的调用会产生覆盖的情况。

**作用**

没什么用，这种情况是需要极力避免出现的。

**原因**

这与 `~/.bashrc` 中不同工作空间的 `setup.bash` 文件的加载顺序有关：

1. ROS2 会解析 `~/.bashrc` 文件，并生成全局环境变量 `AMENT_PREFIX_PATH`（C++）与 `PYTHONPATH`（Python），环境变量的值由功能包名称组成；
2. 两个变量的值的设置与 `~/.bashrc` 中的 `setup.bash` 的配置顺序有关，对于自定义的工作空间而言，后配置的优先级更高，主要表现在后配置的工作空间的功能包在环境变量值组成的前部，而前配置工作空间的功能包在环境变量值组成的后部分，如果更改两个自定义工作空间在 `~/.bashrc` 中的配置顺序，那么变量值也将相应更改，但是 ROS2 系统工作空间的配置始终处于最后。
3. 调用功能包时，会按照 `AMENT_PREFIX_PATH` 或 `PYTHONPATH` 中包配置顺序从前往后依次查找相关功能包，查找到功能包时会停止搜索，也即配置在前的会优先执行。

使用如下命令查看当前的包搜索路径：

```bash
echo $AMENT_PREFIX_PATH
```

简单来说就是从前往后找，找到了就不找了，配置在前的就会优先执行。

**演示**

1. 分别在不同的工作空间下创建 `turtlesim` 功能包。

终端下进入 `ws00_helloworld` 的 `src` 目录，新建功能包：

```bash
ros2 pkg create turtlesim --node-name turtlesim_node
```

为了方便查看演示结果，将默认生成的 `turtlesim_node.cpp` 中的打印内容修改为：`ws00_helloworld turtlesim`。

终端下进入 `ws01_plumbing` 的 `src` 目录，新建功能包：

```bash
ros2 pkg create turtlesim --node-name turtlesim_node
```

为了方便查看演示结果，将默认生成的 `turtlesim_node.cpp` 中的打印内容修改为：`ws01_plumbing turtlesim`。

2. 在 `~/.bashrc` 文件下追加如下内容（注意先 `source` 的在后、后 `source` 的优先）：

```bash
source /home/ros2/ws00_helloworld/install/setup.bash
source /home/ros2/ws01_plumbing/install/setup.bash
```

修改完毕后，保存并关闭文件。

3. 新建终端，输入如下指令：

```bash
ros2 run turtlesim turtlesim_node
```

输出结果为：`ws01_plumbing turtlesim`，也即执行的是 `ws01_plumbing` 功能包下的 `turtlesim`，而 `ws00_helloworld` 下的 `turtlesim` 与内置的 `turtlesim` 被覆盖了。

**隐患**

前面提到，工作空间覆盖的情况是需要极力避免出现的，因为它会导致一些安全隐患：

1. 可能会出现功能包调用混乱，出现实际调用与预期调用结果不符的情况；
2. 即便可以通过 `~/.bashrc` 来配置不同工作空间的优先级，但是经过测试，修改 `~/.bashrc` 文件之后不一定马上生效，还需要删除工作空间下 `build` 与 `install` 目录重新编译，才能生效，这个过程繁琐且有不确定性。

**解决方案**

在实际工作中，需要制定明确的包命名规范，避免包重名情况。

### 3.9 元功能包

**场景**

完成一个系统性的功能，可能涉及到多个功能包，比如实现了机器人导航模块，该模块下有地图、定位、路径规划……等不同的子级功能包。那么调用者安装该模块时，需要逐一地安装每一个功能包吗？

显而易见的，逐一安装功能包的效率低下，在 ROS2 中，提供了一种方式可以将不同的功能包打包成一个功能包，当安装某个功能模块时，直接调用打包后的功能包即可，该包又称之为**元功能包（metapackage）**。

**概念**

MetaPackage 是 Linux 的一个文件管理系统的概念。是 ROS2 中的一个**虚包**，里面没有实质性的内容，但是它依赖了其他的软件包，通过这种方法可以把其他包组合起来，我们可以认为它是一本书的目录索引，告诉我们这个包集合中有哪些子包，并且该去哪里下载。

例如：`sudo apt install ros-<ros2-distro>-desktop` 命令安装 ROS2 时就使用了元功能包，该元功能包依赖于 ROS2 中的其他一些功能包，安装该包时会一并安装依赖。

**作用**

方便用户的安装，我们只需要这一个包就可以把其他相关的软件包组织到一起安装了。

**实现**

1. 新建一个功能包：

```bash
ros2 pkg create tutorails_plumbing
```

2. 修改 `package.xml` 文件，用 `<exec_depend>` 添加执行时所依赖的包（占位符写法，把下面这些换成自己工程里实际的包名）：

```xml
<?xml version="1.0"?>
<?xml-model href="http://download.ros.org/schema/package_format3.xsd" schematypens="http://www.w3.org/2001/XMLSchema"?>
<package format="3">
  <name>tutorails_plumbing</name>
  <version>0.0.0</version>
  <description>TODO: Package description</description>
  <maintainer email="ros2@todo.todo">ros2</maintainer>
  <license>TODO: License declaration</license>

  <buildtool_depend>ament_cmake</buildtool_depend>

  <exec_depend>base_interfaces_demo</exec_depend>
  <exec_depend>topic_cpp</exec_depend>
  <exec_depend>service_cpp</exec_depend>
  <exec_depend>action_cpp</exec_depend>

  <export>
    <build_type>ament_cmake</build_type>
  </export>
</package>
```

3. 文件 `CMakeLists.txt` 内容如下：

```cmake
cmake_minimum_required(VERSION 3.8)
project(tutorails_plumbing)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)

ament_package()
```

**小结**

1. 创建一个虚功能包，里面没有任何东西；
2. 修改 `package.xml` 文件，用 `<exec_depend>` 来添加执行时所依赖的包；
3. 调用这个虚功能包的时候，自动引用对应的功能包。

### 3.10 节点重名

**问题描述**

在 ROS2 中不同的节点可以有相同的节点名称，比如可以启动多个 `turtlesim_node` 节点，这些节点的名称都是 `turtlesim`。节点重名虽然是被允许的，但是开发者应该主动避免这种情况，因为节点重名时可能会导致操作上的混淆：仍以启动了多个 `turtlesim_node` 节点为例，当使用计算图（`rqt_graph`）查看节点运行状态时，由于它们的节点名称一致，那么虽然实际有多个节点，但是在计算图上只显示一个。并且节点名称也会和话题名称、服务名称、动作名称、参数等产生关联，届时也可能会导致通信逻辑上的混乱。

那么在 ROS2 中如何避免节点重名呢？

**解决思路**

避免重名问题，一般有两种策略：

1. **名称重映射**，也即为节点起别名；
2. **命名空间**，是为节点名称添加前缀，可以有多级，格式：`/xxx/yyy/zzz`。

这也是在 ROS2 中解决重名问题的常用策略。

**解决方案**

上述两种策略的实现途径主要有如下三种：

1. `ros2 run` 命令实现；
2. launch 文件实现；
3. 编码实现。

#### 3.10.1 ros2 run 设置节点名称

**1. ros2 run 设置命名空间**

语法：`ros2 run 包名 节点名 --ros-args --remap __ns:=命名空间`

```bash
ros2 run topic_cpp demo01_talker_str --ros-args --remap __ns:=/t1
```

使用 `ros2 node list` 查看节点信息，显示结果：

```text
/t1/minimal_publisher
```

**2. ros2 run 名称重映射**

为节点起别名。

语法：`ros2 run 包名 节点名 --ros-args --remap __name:=新名称` 或 `ros2 run 包名 节点名 --ros-args --remap __node:=新名称`

```bash
ros2 run topic_cpp demo01_talker_str --ros-args --remap __name:=turtle1
```

使用 `ros2 node list` 查看节点信息，显示结果：

```text
/turtle1
```

**3. ros2 run 命名空间与名称重映射叠加**

语法：`ros2 run 包名 节点名 --ros-args --remap __ns:=新名称 --remap __name:=新名称`

```bash
ros2 run topic_cpp demo01_talker_str --ros-args --remap __ns:=/t1 --remap __name:=turtle1
```

使用 `ros2 node list` 查看节点信息，显示结果：

```text
/t1/turtle1
```

#### 3.10.2 launch 文件设置节点名称

在 ROS2 中 launch 文件可以由 Python、XML 或 YAML 三种语言编写（关于 launch 文件的基本使用见 5.1），每种实现方式都可以设置节点的命名空间或为节点起别名。

**1. Python 方式实现的 launch 文件设置命名空间与名称重映射**

在 Python 方式实现的 launch 文件中，可以通过类 `launch_ros.actions.Node` 来创建被启动的节点对象，在对象的构造函数中提供了 `name` 和 `namespace` 参数来设置节点的名称与命名空间，使用示例如下：

```python
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():

    return LaunchDescription([
        Node(package="topic_cpp", executable="demo01_talker_str", name="turtle1"),
        Node(package="topic_cpp", executable="demo01_talker_str", namespace="t1"),
        Node(package="topic_cpp", executable="demo01_talker_str", namespace="t1", name="turtle1")
    ])
```

**2. XML 方式实现的 launch 文件设置命名空间与名称重映射**

在 XML 方式实现的 launch 文件中，可以通过 `node` 标签中 `name` 和 `namespace` 属性来设置节点的名称与命名空间，使用示例如下：

```xml
<launch>
    <node pkg="topic_cpp" exec="demo01_talker_str" name="turtle1" />
    <node pkg="topic_cpp" exec="demo01_talker_str" namespace="t1" />
    <node pkg="topic_cpp" exec="demo01_talker_str" namespace="t1" name="turtle1" />
</launch>
```

**3. YAML 方式实现的 launch 文件设置命名空间与名称重映射**

在 YAML 方式实现的 launch 文件中，可以通过 `node` 属性中 `name` 和 `namespace` 属性来设置节点的名称与命名空间，使用示例如下：

```yaml
launch:
- node:
    pkg: topic_cpp
    exec: demo01_talker_str
    name: turtle1
- node:
    pkg: topic_cpp
    exec: demo01_talker_str
    namespace: t1
- node:
    pkg: topic_cpp
    exec: demo01_talker_str
    namespace: t1
    name: turtle1
```

**4. 测试**

上述三种方式在设置命名空间与名称重映射时虽然语法不同，但是实现功能类似，都是启动了三个节点，第一个节点设置了节点名称，第二个节点设置了命名空间，第三个节点既设置了命名空间又设置了节点名称。分别执行三个 launch 文件，然后使用 `ros2 node list` 查看节点信息，显示结果都如下所示：

```text
/t1/turtle1
/t1/minimal_publisher
/turtle1
```

#### 3.10.3 编码设置节点名称

在 rclcpp 和 rclpy 中，节点类的构造函数中，都分别提供了设置节点名称与命名空间的参数。

**1. rclcpp 中的相关 API**

rclcpp 中节点类的构造函数如下：

```cpp
Node (节点名字符串, const NodeOptions &options=NodeOptions())
Node (节点名字符串, 命名空间字符串, const NodeOptions &options=NodeOptions())
```

构造函数 1 中可以直接通过 `node_name` 设置节点名称，构造函数 2 既可以通过 `node_name` 设置节点名称也可以通过 `namespace_` 设置命名空间。

> Python（rclpy）中节点构造函数的写法与参数含义，见《ROS学习-Python.md》7.1。

### 3.11 话题重名

**问题描述**

节点名称可能出现重名的情况，同理话题名称也可能重名，不过与节点重名不同的是，有些场景下需要避免话题重名的情况，但有些场景下又需要将不同的两个话题名称修改为相同。

> 在 ROS2 不同的节点之间通信都依赖于话题，话题名称也可能出现重名的情况，话题重名时，系统虽然不会抛出异常，但是通过相同话题名称关联到一起的节点可能并不属于同一通信逻辑，从而导致通信错乱，甚至出现异常。这种情况下可能就需要将相同的话题名称设置为不同。
>
> 又或者，两个节点是属于同一通信逻辑的，但是节点之间话题名称不同，导致通信失败。这种情况下就需要将两个节点的话题名称由不同修改为相同。

那么如何修改话题名称呢？

**解决思路**

与节点重名的解决思路类似，为了避免话题重名问题，一般有两种策略：

1. **名称重映射**，也即为话题名称起别名；
2. **命名空间**，是为话题名称添加前缀，可以有多级，格式：`/xxx/yyy/zzz`。

需要注意的是，通过命名空间设置话题名称时，需要保证话题是**非全局话题**。

**解决方案**

与节点重名解决方案类似，修改话题名称的方式主要有如下三种：

1. `ros2 run` 命令实现；
2. launch 文件实现；
3. 编码实现。

#### 3.11.1 ros2 run 修改话题名称

**1. ros2 run 设置命名空间**

该实现与 3.10.1 中演示的语法使用一致。

语法：`ros2 run 包名 节点名 --ros-args --remap __ns:=命名空间`

```bash
ros2 run topic_cpp demo01_talker_str --ros-args --remap __ns:=/t1
```

使用 `ros2 topic list` 查看话题信息，显示结果：

```text
/t1/topic
```

节点下的话题已经添加了命名空间前缀。

**2. ros2 run 话题名称重映射**

为话题起别名。

语法：`ros2 run 包名 节点名 --ros-args --remap 原话题名称:=新话题名称`

```bash
ros2 run topic_cpp demo01_talker_str --ros-args --remap /topic:=/cmd_vel
```

使用 `ros2 topic list` 查看话题信息，显示结果：

```text
/cmd_vel
```

节点下的话题 `/topic` 已经被修改为了 `/cmd_vel`。

**注意**

当为节点添加命名空间时，节点下的所有非全局话题都会加前缀命名空间，而重映射的方式只是修改指定话题。

#### 3.11.2 launch 文件修改话题名称

**1. Python 方式实现的 launch 文件修改话题名称**

在 Python 方式实现的 launch 文件中，可以通过类 `launch_ros.actions.Node` 的构造函数中的参数 `remappings` 修改话题名称，使用示例如下：

```python
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():

    return LaunchDescription([
        Node(package="topic_cpp", executable="demo01_talker_str", namespace="t1"),
        Node(package="topic_cpp",
            executable="demo01_talker_str",
            remappings=[("/topic","/cmd_vel")]
        )
    ])
```

**2. XML 方式实现的 launch 文件修改话题名称**

在 XML 方式实现的 launch 文件中，可以通过 `node` 标签的子标签 `remap`（属性 `from` 取值为被修改的话题名称，属性 `to` 的取值为修改后的话题名称）修改话题名称，使用示例如下：

```xml
<launch>
    <node pkg="topic_cpp" exec="demo01_talker_str" namespace="t1" />
    <node pkg="topic_cpp" exec="demo01_talker_str">
        <remap from="/topic" to="/cmd_vel" />
    </node>
</launch>
```

**3. YAML 方式实现的 launch 文件修改话题名称**

在 YAML 方式实现的 launch 文件中，可以通过 `node` 属性中 `remap`（属性 `from` 取值为被修改的话题名称，属性 `to` 的取值为修改后的话题名称）修改话题名称，使用示例如下：

```yaml
launch:
- node:
    pkg: topic_cpp
    exec: demo01_talker_str
    namespace: t1
- node:
    pkg: topic_cpp
    exec: demo01_talker_str
    remap:
    -
        from: "/topic"
        to: "/cmd_vel"
```

**4. 测试**

上述三种方式在修改话题名称时虽然语法不同，但是实现功能类似，都是启动了两个节点，一个节点添加了命名空间，另一个节点将话题从 `/topic` 映射到了 `/cmd_vel`。使用 `ros2 topic list` 查看话题信息，显示结果：

添加命名空间的节点对应的话题为：

```text
/t1/topic
```

重映射的节点对应的话题为：

```text
/cmd_vel
```

#### 3.11.3 编码设置话题名称

**话题分类**

话题的名称的设置是与节点的命名空间、节点的名称有一定关系的，话题名称大致可以分为三种类型：

- **全局话题**（话题参考 ROS 系统，与节点命名空间平级）；
- **相对话题**（话题参考的是节点的命名空间，与节点名称平级）；
- **私有话题**（话题参考节点名称，是节点名称的子级）。

总之，以编码方式设置话题名称是比较灵活的。本节介绍如何在 rclcpp 中设置不同类型的话题（rclpy 的写法见《ROS学习-Python.md》7.2）；三种类型的划分与规则对两种语言一致。

**准备**

请先创建 C++ 功能包以及节点，且假定在创建节点时，使用的命名空间为 `xxx`，节点名称为 `yyy`。

**话题设置**

**1. 全局话题**

格式：定义时以 `/` 开头的名称，和命名空间、节点名称无关。

```cpp
publisher_ = this->create_publisher<std_msgs::msg::String>("/topic/chatter", 10);
```

话题：话题名称为 `/topic/chatter`，与命名空间 `xxx` 以及节点名称 `yyy` 无关。

**2. 相对话题**

格式：非 `/` 开头的名称，参考命名空间设置话题名称，和节点名称无关。

```cpp
publisher_ = this->create_publisher<std_msgs::msg::String>("topic/chatter", 10);
```

话题：话题名称为 `/xxx/topic/chatter`，与命名空间 `xxx` 有关，与节点名称 `yyy` 无关。

**3. 私有话题**

格式：定义时以 `~/` 开头的名称，和命名空间、节点名称都有关系。

```cpp
publisher_ = this->create_publisher<std_msgs::msg::String>("~/topic/chatter", 10);
```

话题：话题名称为 `/xxx/yyy/topic/chatter`，使用命名空间 `xxx` 以及节点名称 `yyy` 作为话题名称前缀。

综上，话题名称设置规则在 rclcpp 与 rclpy 中基本一致，且上述规则也同样适用于 `ros2 run` 指令与 launch 文件。

### 3.12 时间相关 API

在话题通信案例中，要求话题发布方按照一定的频率发布消息，我们实现时是通过定时器来控制发布频率的。其实，除了定时器之外，ROS2 中还提供了一组与时间相关的 API：Rate、Time、Duration。

#### 3.12.1 Rate

**1. rclcpp 中的 Rate**

示例：周期性输出一段文本。

```cpp
#include "rclcpp/rclcpp.hpp"

int main(int argc, char ** argv)
{
  rclcpp::init(argc,argv);
  auto node = rclcpp::Node::make_shared("rate_demo");
  // rclcpp::Rate rate(1000ms); // 创建 Rate 对象方式1（`1000ms` 是时长字面量后缀，需先引入 chrono_literals 命名空间，下同）
  rclcpp::Rate rate(1.0); // 创建 Rate 对象方式2
  while (rclcpp::ok())
  {
    RCLCPP_INFO(node->get_logger(),"hello rate");
    // 休眠
    rate.sleep();
  }

  rclcpp::shutdown();
  return 0;
}
```

- 创建 Rate 对象有两种方式：传时长（如 `1000ms`）或传频率（如 `1.0` 表示 1 Hz）；
- 在循环里调用 `rate.sleep()` 休眠，从而把循环压到指定频率。

> Python（rclpy）中 Rate 的写法见《ROS学习-Python.md》7.3。

#### 3.12.2 Time

**1. rclcpp 中的 Time**

示例：创建 Time 对象，并调用其函数。

```cpp
#include "rclcpp/rclcpp.hpp"

int main(int argc, char const *argv[])
{
    rclcpp::init(argc,argv);
    auto node = rclcpp::Node::make_shared("time_demo");

    // 创建 Time 对象
    rclcpp::Time t1(10500000000L);
    rclcpp::Time t2(2,1000000000L);
    // 通过节点获取当前时刻。
    // rclcpp::Time roght_now = node->get_clock()->now();
    rclcpp::Time roght_now = node->now();
    RCLCPP_INFO(node->get_logger(),"s = %.2f, ns = %ld",t1.seconds(),t1.nanoseconds());
    RCLCPP_INFO(node->get_logger(),"s = %.2f, ns = %ld",t2.seconds(),t2.nanoseconds());
    RCLCPP_INFO(node->get_logger(),"s = %.2f, ns = %ld",roght_now.seconds(),roght_now.nanoseconds());

    rclcpp::shutdown();

    return 0;
}
```

> Python（rclpy）中 Time 的写法见《ROS学习-Python.md》7.3。

#### 3.12.3 Duration

**1. rclcpp 中的 Duration**

示例：创建 Duration 对象，并调用其函数。

```cpp
#include "rclcpp/rclcpp.hpp"

int main(int argc, char const *argv[])
{
    rclcpp::init(argc,argv);
    auto node = rclcpp::Node::make_shared("duration_node");

    // 创建 Duration 对象
    rclcpp::Duration du1(1s);
    rclcpp::Duration du2(2,500000000);

    RCLCPP_INFO(node->get_logger(),"s = %.2f, ns = %ld", du2.seconds(),du2.nanoseconds());

    rclcpp::shutdown();
    return 0;
}
```

> 上面 `rclcpp::Duration du1(1s)` 里的 `1s` 是 C++ 时长字面量后缀，需要先引入 chrono_literals 命名空间才能直接用；也可以写成 `rclcpp::Duration du1(1, 0)` 这种「秒 + 纳秒」的等价形式。

> Python（rclpy）中 Duration 的写法见《ROS学习-Python.md》7.3。

#### 3.12.4 Time 与 Duration 运算

**1. rclcpp 中的运算**

示例：Time 以及 Duration 的相关运算。

```cpp
#include "rclcpp/rclcpp.hpp"

int main(int argc, char const *argv[])
{
    rclcpp::init(argc,argv);
    auto node = rclcpp::Node::make_shared("time_opt_demo");

    rclcpp::Time t1(1,500000000);
    rclcpp::Time t2(10,0);

    rclcpp::Duration du1(3,0);
    rclcpp::Duration du2(5,0);

    // 比较
    RCLCPP_INFO(node->get_logger(),"t1 >= t2 ? %d",t1 >= t2);
    RCLCPP_INFO(node->get_logger(),"t1 < t2 ? %d",t1 < t2);
    // 数学运算
    rclcpp::Time t3 = t2 + du1;
    rclcpp::Time t4 = t1 - du1;
    rclcpp::Duration du3 = t2 - t1;

    RCLCPP_INFO(node->get_logger(), "t3 = %.2f",t3.seconds());
    RCLCPP_INFO(node->get_logger(), "t4 = %.2f",t4.seconds());
    RCLCPP_INFO(node->get_logger(), "du3 = %.2f",du3.seconds());

    RCLCPP_INFO(node->get_logger(),"--------------------------------------");
    // 比较
    RCLCPP_INFO(node->get_logger(),"du1 >= du2 ? %d", du1 >= du2);
    RCLCPP_INFO(node->get_logger(),"du1 < du2 ? %d", du1 < du2);
    // 数学运算
    rclcpp::Duration du4 = du1 * 3.0;
    rclcpp::Duration du5 = du1 + du2;
    rclcpp::Duration du6 = du1 - du2;

    RCLCPP_INFO(node->get_logger(), "du4 = %.2f",du4.seconds());
    RCLCPP_INFO(node->get_logger(), "du5 = %.2f",du5.seconds());
    RCLCPP_INFO(node->get_logger(), "du6 = %.2f",du6.seconds());

    rclcpp::shutdown();
    return 0;
}
```

> Python（rclpy）中 Time 与 Duration 的运算见《ROS学习-Python.md》7.3。

**运算规则小结**

- **Time 与 Time 运算**：可以比较大小；两个 Time 相减得到一个 Duration（时间点之间的间隔）。Time 与 Time 不能相加，没有意义。
- **Time 与 Duration 运算**：Time + Duration、Time - Duration 都得到 Time（时间点偏移一段时间）；可以比较大小。
- **Duration 与 Duration 运算**：可以比较大小；可以相加减、可以乘一个倍数，结果仍是 Duration。

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
mkdir -p ros2_learning_plumbing/src  # 创建一个工作空间，已经有了就不用创建了

cd ros2_learning_plumbing/src  # 进入源码目录

# 调用 ROS2 的创建功能包命令
# --build-type 指定构建系统（ament_cmake），--dependencies 声明依赖（rclcpp），--node-name 设置节点名称
ros2 pkg create topic_cpp --build-type ament_cmake --dependencies rclcpp std_msgs --node-name demo01_talker_str
```

**2. 编辑源文件**

编辑生成的包下面的 src 目录下的源文件。示例工程里的发布方 `topic_cpp/src/demo01_talker_str.cpp` 完整代码见附录 A，这里只看最核心的骨架：

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

- 上面是最小骨架，只创建节点、打印一句话。
- 实际项目（包括示例工程）用的是**自定义节点类**的写法：`class Xxx : public rclcpp::Node`，把发布方、定时器等作为成员变量，`main()` 里仍是最简单的 `init` → `spin` → `shutdown` 三步（见 2.2 的编码规范）。

**3. 编辑配置文件**

不用完全自己写，以后只需要知道关键的地方是什么意思、哪些地方需要修改即可。CMAKE 文件、XML 文件（两者的作用与改法见 2.3）。

新增可执行文件时，`CMakeLists.txt` 里要有 `add_executable()`、`ament_target_dependencies()`、`install(TARGETS ...)` 三件套，写法见 2.3.3。

**4. 编译**

使用 colcon 进行 build。

```bash
cd ..  # 进入工作空间目录
colcon build  # 使用 colcon 进行 build
```

**5. 运行**

```bash
. install/setup.bash
ros2 run topic_cpp demo01_talker_str  # ros2 run <包名> <可执行名>
```

## 五、常用工具

既有 ROS2 的命令行工具，也有图形化工具 RQT。

### 5.1 launch 与 rosbag2

launch 文件：通过 launch 文件，可以批量启动 ROS2 节点，这是在构建大型项目时启动多节点的常用方式。

#### 5.1.1 launch 文件是什么

**launch 文件是一份「启动清单」**，用来描述「一个 ROS2 系统该怎么被拉起来」：要启动哪些节点、每个节点的命名空间 / 参数 / 话题重映射是什么、要附带拉起哪些外部程序（rviz2、rosbag2 等），以及它们之间的启动先后关系。

一句话：**它是 ROS2 系统的「一键启动方案」。**

需要强调：launch 文件**自己不产生节点**，它只是去调起别的可执行程序。节点仍是那些用 C++ / Python 写好、能用 `ros2 run` 直接跑的目标；launch 相当于把「你本来要手动敲的一串 `ros2 run` + `ros2 param set`」打包成一份文件。所以它属于**配置**，不属于**实现**。

#### 5.1.2 它解决什么问题

1. **批量启动**：一个真实系统动辄十几个节点，手工开十几个终端逐个 `ros2 run` 既麻烦又容易漏、容易错。launch 一条命令全拉起。
2. **配置集中**：节点的参数、命名空间、话题重映射这些「启动时就要定好」的东西统一写在一处，可复用、可版本管理、可分享。

对比一下同一个系统（1 个发布方 + 1 个订阅方 + 参数 + rviz2）的两种启动方式：

- 手工：开 4 个终端，分别敲 `ros2 run ...`、`ros2 param set ...`、`ros2 run rviz2 rviz2`，顺序还得记牢；
- 用 launch：一条 `ros2 launch 包名 xx.launch.py`，上述全部自动完成。

#### 5.1.3 三种写法

launch 文件可以用 **Python、XML、YAML** 三种格式编写，三者功能完全等价，只是写法不同（同一件事——启动一个叫 `turtle1` 的节点——的三种写法）：

**1. Python（`.launch.py`）—— 最灵活，能写条件 / 循环**

```python
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(package="topic_cpp", executable="demo01_talker_str", name="turtle1")
    ])
```

**2. XML（`.launch.xml`）—— 不用写代码，结构清晰**

```xml
<launch>
    <node pkg="topic_cpp" exec="demo01_talker_str" name="turtle1" />
</launch>
```

**3. YAML（`.launch.yaml`）—— 最简洁，但缩进必须对**

```yaml
launch:
- node:
    pkg: topic_cpp
    exec: demo01_talker_str
    name: turtle1
```

三者的选择建议：**做简单启动用 XML / YAML 就够了**（不用写代码、一眼看懂）；需要写条件判断、循环生成多个节点、做复杂组合时，才用 Python。本笔记 3.10.2、3.11.2 展示命名空间 / 重映射时也是三种写法并列，可对照查看。

#### 5.1.4 怎么运行、放在哪

运行：

```bash
ros2 launch <包名> <launch文件名>   # 例如 ros2 launch topic_cpp demo01_talker.launch.py
```

放置位置与打包：launch 文件通常放在功能包下的 `launch/` 目录，并在构建脚本里声明，否则安装后 `ros2 launch` 找不到它——

- **ament_cmake 包**：在 `CMakeLists.txt` 里用 `install(DIRECTORY launch ...)`（写法见 2.3.3）；
- **ament_python 包**：在 `setup.py` 的 `data_files` 里加 `(目标路径, [文件列表])` 项（见《ROS学习-Python.md》3.2）。

#### 5.1.5 launch 文件里通常能写什么

| 内容 | 说明 |
| --- | --- |
| 启动节点（node） | 指定「包名 + 可执行名」，可附加 `name`（改节点名）、`namespace`（命名空间）、`parameters`（参数）、`remappings`（话题重映射） |
| 设置参数（param） | 给节点设参数值，或从 YAML 文件批量加载（`parameters=['config/params.yaml']`） |
| 名称重映射（remap） | 把话题名从 `/topic` 改成 `/cmd_vel` 之类（见 3.11.2） |
| 包含其它 launch（include） | 一个 launch 里包含另一个，做模块化组合 |
| 执行外部命令（executable） | 拉起 rviz2、rosbag2，甚至普通 shell 命令 |
| 启动顺序 / 条件 | Python 版可写延迟启动、条件分支等（XML / YAML 也有对应标签，但表达力较弱） |


### 5.2 坐标变换（TF）

TF 坐标变换：能实现不同机器人、或者机器人的不同部件之间相对关系的转换。

### 5.3 可视化（rviz2 / RQT）

可视化：内置三维可视化 rviz2，以图形化的方式显示机器人模型或显示机器人系统中的一些抽象数据。

RQT 是一个图形化框架，把各种调试工具做成「插件」，可以在一个窗口里同时打开多个插件面板。查看节点、话题最常用的就是它。

#### 5.3.1 安装与启动 RQT

RQT 在桌面完整版（`ros-humble-desktop`）里通常已经装好；如果启动时报找不到命令，或者 Plugins 菜单里插件不全，可以单独安装所有 rqt 插件：

```bash
sudo apt install ros-humble-rqt*  # 安装/补齐所有 rqt 插件（* 是通配，装完整套）
```

启动方式有两种，效果一样，取其一即可：

```bash
rqt                             # 打开 RQT 主界面，之后从菜单里选插件
ros2 run rqt_gui rqt_gui        # 等价写法：用 ros2 run 启动 RQT 主程序
rqt_graph                       # 直接以独立窗口打开「节点-话题」关系图（最常用）
```

- 第一次打开 RQT 主界面时窗口是空的，需要自己加载插件：点击菜单栏的 **Plugins**，里面按类别列出了所有可用插件（具体有哪些取决于装了哪些 `rqt_*` 包）。
- 如果 Plugins 菜单里空空如也，先关掉 RQT，用 `rqt --force-discover` 重新启动，让它重新扫描一遍插件。
- 每个插件都可以脱离主界面单独运行，格式是 `ros2 run <插件包名> <插件名>`，例如 `ros2 run rqt_graph rqt_graph`、`ros2 run rqt_top rqt_top`。
- 想确认系统里有哪些 rqt 插件，可以用 `ros2 pkg list`，然后找以 `rqt_` 开头的包。

#### 5.3.2 看节点和话题的关系：Node Graph（rqt_graph）

**Plugins → Introspection → Node Graph**，或者直接命令行 `rqt_graph`。

这是最直观的一个：把当前系统里所有节点和话题画成一张关系图。

- **圆形图标**代表节点，**矩形图标**代表话题，两者之间的箭头就是「谁发布、谁订阅」的关系。
- 鼠标悬停在某个节点或话题上，相关的连线会高亮，方便在复杂系统里顺着数据流往下看。
- 顶部有几个开关用来过滤画面：是否显示所有话题、是否只显示有活跃连接的、按命名空间过滤、是否隐藏调试类节点等。
- 配合命令行交叉验证：图上看到的节点名，可以用 `ros2 node info <节点名>` 查看它具体发布了哪些话题、订阅了哪些话题、提供了哪些服务。
- 注意：`rqt_graph` 显示的是**当前正在运行的**节点和话题，节点没启动或者话题上还没有数据时，图上是空的。

#### 5.3.3 看话题的详细数据：Topic Monitor（rqt_topic）

**Plugins → Topics → Topic Monitor**，或者 `ros2 run rqt_topic rqt_topic`。

它相当于图形版的 `ros2 topic echo` + `ros2 topic hz` + `ros2 topic bw`，一个界面里把这些信息都列出来：

- 勾选要监控的话题后，会以表格形式显示每条话题的：**类型**、**当前值 / 最新一条消息内容**、**发布频率**、**占用带宽**。
- 相比命令行 `echo` 刷屏，表格形式更适合同时盯多个话题、对比它们的频率是否正常。
- 想让它统计「频率、带宽」这类数据，话题上必须**确实有数据在流动**，否则这几列是空的。

#### 5.3.4 在 RQT 里加载插件的完整流程

以查话题为例，把「打开界面 → 加载插件 → 使用」串一遍：

1. 终端执行 `rqt`，出现主界面。
2. 菜单栏 **Plugins** → 按类别展开（如 **Topics**、**Services**、**Introspection**）→ 点选某个插件。
3. 插件会作为一个面板出现在窗口里；同时开多个插件时，它们以标签页（Tab）或分栏形式排列，可以拖动边框调整大小。
4. 如果某个插件是灰的 / 不可选，说明对应的 `rqt_*` 包没装，按 5.3.1 的命令补装后重启 RQT。
5. 想关掉某个面板：菜单栏 **Plugins** → 再点一次该插件名取消勾选；或直接关掉它所在的分栏。

常用插件与对应命令行的关系见 5.3.5 的表格。

#### 5.3.5 其它常用插件

| 插件 | 菜单路径 | 作用 |
| --- | --- | --- |
| Node Graph | Introspection → Node Graph | 节点 / 话题关系图（等同于 `rqt_graph`） |
| Topic Monitor | Topics → Topic Monitor | 话题的类型、频率、带宽、内容（等同于 `ros2 topic echo/hz/bw`） |
| Topic Publisher | Topics → Topic Publisher | 图形化手动发布消息（等同于 `ros2 topic pub`） |
| Message Type Browser | Topics → Message Type Browser | 浏览系统里所有消息类型（等同于 `ros2 interface show`） |
| Service Caller | Services → Service Caller | 图形化调用服务（等同于 `ros2 service call`） |
| Node Monitor（rqt_top） | —（独立运行） | 各节点进程的 PID、CPU、内存占用（等同于系统任务管理器） |

#### 5.3.6 与命令行工具的分工

两者是互补的，不是替代关系：

- **图形化（RQT）**：适合「先看清楚系统长什么样」——节点怎么连、谁在发谁在收、数据频率是否正常。排查「话题名字写错了导致连不上」这类问题，看 `rqt_graph` 比敲命令快得多。
- **命令行（`ros2 node` / `ros2 topic`）**：适合精确查询和写进脚本，输出是文本，可以复制、可以 grep、可以自动化。具体子命令见 3.2 与 5.4。

### 5.4 常用命令行

ros2 的命令都遵循「`ros2 <子命令> <关键字> [参数]`」的格式。任何一条命令加上 `-h` 或 `--help` 就会打印它的帮助文档，列出可用子命令和参数——**记不住时先用 `-h`**，比背命令快。

```bash
ros2 -h                          # 顶层帮助：列出所有子命令
ros2 topic -h                    # 查看 topic 子命令下又有哪些子命令
ros2 topic echo -h               # 再往下钻，查看 echo 的具体参数
```

#### 5.4.1 查找类：ros2 pkg

```bash
ros2 pkg executables [包名]      # 输出所有功能包或指定功能包下的可执行程序
ros2 pkg list                    # 列出所有功能包，包括自己写的和系统自带的
ros2 pkg prefix [包名]           # 列出功能包路径
ros2 pkg xml [包名]              # 输出功能包的 package.xml 内容
```

#### 5.4.2 节点：ros2 node

```bash
ros2 node list                   # 列出当前运行的所有节点
ros2 node info <节点名>          # 查看指定节点的详细信息（发布/订阅的话题、提供的服务、动作、参数等）
```

#### 5.4.3 接口：ros2 interface

```bash
ros2 interface list              # 列出系统里所有可用的接口类型（消息 / 服务 / 动作）
ros2 interface show <接口类型>   # 查看指定接口的具体定义，如 ros2 interface show base_interfaces_demo/msg/Student
ros2 interface proto <接口类型>  # 打印该接口的「原型」，即一份可直接填写的空模板
```

#### 5.4.4 话题：ros2 topic

```bash
ros2 topic list                  # 列出当前所有话题
ros2 topic list -t               # 列出话题的同时显示各自的类型
ros2 topic info <话题名>         # 查看话题的类型以及发布方 / 订阅方数量
ros2 topic info <话题名> -v      # 详细模式：列出具体是哪些节点在发布 / 订阅
ros2 topic echo <话题名>         # 实时打印话题上流动的消息内容
ros2 topic pub <话题名> <类型> <消息内容>          # 手动向话题发布一条消息
ros2 topic pub --once <话题名> <类型> <消息内容>   # 只发布一次便退出
ros2 topic hz <话题名>           # 统计话题的发布频率（每秒多少条）
ros2 topic bw <话题名>           # 统计话题占用的带宽
ros2 topic find <类型>           # 按消息类型反查有哪些话题在用
```

其中 `pub` 的默认行为是**按固定频率持续发布**（默认 1 Hz），加 `--once` 才是只发一条。

#### 5.4.5 服务：ros2 service

```bash
ros2 service list                # 列出当前所有服务
ros2 service list -t             # 列出服务的同时显示各自的类型
ros2 service info <服务名>       # 查看服务的类型
ros2 service type <服务名>       # 只查看服务的类型
ros2 service find <类型>         # 按服务类型反查有哪些服务在用
ros2 service call <服务名> <类型> <请求内容>   # 手动调用一次服务
```

#### 5.4.6 动作：ros2 action

```bash
ros2 action list                 # 列出当前所有动作
ros2 action list -t              # 列出动作的同时显示各自的类型
ros2 action info <动作名>        # 查看动作的详细信息（服务端 / 客户端数量、底层接口）
ros2 action send_goal <动作名> <类型> <目标内容>   # 手动发送一个动作目标
```

#### 5.4.7 参数：ros2 param

```bash
ros2 param list                  # 列出系统里所有节点声明的参数
ros2 param list <节点名>         # 只列指定节点的参数
ros2 param get <节点名> <参数名> # 读取某个参数的值
ros2 param set <节点名> <参数名> <值>  # 修改某个参数的值
ros2 param describe <节点名> <参数名>  # 查看参数的描述（类型、取值范围等）
ros2 param dump <节点名>         # 把某节点的所有参数导出成 YAML 文本
ros2 param load <节点名> <YAML文件>    # 从 YAML 文件把参数加载进节点
```

`param` 是对 3.5 那组参数服务的命令行封装：`get` / `set` / `describe` 分别对应 `get_parameters` / `set_parameters` / `describe_parameters` 服务。

> 各通信方式（话题 / 服务 / 动作 / 参数）自身最常用的命令，在 3.2 ~ 3.5 各节的「常用命令」小节里已有针对性说明；本节按命令组做了汇总，便于整体查阅。

## 六、应用方向

## 七、技术支持与资源

## 附录 A：示例工程

### A.1 目录结构

```text
ros2_learning_plumbing/
└── src/
    ├── base_interfaces_demo/          # 接口包（ament_cmake，只放 .msg/.srv/.action）
    │   ├── msg/Student.msg
    │   ├── srv/AddInts.srv
    │   ├── action/Progress.action
    │   ├── package.xml
    │   └── CMakeLists.txt
    ├── topic_cpp/                     # 话题通信
    │   ├── src/demo01_talker_str.cpp
    │   ├── src/demo02_listener_str.cpp
    │   ├── src/demo03_talker_student.cpp
    │   ├── src/demo04_listener_student.cpp
    │   ├── package.xml
    │   └── CMakeLists.txt
    ├── service_cpp/                   # 服务通信
    │   ├── src/demo01_server.cpp
    │   ├── src/demo02_client.cpp
    │   ├── package.xml
    │   └── CMakeLists.txt
    └── action_cpp/                    # 动作通信
        ├── src/demo01_action_server.cpp
        ├── src/demo02_action_client.cpp
        ├── package.xml
        └── CMakeLists.txt
```

### A.2 功能包与可执行程序清单

| 功能包 | 构建类型 | 可执行程序 | 对应正文 |
| --- | --- | --- | --- |
| `base_interfaces_demo` | ament_cmake | 无（纯接口包） | 3.6 |
| `topic_cpp` | ament_cmake | `demo01_talker_str`（发布内置 String） | 3.2 案例一 |
| `topic_cpp` | ament_cmake | `demo02_listener_str`（订阅内置 String） | 3.2 案例一 |
| `topic_cpp` | ament_cmake | `demo03_talker_student`（发布自定义 Student） | 3.2 案例二 |
| `topic_cpp` | ament_cmake | `demo04_listener_student`（订阅自定义 Student） | 3.2 案例二 |
| `service_cpp` | ament_cmake | `demo01_server`（AddInts 服务端） | 3.3 |
| `service_cpp` | ament_cmake | `demo02_client`（AddInts 客户端） | 3.3 |
| `action_cpp` | ament_cmake | `demo01_action_server`（get_sum 服务端） | 3.4 |
| `action_cpp` | ament_cmake | `demo02_action_client`（get_sum 客户端） | 3.4 |

### A.3 话题名 / 服务名 / 动作名一览

| 通信 | 名字 | 接口类型 | 节点名 |
| --- | --- | --- | --- |
| 话题 | `topic` | `std_msgs/msg/String` | `minimal_publisher` / `minimal_subscriber` |
| 话题 | `topic_stu` | `base_interfaces_demo/msg/Student` | `student_publisher` / `student_subscriber` |
| 服务 | `add_ints` | `base_interfaces_demo/srv/AddInts` | `minimal_service` / `minimal_client` |
| 动作 | `get_sum` | `base_interfaces_demo/action/Progress` | `minimal_action_server` / `minimal_action_client` |

### A.4 接口文件内容

`msg/Student.msg`（学生信息）：

```text
string name # 学生姓名，使用string，而不是String，否则会被当成ROS2内置的消息类型
int32 age # 学生年龄
float64 height # 学生身高
```

`srv/AddInts.srv`（两数相加）：

```text
# 请求部分
int32 num1
int32 num2
---
# 响应部分
int32 sum
```

`action/Progress.action`（累加求和 + 进度反馈）：

```text
#目标字段：客户端发送的请求
int64 num
---
# 结果字段：任务完成后返回的最终结果
int64 sum
---
# 任务执行过程中周期性发送的中间状态
float64 progress
```

### A.5 编译与运行

在工作空间根目录（即 `ros2_learning_plumbing/`）执行：

```bash
colcon build                # 编译全部功能包
. install/setup.bash        # 让当前终端能找到刚编译出来的包

# 话题：开两个终端分别运行
ros2 run topic_cpp demo01_talker_str
ros2 run topic_cpp demo02_listener_str

# 服务：先起服务端，再起客户端（客户端需要两个整数参数）
ros2 run service_cpp demo01_server
ros2 run service_cpp demo02_client 3 5

# 动作：先起服务端，再起客户端
ros2 run action_cpp demo01_action_server
ros2 run action_cpp demo02_action_client
```

### A.6 代码写法要点

- **节点类**：三个包里的 C++ 代码统一写成自定义节点类（`class Xxx : public rclcpp::Node`），`main()` 里依次 `rclcpp::init()` → `rclcpp::spin()` → `rclcpp::shutdown()`，与 2.2 的编码规范一致。
- **C++ 包的 CMakeLists.txt**：每条 `add_executable()` 紧跟一条 `ament_target_dependencies()` 声明它用到的依赖，最后统一 `install(TARGETS ...)` 到 `lib/${PROJECT_NAME}`（写法见 2.3.3）。
- **接口包的 CMakeLists.txt**：只有 `find_package(rosidl_default_generators REQUIRED)` 和 `rosidl_generate_interfaces()`，不出可执行文件（见 3.6.4）。
- **动作服务端的线程处理**：`handle_accepted()` 里另开子线程执行 `execute()`，线程句柄存进 `worker_threads_`，析构函数里 `join()` 回收；代码注释明确写了**不要用 `detach()`**，否则节点销毁时子线程可能访问已析构的服务端导致崩溃。
- **接口头文件的包含路径**：全部小写，如 `base_interfaces_demo/msg/student.hpp`、`.../srv/add_ints.hpp`、`.../action/progress.hpp`。

## 附录 B：ROS2 接口速查

本附录只收录 **ROS2 自己提供的接口**（`rclcpp` / `rclcpp_action` 的类和函数），不涉及 C++ 语言本身的语法与标准库设施。按用途分类，示例列取自示例工程。

### B.1 生命周期与执行

| 接口 | 参数（含义 / 要传什么） | 作用 |
| --- | --- | --- |
| `rclcpp::init()` | `argc`（整数）、`argv`（字符串数组）：`main` 的命令行参数，原样传入 | 初始化 ROS2 通信环境，必须最先调用 |
| `rclcpp::spin(节点指针)` | `节点指针`（节点对象的共享指针）：要托管的节点对象 | 进入事件循环，阻塞处理各类回调，Ctrl+C 才退出 |
| `rclcpp::spin_until_future_complete(节点, future)` | `节点`（节点对象的共享指针）：等结果期间处理回调的节点；`future`：要等待的异步结果（如 `share()` 出来的） | 一边处理回调一边等 future 完成，用于服务 / 动作的「等结果」 |
| `rclcpp::shutdown()` | 无参数 | 关闭通信、释放资源，程序结束前调用 |
| `rclcpp::ok()` | 无参数，返回 `bool` | 判断程序是否仍应继续运行 |
| `rclcpp::Node` | 无参数（这是类，构造时传节点名，如 `Node("节点名")`） | 所有节点类的父类，自定义节点必须继承它 |
| `rclcpp::Node::make_shared(节点名)` | `节点名`（字符串）：节点名称，如 `"node_demo"` | 不写自定义类时，直接创建一个节点对象 |
| `rclcpp::Rate(频率)` + `.sleep()` | `频率`（浮点）：循环频率，如 `10.0` 表示 10 Hz；`.sleep()` 无参数 | 控制循环频率（如 10 Hz） |

### B.2 消息类型与日志

| 接口 | 参数（含义 / 要传什么） | 作用 |
| --- | --- | --- |
| `std_msgs/msg/String` | 无参数（这是消息类型）；其字段 `data`（`string`）存放文本内容 | 常用内置消息类型，内容放在 `data` 字段 |
| `base_interfaces_demo/msg/Student` | 无参数（这是消息类型）；字段见 `Student.msg` 定义 | 示例工程的自定义消息类型，编译接口包时自动生成 |
| `this->get_logger()` | 无参数 | 取当前节点的日志器，作为日志宏的第一个参数 |
| `RCLCPP_INFO` / `RCLCPP_WARN` / `RCLCPP_ERROR` | `日志器`（日志器对象）：传 `this->get_logger()`；`"格式"`（字符串）：`printf` 风格格式串；`参数...`：按格式串依次填入的值 | 打印常规 / 告警 / 错误日志，格式串沿用 `printf` 风格 |

日志格式占位符：`%s` 字符串、`%d` 整数、`%ld` 长整数（对应 `int64`）、`%.2f` 保留两位小数。

### B.3 话题通信

| 接口 | 参数（含义 / 要传什么） | 作用 |
| --- | --- | --- |
| `create_publisher<消息类型>(话题名, 队列长度)` | `话题名`（字符串）：话题名称，如 `"topic"`；`队列长度`（整数）：QoS 队列深度，如 `10` | 创建发布方 |
| `publish(消息)` | `消息`（消息类型对象，只读引用）：要发布的消息对象 | 把消息发到话题上 |
| `create_subscription<消息类型>(话题名, 队列长度, 回调)` | `话题名`（字符串）：话题名称；`队列长度`（整数）：QoS 队列深度；`回调`：绑定了节点对象的成员函数 | 创建订阅方并绑定回调 |
| `create_wall_timer(周期, 回调)` | `周期`（时长）：触发间隔，如 `500ms`；`回调`：无入参的成员函数 | 创建定时器，按固定周期触发回调 |
| `rclcpp::Publisher<消息类型>::SharedPtr` | 无参数（这是类型，不是函数） | 发布方对象句柄（成员变量声明用） |
| `rclcpp::Subscription<消息类型>::SharedPtr` | 无参数（这是类型，不是函数） | 订阅方对象句柄 |
| `rclcpp::TimerBase::SharedPtr` | 无参数（这是类型，不是函数） | 定时器对象句柄 |

订阅回调的参数是「收到的消息」，用 `const` 修饰表示回调里不该修改它。

### B.4 服务通信

| 接口 | 参数（含义 / 要传什么） | 作用 |
| --- | --- | --- |
| `create_service<接口类型>(服务名, 回调)` | `服务名`（字符串）：服务名称，如 `"add_ints"`；`回调`：（请求、响应）两个入参的成员函数 | 创建服务端并绑定回调 |
| `create_client<接口类型>(服务名)` | `服务名`（字符串）：要连接的服务名 | 创建客户端 |
| `client->wait_for_service(超时)` | `超时`（时长）：最长等待时长，如 `1s` | 等待服务端上线 |
| `client->async_send_request(请求)` | `请求`（请求对象的共享指针）：要发送的请求对象 | 异步发送请求，立刻返回 |
| `rclcpp::FutureReturnCode::SUCCESS` | 无参数（枚举值，与返回码比较） | 判断 future 是否成功完成 |
| `<接口类型>::Request` / `<接口类型>::Response` | 无参数（这是类型，不是函数） | 请求类 / 响应类，由接口包编译生成 |
| `rclcpp::Service<接口类型>::SharedPtr` | 无参数（这是类型，不是函数） | 服务端对象句柄 |
| `rclcpp::Client<接口类型>::SharedPtr` | 无参数（这是类型，不是函数） | 客户端对象句柄 |

服务回调的两个入参分别是请求和响应：请求只读，响应可写（往字段里赋值即完成响应）。

### B.5 动作通信

**服务端**（`rclcpp_action`）：

| 接口 | 参数（含义 / 要传什么） | 作用 |
| --- | --- | --- |
| `rclcpp_action::create_server<接口类型>(节点, 动作名, 处理目标, 处理取消, 接受后执行)` | `节点`（节点对象的共享指针）：承载服务端的节点；`动作名`（字符串）：如 `"get_sum"`；三个回调：处理目标 / 处理取消 / 接受后执行 | 创建动作服务端，要提供三个回调 |
| `rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE` / `::REJECT` | 无参数（枚举值，作 `handle_goal` 返回值） | 处理目标回调的返回值：接受并执行 / 拒绝 |
| `rclcpp_action::CancelResponse::ACCEPT` | 无参数（枚举值，作 `handle_cancel` 返回值） | 处理取消回调的返回值：同意取消 |
| `rclcpp_action::ServerGoalHandle<接口类型>` | 无参数（这是类型，不是函数） | 单个目标的句柄，用它反馈进度、结束任务 |
| `目标句柄->get_goal()` | 无参数，返回目标对象指针 | 取出客户端发来的目标 |
| `目标句柄->publish_feedback(反馈对象)` | `反馈对象`（反馈对象的共享指针）：填好的进度对象 | 发布一次连续反馈 |
| `目标句柄->is_canceling()` | 无参数，返回 `bool` | 查询是否被请求取消 |
| `目标句柄->succeed(结果对象)` | `结果对象`（结果对象的共享指针）：填好的结果对象 | 任务正常完成并返回结果 |
| `目标句柄->canceled(结果对象)` / `abort(结果对象)` | `结果对象`（结果对象的共享指针）：当前结果对象 | 任务被取消 / 被中止并返回结果 |
| `rclcpp_action::Server<接口类型>::SharedPtr` | 无参数（这是类型，不是函数） | 动作服务端对象句柄 |

**客户端**（`rclcpp_action`）：

| 接口 | 参数（含义 / 要传什么） | 作用 |
| --- | --- | --- |
| `rclcpp_action::create_client<接口类型>(节点, 动作名)` | `节点`（节点对象的共享指针）：承载客户端的节点；`动作名`（字符串）：要连接的动作名 | 创建动作客户端 |
| `client->wait_for_action_server(超时)` | `超时`（时长）：最长等待时长，如 `10s` | 等待动作服务端上线 |
| `rclcpp_action::Client<接口类型>::SendGoalOptions` | 无参数（这是结构体类型，往三个 `callback` 成员里填回调） | 承载三个回调的结构体，发目标前先填好 |
| `send_goal_options.goal_response_callback` | 赋值为回调函数，入参是目标句柄的 future（为空表示被拒绝） | 服务端回应「接受 / 拒绝」时的回调 |
| `send_goal_options.feedback_callback` | 赋值为回调函数，入参是（目标句柄、反馈对象） | 每收到一次连续反馈时的回调 |
| `send_goal_options.result_callback` | 赋值为回调函数，入参是结果包装（含 `code` 与 `result`） | 任务结束时的回调 |
| `client->async_send_goal(目标对象, 选项)` | `目标对象`（目标对象的共享指针）：提交的目标；`选项`（`SendGoalOptions`）：三个回调的结构体 | 异步发送目标，结果全交给回调处理 |
| `rclcpp_action::ResultCode::SUCCEEDED` / `ABORTED` / `CANCELED` | 无参数（枚举值，与 `result.code` 比较） | 结果里的状态码，判断任务最终怎么了 |
| `rclcpp_action::Client<接口类型>::SharedPtr` | 无参数（这是类型，不是函数） | 动作客户端对象句柄 |

### B.6 参数服务

| 接口 | 参数（含义 / 要传什么） | 所在侧 | 作用 |
| --- | --- | --- | --- |
| `declare_parameter(名字, 默认值)` | `名字`（字符串）：参数名，如 `"max_speed"`；`默认值`：初值，同时决定类型，如 `1.0` / `true` / `"abc"` | 服务端 | 声明参数（必须先声明） |
| `get_parameter(名字, 变量)` | `名字`（字符串）：要读的参数名；`变量`：接收值的变量（按类型引用） | 服务端 | 读取参数 |
| `set_parameter(Parameter(名字, 值))` | `Parameter(名字, 值)`：参数对象，如 `rclcpp::Parameter("max_speed", 2.0)` | 服务端 | 修改参数 |
| `add_on_set_parameters_callback(回调)` | `回调`（函数对象）：入参是参数列表（参数对象列表），返回 `SetParametersResult`（用 `successful` 表示是否放行） | 服务端 | 参数被改时的回调 |
| `get_parameters(名字列表, 结果)` | `名字列表`（字符串列表）：要读的参数名；`结果`（参数对象列表的引用）：接收结果 | 客户端 | 批量读参数 |
| `set_parameters({Parameter(...)})` | `{...}`（参数对象列表）：要改的参数对象列表 | 客户端 | 批量改参数 |
| `list_parameters(前缀, 深度)` | `前缀`（字符串列表）：名字过滤前缀，空表示全部；`深度`（整数）：递归深度 | 客户端 | 列出参数名 |
