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

---

## 一、开发环境（VSCode 插件）

开发 Python 节点，除通用插件外还需要：

- Python：官方 Python 开发插件

（Chinese、Msg Language Support、vscode-pdf、XML、YAML、URDF 等通用插件见《ROS学习.md》1.2）

## 二、节点（Python 写法）

编码规范：Node 节点必须以继承的方式进行（之前是直接实例化）。这种方式可以在一个进程内组织多个节点，对于提高通信非常有帮助。

```python
import rclpy
from rclpy.node import Node

class MyNode(Node):
    def __init__(self):
        super().__init__("node_demo")  # 初始化节点

    def get_message(self):  # 定义一个函数，输出对应的信息
        self.get_logger().info("msg: node_demo")

def main():
    rclpy.init()
    node = MyNode()
    node.get_message()  # 调用对象方法，输出信息
    rclpy.shutdown()  # 回收资源

if __name__ == "__main__":
    main()
```

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
        'helloworld = pkg_demo_py.helloworld:main',
        'talker = pkg_demo_py.talker:main',  # 新增的节点
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
ros2 pkg create pkg_demo_py --build-type ament_python --dependencies rclpy --node-name helloworld
```

**2. 编辑源文件**

包下面与包同名的目录下面的 node 名的 py 文件，这个就是主文件。编辑代码。

```python
import rclpy

def main():
    rclpy.init()

    node = rclpy.create_node("node_demo")
    node.get_logger().info("msg: node_demo")

    rclpy.shutdown()

if __name__ == "__main__":
    main()
```

**3. 编辑配置文件**

XML 文件、setup.py 文件。

注册入口：告诉构建系统，把某个 Python 函数包装成一个可以用命令运行的可执行程序。

```python
entry_points={
    'console_scripts': [
        'helloworld = pkg_demo_py.helloworld:main',  # 列表中手动补上对应的节点
    ],
},
# ros2 run 时使用的名字 = Python包名.文件名:函数名
```

- helloworld：运行命令时用的可执行名
- pkg_demo_py：内层 Python 包目录
- helloworld：helloworld.py
- main：helloworld.py 里的 main 函数

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
ros2 run pkg_demo_py helloworld  # ros2 run <包名> <可执行名>
```

## 五、在 Python 中使用自定义接口

接口（msg / srv / action）的语法与注册方式见《ROS学习.md》3.6。使用方需要在 `package.xml` 中声明依赖：

```xml
<depend>pkg_demo_msg</depend>
```

之后在 Python 代码里导入：

```python
from pkg_demo_msg.msg import Student
```
