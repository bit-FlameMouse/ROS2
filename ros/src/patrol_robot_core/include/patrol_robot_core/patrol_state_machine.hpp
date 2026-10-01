// 文件用途：巡逻任务状态机（纯逻辑，不依赖 ROS 运行时，可独立单元测试）
// 说明：状态与错误码的数值必须与 patrol_interfaces/msg/PatrolStatus.msg
//       中的 MODE_* / ERR_* 常量保持一致；本类不 include 任何 ROS 头文件（AS-65），
//       因此以本地常量重复定义数值。
// 版权：2026 patrol_robot developer，Apache-2.0 许可（许可证原文见仓库根目录 LICENSE 文件）
#ifndef PATROL_ROBOT_CORE__PATROL_STATE_MACHINE_HPP_
#define PATROL_ROBOT_CORE__PATROL_STATE_MACHINE_HPP_

#include <cstdint>
#include <string>
#include <vector>

namespace patrol_robot_core
{

/// @brief 巡逻状态（数值对应 PatrolStatus.msg 的 MODE_* 常量）
enum class PatrolState : uint8_t
{
  IDLE = 0,
  MOVING = 1,
  WAITING = 2,
  PAUSED = 3,
  SAFETY_HOLD = 4,
  FAILED = 5,
};

/// @brief 状态机外部事件
enum class PatrolEvent : uint8_t
{
  START,
  GOAL_SUCCEEDED,
  GOAL_FAILED,
  WAIT_ELAPSED,
  PAUSE,
  RESUME,
  STOP,
  RESET,
  SAFETY_TRIGGERED,
  SAFETY_CLEARED,
};

/// @brief 状态机使用的错误码（数值与 PatrolStatus.msg 的 ERR_* 一致）
namespace sm_error
{
constexpr uint16_t kNone = 0;
constexpr uint16_t kNoWaypoints = 1;
constexpr uint16_t kMaxRetriesExceeded = 4;
}  // namespace sm_error

const char * toString(PatrolState state);
const char * toString(PatrolEvent event);

/// @brief 状态机对外的副作用指令
struct SmCommand
{
  bool send_goal{false};     ///< 需要向 Nav2 发送目标
  bool cancel_goal{false};   ///< 需要取消当前目标
  bool restart_wait{false};  ///< 需要重置停留计时（恢复到 WAITING 时）
  uint32_t target_index{0};  ///< send_goal 时的目标航点索引
};

/// @brief 巡逻状态机（单点更新：只允许由 tick 线程调用 handle()）
class PatrolStateMachine
{
public:
  struct Config
  {
    uint32_t waypoint_count{0};
    uint32_t loop_count{0};           ///< 0 = 无限循环
    uint32_t max_retries{2};          ///< 单个航点最大尝试次数（含首次）
    bool skip_failed_waypoint{true};  ///< 重试超限后是否跳过
    std::vector<double> wait_table;   ///< 各航点停留时间（s）
  };

  /// @brief 构造函数
  /// @param config 状态机配置（由 patrol_node 从参数构造）
  explicit PatrolStateMachine(const Config & config);

  /// @brief 处理一个事件，返回需要执行的副作用
  /// @param event 外部事件
  /// @return 需要执行的副作用指令
  SmCommand handle(PatrolEvent event);

  /// @brief 当前状态
  PatrolState state() const {return state_;}
  /// @brief 当前目标航点索引
  uint32_t waypointIndex() const {return index_;}
  /// @brief 已完成的循环轮次
  uint32_t loopCount() const {return loop_;}
  /// @brief 累计到达的航点数
  uint32_t completedWaypoints() const {return completed_;}
  /// @brief 当前航点重试次数
  int32_t retryCount() const {return retry_;}
  /// @brief 最近一次错误码
  uint16_t errorCode() const {return error_code_;}
  /// @brief 最近一次错误描述
  const std::string & errorMessage() const {return error_message_;}
  /// @brief 是否已跑完全部循环（loop_count 非 0 时有效）
  bool finishedAllLoops() const {return finished_all_;}

  /// @brief 当前航点的停留时间（s）
  double currentWaitSec() const;

private:
  /// @brief 推进到下一个航点；返回 false 表示本轮结束
  bool advanceIndex();

  Config config_;
  PatrolState state_{PatrolState::IDLE};
  /// @brief 进入 PAUSED / SAFETY_HOLD 前的状态（MOVING 或 WAITING），
  ///        用于恢复时区分"重新发目标"与"回到停留"
  PatrolState resume_state_{PatrolState::MOVING};
  uint32_t index_{0};
  uint32_t loop_{0};
  uint32_t completed_{0};
  int32_t retry_{0};
  uint16_t error_code_{0};
  std::string error_message_;
  bool finished_all_{false};
};

}  // namespace patrol_robot_core

#endif  // PATROL_ROBOT_CORE__PATROL_STATE_MACHINE_HPP_
