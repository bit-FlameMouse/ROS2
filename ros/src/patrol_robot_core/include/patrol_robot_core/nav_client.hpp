// 文件用途：Nav2 NavigateToPose Action 的非阻塞客户端封装
// 版权：2026 patrol_robot developer，Apache-2.0 许可（许可证原文见仓库根目录 LICENSE 文件）
#ifndef PATROL_ROBOT_CORE__NAV_CLIENT_HPP_
#define PATROL_ROBOT_CORE__NAV_CLIENT_HPP_

#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include "nav2_msgs/action/navigate_to_pose.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"

namespace patrol_robot_core
{

/// @brief 单次导航任务的状态
enum class NavTaskState : uint8_t
{
  IDLE = 0,    ///< 未发送目标
  PENDING,     ///< 已发送，等待服务器接受
  ACTIVE,      ///< 已接受，导航进行中
  SUCCEEDED,   ///< 到达目标
  ABORTED,     ///< 导航失败
  CANCELED,    ///< 被取消
  REJECTED,    ///< 目标被服务器拒绝
};

/// @brief 导航任务状态转字符串（日志用）
const char * toString(NavTaskState state);

/// @brief NavigateToPose Action 客户端封装（非阻塞）
/// @details 线程安全：内部成员由 mutex_ 保护；回调只更新内部状态，
///          上层通过 state() 轮询（单点更新模型，见 16 文档 AS-54）。
///          代数守卫（generation_）：每个目标携带递增代数，回调仅在代数
///          与当前值一致时生效，丢弃已取消/已分离目标的迟到回调（K-16 加固）。
class NavClient
{
public:
  using NavigateToPose = nav2_msgs::action::NavigateToPose;
  using GoalHandle = rclcpp_action::ClientGoalHandle<NavigateToPose>;

  /// @brief 构造函数
  /// @param node        宿主节点（不持有所有权）
  /// @param action_name Action 名称，默认 "navigate_to_pose"
  /// @param frame_id    目标位姿参考坐标系，通常为 "map"
  NavClient(rclcpp::Node * node, const std::string & action_name, const std::string & frame_id);

  /// @brief 阻塞等待 Action Server 就绪（仅启动期使用，见 AS-60）
  /// @param timeout 超时时间
  /// @return 就绪返回 true
  bool waitForServer(std::chrono::milliseconds timeout);

  /// @brief 发送导航目标（非阻塞，立即返回）
  /// @details 服务端未就绪时立即返回 false（不做阻塞等待），
  ///          保证 tick 线程与安全响应的实时性（AS-53 例外条款口径）。
  /// @param x 目标 x（m）
  /// @param y 目标 y（m）
  /// @param yaw 目标偏航角（rad），内部转四元数
  /// @return 是否成功提交（不代表导航成功）
  bool sendGoal(double x, double y, double yaw);

  /// @brief 请求取消当前目标（目标尚未被接受时回退为 cancel all）
  void cancel();

  /// @brief 清空内部状态（下一次 sendGoal 前调用，避免读到旧结果）
  void detach();

  /// @brief 当前任务状态（线程安全）
  NavTaskState state() const;

  /// @brief 是否处于终态
  bool isFinished() const;

  /// @brief 距目标剩余距离（m），无反馈时返回 -1.0
  double distanceRemaining() const;

  /// @brief Nav2 触发恢复行为的次数
  int16_t recoveryCount() const;

private:
  void onGoalResponse(uint64_t generation, GoalHandle::SharedPtr goal_handle);
  void onFeedback(
    uint64_t generation,
    GoalHandle::SharedPtr,
    const std::shared_ptr<const NavigateToPose::Feedback> feedback);
  void onResult(uint64_t generation, const GoalHandle::WrappedResult & result);

  /// @brief 是否为终态（调用方必须已持有 mutex_）
  bool isFinished_locked() const;

  rclcpp::Node * node_{nullptr};
  std::string frame_id_;
  rclcpp_action::Client<NavigateToPose>::SharedPtr client_;

  mutable std::mutex mutex_;  ///< 保护以下全部可变成员
  NavTaskState state_{NavTaskState::IDLE};
  GoalHandle::SharedPtr goal_handle_;
  double distance_remaining_{-1.0};
  int16_t recovery_count_{0};
  bool goal_sent_{false};
  uint64_t generation_{0};  ///< 目标代数：回调携带快照代数，不一致即丢弃
};

}  // namespace patrol_robot_core

#endif  // PATROL_ROBOT_CORE__NAV_CLIENT_HPP_
