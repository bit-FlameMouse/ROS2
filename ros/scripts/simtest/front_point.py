#!/usr/bin/env python3
# 文件用途：读取机器人当前位姿并计算"正前方 N 米"的坐标（TC-S-04 插障碍物辅助）
#   用途：headless 环境中无法用 Gazebo 界面 Insert 时，先得到障碍物应插入的坐标，
#         再用官方 spawn_entity.py 插入（见 docs/17 文档 §9）
# 用法： python3 scripts/simtest/front_point.py [前方距离，默认 0.25]
# 前置： 仿真已启动（/odom 可用）
# 版权：2026 patrol_robot developer，Apache-2.0 许可（许可证原文见仓库根目录 LICENSE 文件）
"""打印机器人位姿与正前方指定距离处的世界坐标。"""
import math
import sys

import rclpy
from nav_msgs.msg import Odometry
from rclpy.node import Node


class PoseReader(Node):
    def __init__(self):
        super().__init__('front_point')
        self.x = self.y = self.yaw = None
        self.create_subscription(Odometry, '/odom', self.on_odom, 10)

    def on_odom(self, msg):
        q = msg.pose.pose.orientation
        self.x = msg.pose.pose.position.x
        self.y = msg.pose.pose.position.y
        self.yaw = math.atan2(2.0 * (q.w * q.z + q.x * q.y),
                              1.0 - 2.0 * (q.y * q.y + q.z * q.z))


def main():
    rclpy.init()
    node = PoseReader()
    while node.x is None and rclpy.ok():
        rclpy.spin_once(node, timeout_sec=0.2)
    distance = float(sys.argv[1]) if len(sys.argv) > 1 else 0.25
    fx = node.x + distance * math.cos(node.yaw)
    fy = node.y + distance * math.sin(node.yaw)
    print(f'机器人位姿: x={node.x:.2f} y={node.y:.2f} yaw={math.degrees(node.yaw):.1f}°')
    print(f'正前方 {distance:.2f} m 处坐标: x={fx:.2f} y={fy:.2f}')
    print('插入障碍物示例（复制执行，注意替换坐标）：')
    print(f'  ros2 run gazebo_ros spawn_entity.py -entity safety_box \\')
    print(f'    -file /tmp/safety_box.sdf -x {fx:.2f} -y {fy:.2f} -z 0.2')
    node.destroy_node()
    rclpy.shutdown()
    return 0


if __name__ == '__main__':
    sys.exit(main())
