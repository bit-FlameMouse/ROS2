// 文件用途：PatrolStateMachine 实现
// 版权：2026 patrol_robot developer，Apache-2.0 许可（许可证原文见仓库根目录 LICENSE 文件）
#include "patrol_robot_core/patrol_state_machine.hpp"

namespace patrol_robot_core
{

const char * toString(PatrolState state)
{
  switch (state) {
    case PatrolState::IDLE: return "IDLE";
    case PatrolState::MOVING: return "MOVING";
    case PatrolState::WAITING: return "WAITING";
    case PatrolState::PAUSED: return "PAUSED";
    case PatrolState::SAFETY_HOLD: return "SAFETY_HOLD";
    case PatrolState::FAILED: return "FAILED";
  }
  return "UNKNOWN";
}

const char * toString(PatrolEvent event)
{
  switch (event) {
    case PatrolEvent::START: return "START";
    case PatrolEvent::GOAL_SUCCEEDED: return "GOAL_SUCCEEDED";
    case PatrolEvent::GOAL_FAILED: return "GOAL_FAILED";
    case PatrolEvent::WAIT_ELAPSED: return "WAIT_ELAPSED";
    case PatrolEvent::PAUSE: return "PAUSE";
    case PatrolEvent::RESUME: return "RESUME";
    case PatrolEvent::STOP: return "STOP";
    case PatrolEvent::RESET: return "RESET";
    case PatrolEvent::SAFETY_TRIGGERED: return "SAFETY_TRIGGERED";
    case PatrolEvent::SAFETY_CLEARED: return "SAFETY_CLEARED";
  }
  return "UNKNOWN";
}

PatrolStateMachine::PatrolStateMachine(const Config & config)
: config_(config)
{
}

bool PatrolStateMachine::advanceIndex()
{
  if (index_ + 1 < config_.waypoint_count) {
    ++index_;
    return true;
  }
  return false;  // 本轮结束
}

double PatrolStateMachine::currentWaitSec() const
{
  if (index_ < config_.wait_table.size()) {
    return config_.wait_table[index_];
  }
  return 0.0;
}

SmCommand PatrolStateMachine::handle(PatrolEvent event)
{
  SmCommand cmd;

  switch (event) {
    case PatrolEvent::START:
      if (state_ == PatrolState::IDLE) {
        if (config_.waypoint_count == 0) {
          error_code_ = sm_error::kNoWaypoints;
          error_message_ = "未配置航点，无法开始巡逻";
          return cmd;
        }
        state_ = PatrolState::MOVING;
        cmd.send_goal = true;
        cmd.target_index = index_;
      }
      return cmd;

    case PatrolEvent::STOP:
      if (state_ != PatrolState::IDLE) {
        cmd.cancel_goal = true;
      }
      state_ = PatrolState::IDLE;
      index_ = 0;
      retry_ = 0;
      return cmd;

    case PatrolEvent::RESET:
      completed_ = 0;
      retry_ = 0;
      loop_ = 0;
      error_code_ = sm_error::kNone;
      error_message_.clear();
      finished_all_ = false;
      if (state_ == PatrolState::FAILED) {
        state_ = PatrolState::IDLE;
        index_ = 0;
      }
      return cmd;

    case PatrolEvent::PAUSE:
      if (state_ == PatrolState::MOVING || state_ == PatrolState::WAITING) {
        state_ = PatrolState::PAUSED;
        cmd.cancel_goal = true;
      }
      return cmd;

    case PatrolEvent::RESUME:
      if (state_ == PatrolState::PAUSED) {
        state_ = PatrolState::MOVING;
        cmd.send_goal = true;
        cmd.target_index = index_;
      }
      return cmd;

    case PatrolEvent::SAFETY_TRIGGERED:
      if (state_ == PatrolState::MOVING || state_ == PatrolState::WAITING) {
        state_ = PatrolState::SAFETY_HOLD;
        cmd.cancel_goal = true;
      }
      return cmd;

    case PatrolEvent::SAFETY_CLEARED:
      if (state_ == PatrolState::SAFETY_HOLD) {
        state_ = PatrolState::MOVING;
        cmd.send_goal = true;
        cmd.target_index = index_;
      }
      return cmd;

    case PatrolEvent::GOAL_SUCCEEDED:
      if (state_ == PatrolState::MOVING) {
        state_ = PatrolState::WAITING;
        ++completed_;
        retry_ = 0;
      }
      return cmd;

    case PatrolEvent::GOAL_FAILED:
      if (state_ != PatrolState::MOVING) {
        return cmd;
      }
      ++retry_;
      if (retry_ < static_cast<int32_t>(config_.max_retries)) {
        cmd.send_goal = true;
        cmd.target_index = index_;
        return cmd;
      }
      if (config_.skip_failed_waypoint && advanceIndex()) {
        retry_ = 0;
        cmd.send_goal = true;
        cmd.target_index = index_;
        return cmd;
      }
      state_ = PatrolState::FAILED;
      error_code_ = sm_error::kMaxRetriesExceeded;
      error_message_ = "航点重试次数超限，任务终止";
      return cmd;

    case PatrolEvent::WAIT_ELAPSED:
      if (state_ != PatrolState::WAITING) {
        return cmd;
      }
      if (advanceIndex()) {
        state_ = PatrolState::MOVING;
        cmd.send_goal = true;
        cmd.target_index = index_;
        return cmd;
      }
      // 本轮结束，判断是否继续循环
      ++loop_;
      if (config_.loop_count == 0 || loop_ < config_.loop_count) {
        index_ = 0;
        state_ = PatrolState::MOVING;
        cmd.send_goal = true;
        cmd.target_index = index_;
        return cmd;
      }
      finished_all_ = true;
      state_ = PatrolState::IDLE;
      index_ = 0;
      return cmd;
  }

  return cmd;
}

}  // namespace patrol_robot_core
