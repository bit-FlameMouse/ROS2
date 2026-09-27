# =====================================================================
# 案例练习（exe03）：生成目标乌龟并启动测距客户端
# 启动内容：
#   1.调用 /spawn 服务在 (8.54, 9.54) 生成一只名为 t2 的乌龟作为目标；
#   2.exe03_client：向 distance 服务发送目标点 (x, y, theta)，打印两只乌龟的距离。
# 运行方式：先运行 exe_server_launch.py，再运行
#          ros2 launch exercise_cpp exe_client_launch.py
# =====================================================================
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import ExecuteProcess


def generate_launch_description():
    # 设置目标点的坐标，以及目标点乌龟的名称
    x = 8.54
    y = 9.54
    theta = 0.0
    name = "t2"
    # 生成新的乌龟：执行命令行调用 /spawn 服务
    spawn = ExecuteProcess(
        cmd=["ros2 service call /spawn turtlesim/srv/Spawn \"{'x': "
             + str(x) + ",'y': " + str(y) + ",'theta': " + str(theta) + ",'name': '" + name + "'}\""],
        shell=True,
    )
    # 创建客户端节点：把上面设置的目标点作为命令行参数传给 exe03_client
    client = Node(package="exercise_cpp",
                  executable="exe03_client",
                  arguments=[str(x), str(y), str(theta)])
    return LaunchDescription([spawn, client])
