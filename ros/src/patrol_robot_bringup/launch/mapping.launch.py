#!/usr/bin/env python3
# Copyright 2026 patrol_robot developer
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# 文件用途：建图链路（FR-03）：仿真 + SLAM Toolbox + RViz（+ 可选键盘遥控）
#   建图完成后用 nav2_map_server 的 map_saver_cli 保存地图（见 09 文档 §6.2）：
#     ros2 run nav2_map_server map_saver_cli \
#       -f src/patrol_robot_bringup/maps/tb3_world
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
    pkg_slam = get_package_share_directory('slam_toolbox')

    use_sim_time = LaunchConfiguration('use_sim_time')
    use_rviz = LaunchConfiguration('use_rviz')
    use_teleop = LaunchConfiguration('use_teleop')

    return LaunchDescription([
        DeclareLaunchArgument('use_sim_time', default_value='true'),
        DeclareLaunchArgument('use_rviz', default_value='true', description='是否启动 RViz2'),
        DeclareLaunchArgument(
            'use_teleop', default_value='false',
            description='是否随 launch 启动键盘遥控（默认否，建议另开终端运行）'),

        # 1) 仿真世界
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(pkg_bringup, 'launch', 'simulation.launch.py')),
            launch_arguments={'use_sim_time': use_sim_time}.items(),
        ),

        # 2) SLAM Toolbox（在线异步建图）
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(pkg_slam, 'launch', 'online_async_launch.py')),
            launch_arguments={
                'use_sim_time': use_sim_time,
                'slam_params_file': os.path.join(
                    pkg_bringup, 'config', 'slam_toolbox_params.yaml'),
            }.items(),
        ),

        # 3) RViz2（建图观察）
        ExecuteProcess(
            cmd=['rviz2', '-d', os.path.join(pkg_bringup, 'rviz', 'mapping.rviz')],
            condition=IfCondition(use_rviz),
            output='screen',
        ),

        # 4) 键盘遥控（可选）
        ExecuteProcess(
            cmd=['ros2', 'run', 'turtlebot3_teleop', 'teleop_keyboard'],
            condition=IfCondition(use_teleop),
            output='screen',
            emulate_tty=True,
        ),
    ])
