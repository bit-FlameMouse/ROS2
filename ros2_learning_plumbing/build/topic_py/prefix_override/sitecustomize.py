import sys
if sys.prefix == '/usr':
    sys.real_prefix = sys.prefix
    sys.prefix = sys.exec_prefix = '/home/lich/ROS2/ros2_learning_plumbing/install/topic_py'
