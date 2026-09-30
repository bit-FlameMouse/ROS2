// Copyright 2026 patrol_robot developer
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// 文件用途：NavClient 实现
#include "patrol_robot_core/nav_client.hpp"

#include <functional>
#include <utility>

#include "tf2/LinearMath/Quaternion.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"

namespace patrol_robot_core
{
using namespace std::chrono_literals;  // NOLINT(build/namespaces)

const char * toString(NavTaskState state)
{
  switch (state) {
    case NavTaskState::IDLE: return "IDLE";
    case NavTaskState::PENDING: return "PENDING";
    case NavTaskState::ACTIVE: return "ACTIVE";
    case NavTaskState::SUCCEEDED: return "SUCCEEDED";
    case NavTaskState::ABORTED: return "ABORTED";
    case NavTaskState::CANCELED: return "CANCELED";
    case NavTaskState::REJECTED: return "REJECTED";
  }
  return "UNKNOWN";
}

NavClient::NavClient(
  rclcpp::Node * node, const std::string & action_name, const std::string & frame_id)
: node_(node), frame_id_(frame_id)
{
  client_ = rclcpp_action::create_client<NavigateToPose>(node_, action_name);
}

bool NavClient::waitForServer(std::chrono::milliseconds timeout)
{
  if (!client_) {
    return false;
  }
  return client_->wait_for_action_server(timeout);
}

bool NavClient::sendGoal(double x, double y, double yaw)
{
  if (!waitForServer(5s)) {
    RCLCPP_ERROR(node_->get_logger(), "[NavClient] Action server 不可用，目标未发送");
    return false;
  }

  NavigateToPose::Goal goal;
  goal.pose.header.frame_id = frame_id_;
  goal.pose.header.stamp = node_->now();
  goal.pose.pose.position.x = x;
  goal.pose.pose.position.y = y;
  goal.pose.pose.position.z = 0.0;

  // 必须用 tf2::toMsg 生成四元数，避免手工构造漏掉 w 分量（06 文档 §7.1）
  tf2::Quaternion q;
  q.setRPY(0.0, 0.0, yaw);
  goal.pose.pose.orientation = tf2::toMsg(q);

  {
    std::lock_guard<std::mutex> lock(mutex_);
    state_ = NavTaskState::PENDING;
    distance_remaining_ = -1.0;
    recovery_count_ = 0;
    goal_sent_ = true;
  }

  auto options = rclcpp_action::Client<NavigateToPose>::SendGoalOptions();
  options.goal_response_callback =
    std::bind(&NavClient::onGoalResponse, this, std::placeholders::_1);
  options.feedback_callback =
    std::bind(&NavClient::onFeedback, this, std::placeholders::_1, std::placeholders::_2);
  options.result_callback =
    std::bind(&NavClient::onResult, this, std::placeholders::_1);

  client_->async_send_goal(goal, options);

  RCLCPP_INFO(
    node_->get_logger(), "[NavClient] 目标已发送: x=%.2f y=%.2f yaw=%.2f frame=%s",
    x, y, yaw, frame_id_.c_str());
  return true;
}

void NavClient::cancel()
{
  GoalHandle::SharedPtr handle;
  bool cancel_all = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    handle = goal_handle_;
    cancel_all = (handle == nullptr) && goal_sent_;
  }

  if (handle) {
    client_->async_cancel_goal(handle);
    RCLCPP_WARN(node_->get_logger(), "[NavClient] 已请求取消当前导航目标");
  } else if (cancel_all) {
    // 目标刚发出、服务器尚未响应，回退为取消全部目标（K-15）
    client_->async_cancel_all_goals();
    RCLCPP_WARN(node_->get_logger(), "[NavClient] 目标尚未被接受，已请求取消全部目标");
  }
}

void NavClient::detach()
{
  std::lock_guard<std::mutex> lock(mutex_);
  state_ = NavTaskState::IDLE;
  goal_handle_.reset();
  distance_remaining_ = -1.0;
  recovery_count_ = 0;
  goal_sent_ = false;
}

NavTaskState NavClient::state() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  return state_;
}

bool NavClient::isFinished() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  return state_ == NavTaskState::SUCCEEDED || state_ == NavTaskState::ABORTED ||
         state_ == NavTaskState::CANCELED || state_ == NavTaskState::REJECTED;
}

double NavClient::distanceRemaining() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  return distance_remaining_;
}

int16_t NavClient::recoveryCount() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  return recovery_count_;
}

void NavClient::onGoalResponse(GoalHandle::SharedPtr goal_handle)
{
  std::lock_guard<std::mutex> lock(mutex_);
  if (!goal_handle) {
    state_ = NavTaskState::REJECTED;
    return;
  }
  goal_handle_ = std::move(goal_handle);
  state_ = NavTaskState::ACTIVE;
}

void NavClient::onFeedback(
  GoalHandle::SharedPtr,
  const std::shared_ptr<const NavigateToPose::Feedback> feedback)
{
  std::lock_guard<std::mutex> lock(mutex_);
  distance_remaining_ = static_cast<double>(feedback->distance_remaining);
  recovery_count_ = feedback->number_of_recoveries;
}

void NavClient::onResult(const GoalHandle::WrappedResult & result)
{
  std::lock_guard<std::mutex> lock(mutex_);
  goal_handle_.reset();
  switch (result.code) {
    case rclcpp_action::ResultCode::SUCCEEDED:
      state_ = NavTaskState::SUCCEEDED;
      distance_remaining_ = 0.0;
      break;
    case rclcpp_action::ResultCode::ABORTED:
      state_ = NavTaskState::ABORTED;
      break;
    case rclcpp_action::ResultCode::CANCELED:
      state_ = NavTaskState::CANCELED;
      break;
    default:
      // UNKNOWN 保守按失败处理（05 文档 §4.1 结果码映射表）
      state_ = NavTaskState::ABORTED;
      break;
  }
}

}  // namespace patrol_robot_core
