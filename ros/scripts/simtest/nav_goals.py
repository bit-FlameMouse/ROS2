#!/usr/bin/env python3
# 文件用途：单点导航成功率测试（TC-I-05 辅助）——依次下发 5 个目标点并统计到达率
#   目标点均位于外环通道（距障碍/墙体 >= 0.45 m，已用地图校验）；
#   目标消息不设置 stamp（=0），由 Nav2 使用最新 TF 时间，避免系统时间/仿真时间不一致
# 前置： navigation.launch.py 或 patrol.launch.py 已启动且 Nav2 已激活
# 用法： python3 scripts/simtest/nav_goals.py
# 版权：2026 patrol_robot developer，Apache-2.0 许可（许可证原文见仓库根目录 LICENSE 文件）
"""单点导航 5 目标测试。"""
import math
import sys
import time

import rclpy
from nav2_msgs.action import NavigateToPose
from rclpy.action import ActionClient
from rclpy.node import Node

# (x, y, yaw)：前 3 个为演示航点（外环），后 2 个为内环可用点
TARGETS = [
    (1.75, 0.0, 0.0),
    (0.0, 1.9, 1.57),
    (-1.75, 0.0, 3.14),
    (0.5, 0.5, 0.0),
    (0.0, -1.9, -1.57),
]


class GoalSender(Node):
    def __init__(self):
        super().__init__('nav_goal_sender')
        self.client = ActionClient(self, NavigateToPose, '/navigate_to_pose')

    def send(self, x, y, yaw, timeout):
        if not self.client.wait_for_server(timeout_sec=30.0):
            return False, 0.0
        goal = NavigateToPose.Goal()
        goal.pose.header.frame_id = 'map'
        goal.pose.pose.position.x = x
        goal.pose.pose.position.y = y
        goal.pose.pose.orientation.z = math.sin(yaw / 2.0)
        goal.pose.pose.orientation.w = math.cos(yaw / 2.0)
        t0 = time.time()
        future = self.client.send_goal_async(goal)
        rclpy.spin_until_future_complete(self, future, timeout_sec=10.0)
        handle = future.result()
        if handle is None or not handle.accepted:
            return False, time.time() - t0
        result_future = handle.get_result_async()
        rclpy.spin_until_future_complete(self, result_future, timeout_sec=timeout)
        if not result_future.done():
            handle.cancel_goal_async()
            return False, time.time() - t0
        return result_future.result().status == 4, time.time() - t0  # 4 = SUCCEEDED


def main():
    rclpy.init()
    node = GoalSender()
    ok_count = 0
    for i, (x, y, yaw) in enumerate(TARGETS):
        ok, dt = node.send(x, y, yaw, timeout=180.0)
        ok_count += 1 if ok else 0
        print(f'  目标 {i + 1}/5 ({x:+.2f},{y:+.2f}) -> '
              f'{"到达" if ok else "失败/超时"}（{dt:.1f}s）', flush=True)
    print(f'  单点导航成功率 = {ok_count}/5')
    node.destroy_node()
    rclpy.shutdown()
    return 0 if ok_count == 5 else 1


if __name__ == '__main__':
    sys.exit(main())
