# 巡逻状态消息
# 发布者：patrol_node
# 频率：>= 1 Hz（由 status_publish_hz 参数控制）
# 契约：见 docs/05-接口设计说明书.md §2.1

# ---- 运行模式常量 ----
uint8 MODE_IDLE          = 0   # 空闲，未巡逻
uint8 MODE_MOVING        = 1   # 正在前往目标航点
uint8 MODE_WAITING       = 2   # 已到达航点，停留中
uint8 MODE_PAUSED        = 3   # 人工暂停
uint8 MODE_SAFETY_HOLD   = 4   # 因安全守护暂停
uint8 MODE_FAILED        = 5   # 任务失败终止

# ---- 错误码常量 ----
uint16 ERR_NONE                   = 0
uint16 ERR_NO_WAYPOINTS           = 1   # 未配置航点
uint16 ERR_NAV_SERVER_UNAVAILABLE = 2   # Nav2 Action Server 不可用
uint16 ERR_WAYPOINT_TIMEOUT       = 3   # 航点导航超时
uint16 ERR_MAX_RETRIES_EXCEEDED   = 4   # 重试次数超限
uint16 ERR_GOAL_REJECTED          = 5   # 目标被 Nav2 拒绝

std_msgs/Header header

uint8   mode                     # 当前模式，取值见上方常量
uint32  current_waypoint_index   # 当前目标航点索引（从 0 开始）
uint32  total_waypoints          # 航点总数
uint32  completed_waypoints      # 累计成功到达的航点数
uint32  current_loop             # 当前已完成的循环轮次
int32   retry_count              # 当前航点的重试次数
float32 distance_remaining       # 距当前目标剩余距离（m），未知时为 -1.0
uint16  error_code               # 最近一次错误码
string  error_message            # 最近一次错误描述（无错误时为空）
