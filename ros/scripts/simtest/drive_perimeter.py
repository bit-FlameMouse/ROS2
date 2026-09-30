#!/usr/bin/env python3
# 文件用途：建图巡线驱动（TC-I-03 辅助）——闭环驱动机器人绕外环走一圈，供 SLAM 采集
#   依赖：/odom（TB3 Gazebo 插件以世界坐标为里程计源，无累积漂移）、/cmd_vel
#   路径：先原地旋转 360°，再沿半径 2.1 m 圆环走 12 段（每段 30°），最后回到起点闭合回环
# 用法： python3 scripts/simtest/drive_perimeter.py
# 版权：2026 patrol_robot developer，Apache-2.0 许可（许可证原文见仓库根目录 LICENSE 文件）
"""建图巡线：沿六边形房间外环（半径 2.1 m）闭环行进一圈。"""
import math
import sys
import time

import rclpy
from geometry_msgs.msg import Twist
from nav_msgs.msg import Odometry
from rclpy.node import Node


def yaw_from_quat(q):
    return math.atan2(2.0 * (q.w * q.z + q.x * q.y),
                      1.0 - 2.0 * (q.y * q.y + q.z * q.z))


class Driver(Node):
    def __init__(self):
        super().__init__('perimeter_driver')
        self.pub = self.create_publisher(Twist, '/cmd_vel', 10)
        self.create_subscription(Odometry, '/odom', self.on_odom, 10)
        self.x = 0.0
        self.y = 0.0
        self.yaw = 0.0
        self.have_odom = False

    def on_odom(self, msg):
        p = msg.pose.pose.position
        self.x = p.x
        self.y = p.y
        self.yaw = yaw_from_quat(msg.pose.pose.orientation)
        self.have_odom = True

    def spin_for(self, seconds):
        end = time.time() + seconds
        while time.time() < end:
            rclpy.spin_once(self, timeout_sec=0.02)

    def stop(self):
        self.pub.publish(Twist())
        self.spin_for(0.3)

    def rotate(self, delta_yaw, timeout=25.0):
        """旋转指定角度（rad，带符号）。"""
        target = self.yaw + delta_yaw
        start = time.time()
        while time.time() - start < timeout:
            rclpy.spin_once(self, timeout_sec=0.02)
            err = math.atan2(math.sin(target - self.yaw), math.cos(target - self.yaw))
            if abs(err) < 0.08:
                self.stop()
                return True
            cmd = Twist()
            cmd.angular.z = max(-0.7, min(0.7, 1.4 * err))
            self.pub.publish(cmd)
        self.stop()
        return False

    def drive_to(self, tx, ty, timeout=40.0, tol=0.12):
        """驱车到目标点（比例控制，正前方偏差大时先原地转向）。"""
        start = time.time()
        while time.time() - start < timeout:
            rclpy.spin_once(self, timeout_sec=0.02)
            dx, dy = tx - self.x, ty - self.y
            dist = math.hypot(dx, dy)
            if dist < tol:
                self.stop()
                return True
            heading = math.atan2(dy, dx)
            err = math.atan2(math.sin(heading - self.yaw), math.cos(heading - self.yaw))
            cmd = Twist()
            if abs(err) > 1.2:
                cmd.angular.z = max(-0.7, min(0.7, 1.4 * err))
            else:
                cmd.linear.x = max(0.04, min(0.18, 0.8 * dist))
                cmd.angular.z = max(-0.7, min(0.7, 2.0 * err))
            self.pub.publish(cmd)
        self.stop()
        return False


def main():
    rclpy.init()
    d = Driver()
    for _ in range(150):
        rclpy.spin_once(d, timeout_sec=0.1)
        if d.have_odom:
            break
    if not d.have_odom:
        print('未收到 /odom，退出（请确认仿真已启动）', file=sys.stderr)
        return 1

    print(f'起始位姿: x={d.x:.2f} y={d.y:.2f} yaw={d.yaw:.2f}', flush=True)
    print('阶段 1：原地旋转 360°', flush=True)
    d.rotate(2.0 * math.pi, timeout=30.0)
    time.sleep(1.0)

    radius = 2.1
    theta0 = math.atan2(d.y, d.x)
    print(f'阶段 2：沿半径 {radius} m 圆环行进（起始角 {math.degrees(theta0):.0f}°）', flush=True)
    ok_count = 0
    for k in range(13):  # 12 段 + 回到起点
        theta = theta0 + k * (2.0 * math.pi / 12.0)
        tx = radius * math.cos(theta)
        ty = radius * math.sin(theta)
        ok = d.drive_to(tx, ty)
        ok_count += 1 if ok else 0
        print(f'  航点 {k:2d}/12 ({tx:5.2f},{ty:5.2f}) '
              f'{"到达" if ok else "超时"} 当前=({d.x:5.2f},{d.y:5.2f})', flush=True)
    d.stop()
    print(f'路径完成: {ok_count}/13 段', flush=True)
    d.destroy_node()
    rclpy.shutdown()
    return 0 if ok_count >= 12 else 1


if __name__ == '__main__':
    sys.exit(main())
