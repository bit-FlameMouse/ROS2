# =====================================================================
# 案例练习（exe04）：启动导航动作服务端
# 启动内容：
#   1.turtlesim_node：被控对象，服务端订阅它的位姿、发布它的速度指令；
#   2.exe04_action_server：提供 nav 动作，控制乌龟向目标点运动，
#     并连续反馈乌龟与目标点之间的剩余距离。
# 运行方式：ros2 launch exercise_cpp exe_action_server_launch.py
# 配套：另开终端运行 exe_action_client_launch.py
# =====================================================================
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    # 创建 turtlesim_node 节点
    turtle = Node(package="turtlesim", executable="turtlesim_node")
    # 创建动作服务端节点：提供 nav 动作
    server = Node(package="exercise_cpp", executable="exe04_action_server")

    return LaunchDescription([turtle, server])
