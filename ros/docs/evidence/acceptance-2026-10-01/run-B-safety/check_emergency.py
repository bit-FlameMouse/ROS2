#!/usr/bin/env python3
"""读取 /safety/emergency_stop 的锁存值（TRANSIENT_LOCAL，晚加入也能拿到）。"""
import sys

import rclpy
from rclpy.node import Node
from rclpy.qos import DurabilityPolicy, QoSProfile, ReliabilityPolicy
from std_msgs.msg import Bool

got = []


class N(Node):
    def __init__(self):
        super().__init__('emergency_check')
        qos = QoSProfile(
            depth=1,
            reliability=ReliabilityPolicy.RELIABLE,
            durability=DurabilityPolicy.TRANSIENT_LOCAL,
        )
        self.create_subscription(Bool, '/safety/emergency_stop', self.cb, qos)

    def cb(self, m):
        got.append(m.data)


rclpy.init()
n = N()
for _ in range(20):
    rclpy.spin_once(n, timeout_sec=0.25)
    if got:
        break
if got:
    print("emergency_stop 锁存值: data=%s" % got[0])
else:
    print("未收到（超时 5 s）")
    sys.exit(1)
