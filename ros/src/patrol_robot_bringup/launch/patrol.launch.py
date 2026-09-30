#!/usr/bin/env python3
# 文件用途：一键巡逻演示链路（FR-12）：仿真 + Nav2 + 两个自研节点
#   启动顺序（AS-59）：Gazebo → robot_state_publisher → Nav2 激活 → 自研节点
#   任务层延迟 8s 启动，给 Nav2 生命周期激活留时间（K-09）
# 版权：2026 patrol_robot developer，Apache-2.0 许可（许可证原文见仓库根目录 LICENSE 文件）
import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    IncludeLaunchDescription,
    TimerAction,
)
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    pkg_bringup = get_package_share_directory('patrol_robot_bringup')

    use_sim_time = LaunchConfiguration('use_sim_time')
    map_file = LaunchConfiguration('map')
    nav2_params = LaunchConfiguration('params_file')
    patrol_params = LaunchConfiguration('patrol_params')
    auto_start = LaunchConfiguration('auto_start')
    use_rviz = LaunchConfiguration('use_rviz')
    gui = LaunchConfiguration('gui')
    task_delay = LaunchConfiguration('task_delay')

    return LaunchDescription([
        DeclareLaunchArgument('use_sim_time', default_value='true'),
        DeclareLaunchArgument(
            'map', default_value=os.path.join(pkg_bringup, 'maps', 'tb3_world.yaml'),
            description='栅格地图 yaml 路径'),
        DeclareLaunchArgument(
            'params_file',
            default_value=os.path.join(pkg_bringup, 'config', 'nav2_params.yaml'),
            description='Nav2 参数文件路径'),
        DeclareLaunchArgument(
            'patrol_params',
            default_value=os.path.join(pkg_bringup, 'config', 'patrol_params.yaml'),
            description='任务层参数文件路径'),
        DeclareLaunchArgument(
            'auto_start', default_value='false',
            description='任务层启动后是否自动开始巡逻'),
        DeclareLaunchArgument('use_rviz', default_value='true', description='是否启动 RViz2'),
        DeclareLaunchArgument(
            'gui', default_value='true',
            description='是否启动 Gazebo 客户端（gui:=false 为 headless 模式）'),
        DeclareLaunchArgument(
            'task_delay', default_value='8.0',
            description='任务层延迟启动秒数（等待 Nav2 生命周期激活）'),

        # 1) 仿真 + Nav2 + RViz
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(pkg_bringup, 'launch', 'navigation.launch.py')),
            launch_arguments={
                'use_sim_time': use_sim_time,
                'map': map_file,
                'params_file': nav2_params,
                'use_rviz': use_rviz,
                'gui': gui,
            }.items(),
        ),

        # 2) 任务层：延迟启动（双保险：TimerAction + NavClient::waitForServer）
        TimerAction(
            period=task_delay,
            actions=[
                IncludeLaunchDescription(
                    PythonLaunchDescriptionSource(
                        os.path.join(pkg_bringup, 'launch', 'patrol_nodes.launch.py')),
                    launch_arguments={
                        'use_sim_time': use_sim_time,
                        'params_file': patrol_params,
                        'auto_start': auto_start,
                    }.items(),
                ),
            ],
        ),
    ])
