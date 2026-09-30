#!/usr/bin/env python3
# 文件用途：AMCL 定位精度检查（TC-I-04 辅助）——比较 /amcl_pose 与 /odom 的位姿偏差
#   说明：TB3 Gazebo 插件的里程计源为世界坐标（无漂移），可作为近似真值；
#   /amcl_pose 的发布 QoS 为 RELIABLE + TRANSIENT_LOCAL，订阅端必须同时声明
#   durability=transient_local 才能稳定收到（见 docs/17 文档 §5）。
# 前置： navigation.launch.py 已启动且 AMCL 已激活
# 用法： python3 scripts/simtest/amcl_check.py
# 版权：2026 patrol_robot developer，Apache-2.0 许可（许可证原文见仓库根目录 LICENSE 文件）
"""AMCL 定位精度检查（默认阈值 0.3 m）。"""
import math
import sys

import rclpy
from geometry_msgs.msg import PoseWithCovarianceStamped
from nav_msgs.msg import Odometry
from rclpy.node import Node
from rclpy.qos import DurabilityPolicy, QoSProfile, ReliabilityPolicy

AMCL_QOS = QoSProfile(
    depth=10,
    reliability=ReliabilityPolicy.RELIABLE,
    durability=DurabilityPolicy.TRANSIENT_LOCAL)


class Checker(Node):
    def __init__(self):
        super().__init__('amcl_check')
        self.amcl = None
        self.odom = None
        self.create_subscription(PoseWithCovarianceStamped, '/amcl_pose', self.on_amcl, AMCL_QOS)
        self.create_subscription(Odometry, '/odom', self.on_odom, 10)

    def on_amcl(self, msg):
        self.amcl = msg.pose.pose.position

    def on_odom(self, msg):
        self.odom = msg.pose.pose.position


def main():
    rclpy.init()
    node = Checker()
    count = 0
    deadline = 60.0
    while count < 20 and deadline > 0:
        rclpy.spin_once(node, timeout_sec=0.2)
        deadline -= 0.2
        if node.amcl is not None and node.odom is not None:
            d = math.hypot(node.amcl.x - node.odom.x, node.amcl.y - node.odom.y)
            if count % 5 == 0:
                print(f'  样本{count}: amcl=({node.amcl.x:.2f},{node.amcl.y:.2f}) '
                      f'odom=({node.odom.x:.2f},{node.odom.y:.2f}) 偏差={d:.3f} m')
            count += 1
    if count == 0:
        print('  [FAIL] 未收到 /amcl_pose 或 /odom')
        return 1
    dev = math.hypot(node.amcl.x - node.odom.x, node.amcl.y - node.odom.y)
    ok = dev < 0.3
    print(f'  {"[PASS]" if ok else "[FAIL]"} AMCL 位姿偏差 = {dev:.3f} m'
          f'（阈值 0.3 m，样本 {count}）')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
