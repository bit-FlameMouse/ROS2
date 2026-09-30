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
# 文件用途：启动 Gazebo 世界 + TurtleBot3 机器人（FR-01：仿真环境一键启动）
#   复用官方 launch（gzserver / gzclient / robot_state_publisher / spawn），
#   仅通过参数注入世界文件、型号与初始位姿（AS-64）。
import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    IncludeLaunchDescription,
    SetEnvironmentVariable,
)
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    pkg_tb3_gazebo = get_package_share_directory('turtlebot3_gazebo')
    pkg_gazebo_ros = get_package_share_directory('gazebo_ros')

    use_sim_time = LaunchConfiguration('use_sim_time')
    world_name = LaunchConfiguration('world_name')
    robot_model = LaunchConfiguration('robot_model')
    x_pose = LaunchConfiguration('x_pose')
    y_pose = LaunchConfiguration('y_pose')
    gui = LaunchConfiguration('gui')

    default_world = os.path.join(pkg_tb3_gazebo, 'worlds', 'turtlebot3_world.world')

    return LaunchDescription([
        DeclareLaunchArgument(
            'use_sim_time', default_value='true',
            description='使用 Gazebo 仿真时钟'),
        DeclareLaunchArgument(
            'world_name', default_value=default_world,
            description='Gazebo 世界文件路径（默认 TurtleBot3 官方 world）'),
        DeclareLaunchArgument(
            'robot_model', default_value='burger',
            description='TurtleBot3 型号（burger / waffle / waffle_pi）'),
        DeclareLaunchArgument('x_pose', default_value='-2.0', description='机器人初始 x（m）'),
        DeclareLaunchArgument('y_pose', default_value='-0.5', description='机器人初始 y（m）'),
        DeclareLaunchArgument('gui', default_value='true', description='是否启动 Gazebo 客户端'),

        # 官方 TB3 launch 在模块加载期读取这两个环境变量，必须在 Include 之前设置
        SetEnvironmentVariable('TURTLEBOT3_MODEL', robot_model),
        SetEnvironmentVariable(
            'GAZEBO_MODEL_PATH',
            os.path.join(pkg_tb3_gazebo, 'models') + ':' +
            os.environ.get('GAZEBO_MODEL_PATH', '')),

        # 1) Gazebo 服务端（世界物理与传感器）
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(pkg_gazebo_ros, 'launch', 'gzserver.launch.py')),
            launch_arguments={'world': world_name}.items(),
        ),
        # 2) Gazebo 客户端（可视化，可用 gui:=false 关闭）
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(pkg_gazebo_ros, 'launch', 'gzclient.launch.py')),
            condition=IfCondition(gui),
        ),
        # 3) robot_state_publisher（静态 TF + /joint_states 动态 TF）
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(pkg_tb3_gazebo, 'launch', 'robot_state_publisher.launch.py')),
            launch_arguments={'use_sim_time': use_sim_time}.items(),
        ),
        # 4) spawn 机器人
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(pkg_tb3_gazebo, 'launch', 'spawn_turtlebot3.launch.py')),
            launch_arguments={'x_pose': x_pose, 'y_pose': y_pose}.items(),
        ),
    ])
