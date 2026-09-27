# =====================================================================
# 案例练习（exe02）：启动测距服务端
# 启动内容：
#   1.turtlesim_node：提供 /turtle1/pose，被服务端订阅以获取乌龟坐标；
#   2.exe02_server：提供 distance 服务，计算乌龟与目标点之间的直线距离。
# 运行方式：ros2 launch exercise_cpp exe_server_launch.py
# 配套：另开终端运行 exe_client_launch.py
# =====================================================================
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    # 创建 turtlesim_node 节点：服务端需要订阅它的位姿话题
    turtle = Node(package="turtlesim", executable="turtlesim_node")
    # 创建测距服务端节点：提供 distance 服务
    server = Node(package="exercise_cpp", executable="exe02_server")

    return LaunchDescription([turtle, server])
