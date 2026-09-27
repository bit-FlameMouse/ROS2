# =====================================================================
# 案例练习（exe06）：动态修改 turtlesim 的背景颜色
# 启动内容：
#   1.turtlesim_node：参数服务端，参数 background_r 控制背景红色分量；
#   2.exe06_param：参数客户端，循环修改 background_r，实现背景渐变效果。
# 运行方式：ros2 launch exercise_cpp exe_param.py
# =====================================================================
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    # 创建 turtlesim_node 节点：它提供 background_r 等参数
    turtle = Node(package="turtlesim", executable="turtlesim_node")
    # 创建背景色修改节点（参数客户端）
    param = Node(package="exercise_cpp", executable="exe06_param")

    return LaunchDescription([turtle, param])
