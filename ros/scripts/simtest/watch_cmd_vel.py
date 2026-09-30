#!/usr/bin/env python3
# 文件用途：观测 /cmd_vel 与机器人位置（导航排障辅助）
#   用途：目标导航时确认控制器是否在输出速度、机器人是否真的移动
# 用法： python3 scripts/simtest/watch_cmd_vel.py [采样秒数，默认 30]
# 版权：2026 patrol_robot developer，Apache-2.0 许可（许可证原文见仓库根目录 LICENSE 文件）
"""实时观测 /cmd_vel 频率与 /odom 位置。"""
import sys
import time

import rclpy
from geometry_msgs.msg import Twist
from nav_msgs.msg import Odometry
from rclpy.node import Node


class Watch(Node):
    def __init__(self):
        super().__init__('watch_cmd_vel')
        self.cmd = 0
        self.last_cmd = None
        self.x = 0.0
        self.y = 0.0
        self.create_subscription(Twist, '/cmd_vel', self.on_cmd, 10)
        self.create_subscription(Odometry, '/odom', self.on_odom, 10)

    def on_cmd(self, msg):
        self.cmd += 1
        self.last_cmd = (msg.linear.x, msg.angular.z)

    def on_odom(self, msg):
        self.x = msg.pose.pose.position.x
        self.y = msg.pose.pose.position.y


def main():
    rclpy.init()
    node = Watch()
    seconds = float(sys.argv[1]) if len(sys.argv) > 1 else 30.0
    t0 = time.time()
    while time.time() - t0 < seconds:
        rclpy.spin_once(node, timeout_sec=0.1)
    dt = time.time() - t0
    if node.last_cmd is not None:
        print(f'  {dt:.0f}s 内 /cmd_vel {node.cmd} 条（{node.cmd / dt:.1f} Hz），'
              f'最新指令 lin={node.last_cmd[0]:.3f} ang={node.last_cmd[1]:.3f}')
    else:
        print(f'  {dt:.0f}s 内未收到 /cmd_vel')
    print(f'  机器人位置 = ({node.x:.2f}, {node.y:.2f})')
    node.destroy_node()
    rclpy.shutdown()
    return 0


if __name__ == '__main__':
    sys.exit(main())
