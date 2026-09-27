# =====================================================================
# 案例练习（exe05）：生成目标乌龟并启动导航动作客户端
# 启动内容：
#   1.调用 /spawn 服务在 (8.54, 9.54) 生成一只名为 t3 的乌龟作为目标；
#   2.exe05_action_client：向 nav 动作发送目标点，
#     接收连续反馈（剩余距离）和最终结果（乌龟坐标与航向）。
# 运行方式：先运行 exe_action_server_launch.py，再运行
#          ros2 launch exercise_cpp exe_action_client_launch.py
# =====================================================================
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import ExecuteProcess


def generate_launch_description():
    # 设置目标点的坐标，以及目标点乌龟的名称
    x = 8.54
    y = 9.54
    theta = 0.0
    name = "t3"
    # 生成新的乌龟：执行命令行调用 /spawn 服务
    spawn = ExecuteProcess(
        cmd=["ros2 service call /spawn turtlesim/srv/Spawn \"{'x': "
             + str(x) + ",'y': " + str(y) + ",'theta': " + str(theta) + ",'name': '" + name + "'}\""],
        shell=True,
    )
    # 创建动作客户端节点：把上面设置的目标点作为命令行参数传给 exe05_action_client
    client = Node(package="exercise_cpp",
                  executable="exe05_action_client",
                  arguments=[str(x), str(y), str(theta)])
    return LaunchDescription([spawn, client])
