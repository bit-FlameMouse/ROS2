#!/usr/bin/env python3
# 文件用途：导航链路（FR-04/FR-05）：仿真 + Nav2（AMCL + 规划 + 控制）+ RViz
#   复用 nav2_bringup 官方 launch（AS-64），地图与参数由本包提供
# 版权：2026 patrol_robot developer，Apache-2.0 许可（许可证原文见仓库根目录 LICENSE 文件）
import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    ExecuteProcess,
    IncludeLaunchDescription,
)
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    pkg_bringup = get_package_share_directory('patrol_robot_bringup')
    pkg_nav2 = get_package_share_directory('nav2_bringup')

    use_sim_time = LaunchConfiguration('use_sim_time')
    map_file = LaunchConfiguration('map')
    params_file = LaunchConfiguration('params_file')
    autostart = LaunchConfiguration('autostart')
    use_rviz = LaunchConfiguration('use_rviz')
    gui = LaunchConfiguration('gui')

    return LaunchDescription([
        DeclareLaunchArgument(
            'use_sim_time', default_value='true', description='使用仿真时钟'),
        DeclareLaunchArgument(
            'map', default_value=os.path.join(pkg_bringup, 'maps', 'tb3_world.yaml'),
            description='栅格地图 yaml 路径'),
        DeclareLaunchArgument(
            'params_file',
            default_value=os.path.join(pkg_bringup, 'config', 'nav2_params.yaml'),
            description='Nav2 参数文件路径'),
        DeclareLaunchArgument(
            'autostart', default_value='true',
            description='Nav2 生命周期节点是否自动 configure/activate'),
        DeclareLaunchArgument('use_rviz', default_value='true', description='是否启动 RViz2'),
        DeclareLaunchArgument(
            'gui', default_value='true',
            description='是否启动 Gazebo 客户端（gui:=false 为 headless 模式）'),

        # 1) 仿真世界
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(pkg_bringup, 'launch', 'simulation.launch.py')),
            launch_arguments={
                'use_sim_time': use_sim_time,
                'gui': gui,
            }.items(),
        ),

        # 2) Nav2 完整栈（map_server + amcl + planner + controller + bt_navigator）
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(pkg_nav2, 'launch', 'bringup_launch.py')),
            launch_arguments={
                'use_sim_time': use_sim_time,
                'map': map_file,
                'params_file': params_file,
                'autostart': autostart,
            }.items(),
        ),

        # 3) RViz2（导航与航点观察）
        ExecuteProcess(
            cmd=['rviz2', '-d', os.path.join(pkg_bringup, 'rviz', 'patrol.rviz')],
            condition=IfCondition(use_rviz),
            output='screen',
        ),
    ])
