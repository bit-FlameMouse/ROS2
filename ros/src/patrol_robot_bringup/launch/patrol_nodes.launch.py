#!/usr/bin/env python3
# 文件用途：仅启动任务层两个自研节点（调试用，不含仿真与 Nav2）
#   前置：Nav2 已就绪（TC-I-06 前置条件，见 08 文档）
# 版权：2026 patrol_robot developer，Apache-2.0 许可（许可证原文见仓库根目录 LICENSE 文件）
import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    pkg_bringup = get_package_share_directory('patrol_robot_bringup')

    use_sim_time = LaunchConfiguration('use_sim_time')
    params_file = LaunchConfiguration('params_file')
    auto_start = LaunchConfiguration('auto_start')

    return LaunchDescription([
        DeclareLaunchArgument('use_sim_time', default_value='true'),
        DeclareLaunchArgument(
            'params_file',
            default_value=os.path.join(pkg_bringup, 'config', 'patrol_params.yaml'),
            description='任务层参数文件'),
        DeclareLaunchArgument(
            'auto_start', default_value='false',
            description='启动后是否自动开始巡逻'),

        # 安全守护先启动：其标志用 transient_local，晚启动的巡逻节点也能取到当前值
        Node(
            package='patrol_robot_core',
            executable='safety_guard_node',
            name='safety_guard_node',
            output='screen',
            parameters=[params_file, {'use_sim_time': use_sim_time}],
        ),
        Node(
            package='patrol_robot_core',
            executable='patrol_node',
            name='patrol_node',
            output='screen',
            parameters=[
                params_file,
                {'use_sim_time': use_sim_time, 'auto_start': auto_start},
            ],
        ),
    ])
