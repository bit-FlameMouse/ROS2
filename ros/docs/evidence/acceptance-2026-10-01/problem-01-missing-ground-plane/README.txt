发现问题：世界缺少 ground_plane/sun 模型，机器人出生后持续自由落体
时间：2026-10-01 22:56:52
根因：simulation.launch.py 覆盖 GAZEBO_MODEL_PATH，未包含 /usr/share/gazebo-11/models
证据：
  - /odom z=-265398 且按 0.5*g*t^2 增长（odom_free_fall.txt）
  - Gazebo 模型列表缺少 ground_plane（model_list_no_ground.txt）
  - gzserver 报 Unable to find uri[model://ground_plane]（gzserver_errors.log）
