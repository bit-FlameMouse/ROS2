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
# 文件用途：导航链路（FR-04/FR-05）：仿真 + Nav2（AMCL + 规划 + 控制）+ RViz
#   复用 nav2_bringup 官方 launch（AS-64），地图与参数由本包提供
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

        # 1) 仿真世界
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(pkg_bringup, 'launch', 'simulation.launch.py')),
            launch_arguments={'use_sim_time': use_sim_time}.items(),
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
