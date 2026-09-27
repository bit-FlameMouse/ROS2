# =====================================================================
# 案例练习的 launch 文件：一键启动"两只乌龟 + 运动模仿节点"
# 启动顺序：
#   1.启动两个 turtlesim_node（窗口2的节点放进命名空间 t2）；
#   2.给窗口2的乌龟发送动作目标，让它原地掉头 180°；
#   3.检测到掉头完成后，启动自实现的 exe01_pub_sub 节点，
#     开始订阅窗口1的乌龟位姿并控制窗口2的乌龟运动。
# 运行方式：ros2 launch exercise_cpp exe_pub_sub_launch.py
# =====================================================================
from launch import LaunchDescription  # launch 文件必须返回的"启动描述"对象
from launch_ros.actions import Node  # Node：启动一个 ROS2 节点
from launch.actions import ExecuteProcess, RegisterEventHandler  # 执行命令 / 注册事件处理器
from launch.event_handlers import OnProcessExit  # 事件处理器：某个进程退出时触发


def generate_launch_description():
    # 1.创建两个 turtlesim_node 节点
    #    没有 namespace 的是第一只乌龟（窗口1），它发布 /turtle1/pose
    t1 = Node(package="turtlesim", executable="turtlesim_node")
    #    namespace="t2" 给节点加上命名空间前缀（窗口2），
    #    它的话题变成 /t2/...，例如 /t2/turtle1/cmd_vel
    t2 = Node(package="turtlesim", executable="turtlesim_node", namespace="t2")
    # 2.让第二只乌龟掉头
    #    执行命令行动作 send_goal，向 /t2/turtle1/rotate_absolute 发送目标：
    #    theta=3.14 弧度 ≈ 180°，即原地掉头
    #    output="both" 把命令输出打印到终端；shell=True 表示用 shell 解析这条命令
    rotate = ExecuteProcess(
        cmd=[
            "ros2 action send_goal /t2/turtle1/rotate_absolute turtlesim/action/RotateAbsolute \"{'theta': 3.14}\""
        ],
        output="both",
        shell=True,
    )
    # 3.自实现的订阅发布实现（包名 exercise_cpp，可执行文件 exe01_pub_sub）
    pub_sub = Node(package="exercise_cpp", executable="exe01_pub_sub")
    # 4.乌龟掉头完毕后，开始执行步骤3
    #    OnProcessExit：当 rotate 进程退出（命令执行完）时，执行 on_exit 中的动作 pub_sub
    rotate_exit_event = RegisterEventHandler(
        event_handler=OnProcessExit(target_action=rotate, on_exit=pub_sub)
    )
    return LaunchDescription([t1, t2, rotate, rotate_exit_event])
