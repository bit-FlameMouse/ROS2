#!/usr/bin/env python3
# D2 施救：静态障碍永久 mode 4（坑 22 类）——按手册处置"人工驶离 > 0.6 m 解除"
# 步骤：1) 原地转向正对障碍柱 48°（旋转不改变间距，安全）
#       2) 倒车沿远离柱子的方向退出前向锥区（倒车途中在锥内距离持续增大，最终 > 0.60 解除）
# 保护：全程监控 360° 扫描原始最近点，< 0.17 m 立即中止
import math
import time
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy, DurabilityPolicy
from geometry_msgs.msg import Twist
from nav_msgs.msg import Odometry
from std_msgs.msg import Bool, Float32
from sensor_msgs.msg import LaserScan


def yaw_of(q):
    return math.atan2(2.0 * (q.w * q.z + q.x * q.y), 1.0 - 2.0 * (q.y * q.y + q.z * q.z))


def wrap(a):
    return math.atan2(math.sin(a), math.cos(a))


class Recover(Node):
    def __init__(self):
        super().__init__('recover_d2')
        self.pub = self.create_publisher(Twist, '/cmd_vel', 10)
        self.create_subscription(Odometry, '/odom', self.on_odom, 10)
        eq = QoSProfile(depth=1, reliability=ReliabilityPolicy.RELIABLE,
                        durability=DurabilityPolicy.TRANSIENT_LOCAL)
        self.create_subscription(Bool, '/safety/emergency_stop', self.on_emg, eq)
        self.create_subscription(Float32, '/safety/nearest_obstacle_distance', self.on_near, 10)
        self.create_subscription(LaserScan, '/scan', self.on_scan,
                                 rclpy.qos.qos_profile_sensor_data)
        self.yaw = None
        self.pos = None
        self.emg = None
        self.near = None
        self.raw_min = None

    def on_odom(self, m):
        self.pos = (m.pose.pose.position.x, m.pose.pose.position.y)
        self.yaw = yaw_of(m.pose.pose.orientation)

    def on_emg(self, m):
        self.emg = m.data

    def on_near(self, m):
        self.near = m.data

    def on_scan(self, m):
        rs = [r for r in m.ranges if math.isfinite(r) and m.range_min < r < m.range_max]
        self.raw_min = min(rs) if rs else None

    def send(self, lx, az):
        t = Twist()
        t.linear.x = lx
        t.angular.z = az
        self.pub.publish(t)


def main():
    rclpy.init()
    n = Recover()
    t0 = time.time()
    while time.time() - t0 < 10 and (n.yaw is None or n.raw_min is None):
        rclpy.spin_once(n, timeout_sec=0.2)
    if n.yaw is None or n.raw_min is None:
        print('FATAL: 缺少 /odom 或 /scan 数据')
        return
    target = wrap(n.yaw + math.radians(48.0))
    print(f'[PRE] yaw={math.degrees(n.yaw):.1f} target={math.degrees(target):.1f} '
          f'emg={n.emg} near={n.near} raw_min={n.raw_min:.3f} pos=({n.pos[0]:.3f},{n.pos[1]:.3f})')

    # 相位 R：原地转向（旋转不改变各障碍距离，安全）
    t0 = time.time()
    ok = False
    while time.time() - t0 < 8.0:
        rclpy.spin_once(n, timeout_sec=0.1)
        err = wrap(target - n.yaw)
        if abs(err) < 0.05:
            ok = True
            break
        n.send(0.0, max(-0.45, min(0.45, 1.2 * err)))
    n.send(0.0, 0.0)
    for _ in range(5):
        rclpy.spin_once(n, timeout_sec=0.1)
    print(f'[R] ok={ok} yaw={math.degrees(n.yaw):.1f} near={n.near} raw_min={n.raw_min:.3f}')

    # 相位 B：倒车退出（倒车方向与"指向柱子的方向"相反，锥内距离单调增大）
    p0 = n.pos
    t0 = time.time()
    cleared_at = None
    while time.time() - t0 < 14.0:
        rclpy.spin_once(n, timeout_sec=0.1)
        d = math.hypot(n.pos[0] - p0[0], n.pos[1] - p0[1])
        if n.raw_min is not None and n.raw_min < 0.17:
            print(f'[B] ABORT raw_min={n.raw_min:.3f}')
            break
        if cleared_at is None and ((n.emg is False) or (n.near is not None and n.near > 0.62)):
            cleared_at = d
            print(f'[B] CLEARED travel={d:.3f} near={n.near} raw_min={n.raw_min:.3f}')
        if d >= 0.55:
            break
        n.send(-0.075, 0.0)
    n.send(0.0, 0.0)
    t0 = time.time()
    while time.time() - t0 < 1.5:
        n.send(0.0, 0.0)
        rclpy.spin_once(n, timeout_sec=0.1)
    d = math.hypot(n.pos[0] - p0[0], n.pos[1] - p0[1])
    print(f'[END] travel={d:.3f} yaw={math.degrees(n.yaw):.1f} emg={n.emg} '
          f'near={n.near} raw_min={n.raw_min:.3f} pos=({n.pos[0]:.3f},{n.pos[1]:.3f})')
    n.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
