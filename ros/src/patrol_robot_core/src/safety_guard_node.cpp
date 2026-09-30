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
// 文件用途：激光安全守护节点
//   订阅 /scan → 前向扇区最小距离 → 迟滞判定 → 发布安全标志
//   注意：默认不发布 /cmd_vel（见 02 文档 ADR-06，16 文档 AS-46/AS-47）
#include <atomic>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>

#include "geometry_msgs/msg/twist.hpp"
#include "patrol_robot_core/safety_evaluator.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"
#include "std_msgs/msg/bool.hpp"
#include "std_msgs/msg/float32.hpp"

namespace patrol_robot_core
{
using namespace std::chrono_literals;  // NOLINT(build/namespaces)

class SafetyGuardNode : public rclcpp::Node
{
public:
  SafetyGuardNode()
  : rclcpp::Node("safety_guard_node")
  {
    SafetyEvaluator::Config cfg;
    cfg.stop_distance = declare_parameter<double>("stop_distance", 0.30);
    cfg.resume_distance = declare_parameter<double>("resume_distance", 0.60);
    cfg.fov_deg = declare_parameter<double>("fov_deg", 60.0);
    cfg.range_min = declare_parameter<double>("range_min", 0.02);
    cfg.range_max = declare_parameter<double>("range_max", 3.50);

    std::string error;
    if (!SafetyEvaluator::validateConfig(cfg, error)) {
      RCLCPP_FATAL(get_logger(), "安全参数非法：%s（拒绝启动）", error.c_str());
      throw std::invalid_argument(error);   // 显式失败，符合 NFR-09 / AS-49
    }
    evaluator_ = std::make_unique<SafetyEvaluator>(cfg);

    const std::string scan_topic = declare_parameter<std::string>("scan_topic", "scan");
    const std::string emergency_topic =
      declare_parameter<std::string>("emergency_topic", "/safety/emergency_stop");
    const std::string distance_topic =
      declare_parameter<std::string>("distance_topic", "/safety/nearest_obstacle_distance");
    const std::string cmd_vel_topic = declare_parameter<std::string>("cmd_vel_topic", "cmd_vel");
    allow_cmd_vel_override_ = declare_parameter<bool>("allow_cmd_vel_override", false);

    // 安全标志：可靠且可回溯 → RELIABLE + TRANSIENT_LOCAL + KEEP_LAST(1)
    rclcpp::QoS emergency_qos(1);
    emergency_qos.transient_local();
    emergency_pub_ = create_publisher<std_msgs::msg::Bool>(emergency_topic, emergency_qos);

    // 距离数值：高频数据用于绘图 → RELIABLE + VOLATILE + KEEP_LAST(10)
    rclcpp::QoS distance_qos(10);
    distance_pub_ = create_publisher<std_msgs::msg::Float32>(distance_topic, distance_qos);

    // 传感器数据统一用 SensorDataQoS（BEST_EFFORT），兼容性最好（AS-31）
    scan_sub_ = create_subscription<sensor_msgs::msg::LaserScan>(
      scan_topic, rclcpp::SensorDataQoS(),
      std::bind(&SafetyGuardNode::onScan, this, std::placeholders::_1));

    if (allow_cmd_vel_override_) {
      // 仅当参数显式开启时才创建接管发布者（降级预案，默认关闭，见 ADR-06）
      rclcpp::QoS cmd_vel_qos(10);
      cmd_vel_pub_ = create_publisher<geometry_msgs::msg::Twist>(
        // AS-EXEMPT(AS-46)
        cmd_vel_topic, cmd_vel_qos);
      RCLCPP_WARN(
        get_logger(),
        "allow_cmd_vel_override=true：安全节点将直接发布 %s，"
        "请确认不与 Nav2 控制器冲突！", cmd_vel_topic.c_str());
    }

    // AS-35：订阅端若迟迟收不到数据必须给出 WARN，不得静默无数据
    scan_watchdog_ = create_wall_timer(
      5s, std::bind(&SafetyGuardNode::checkScanHealth, this));

    RCLCPP_INFO(
      get_logger(), "safety_guard_node 就绪: stop=%.2fm resume=%.2fm fov=%.0f deg scan=%s",
      cfg.stop_distance, cfg.resume_distance, cfg.fov_deg, scan_topic.c_str());
  }

private:
  void onScan(const sensor_msgs::msg::LaserScan::SharedPtr scan)
  {
    scan_received_.store(true);

    LaserScanView view;
    view.ranges = &scan->ranges;
    view.angle_min = scan->angle_min;
    view.angle_increment = scan->angle_increment;
    view.range_min = scan->range_min;
    view.range_max = scan->range_max;

    const auto result = evaluator_->update(view);

    std_msgs::msg::Float32 dist_msg;
    dist_msg.data = static_cast<float>(result.min_distance);
    distance_pub_->publish(dist_msg);

    if (!result.state_changed) {
      return;  // 无状态翻转：不发布标志、不打日志（AS-75 热路径禁止刷屏）
    }

    std_msgs::msg::Bool flag;
    flag.data = result.emergency;
    emergency_pub_->publish(flag);

    if (result.emergency) {
      RCLCPP_WARN(
        get_logger(), "前方障碍过近（%.2f m < %.2f m），置位紧急标志",
        result.min_distance, evaluator_->config().stop_distance);
    } else {
      RCLCPP_INFO(
        get_logger(), "前方畅通（%.2f m > %.2f m），解除紧急标志",
        result.min_distance, evaluator_->config().resume_distance);
    }

    if (allow_cmd_vel_override_ && cmd_vel_pub_ && result.emergency) {
      // AS-EXEMPT(AS-46): 应急接管路径，默认关闭时不创建该发布者
      cmd_vel_pub_->publish(geometry_msgs::msg::Twist());  // AS-EXEMPT(AS-46)
    }
  }

  /// @brief 启动期健康检查：未收到 /scan 时周期 WARN，收到后停止检查
  void checkScanHealth()
  {
    if (scan_received_.load()) {
      RCLCPP_INFO(get_logger(), "已收到激光数据，扫描链路正常");
      scan_watchdog_->cancel();
      return;
    }
    RCLCPP_WARN(
      get_logger(),
      "尚未收到 scan_topic 数据（期望 QoS: SensorDataQoS/BEST_EFFORT），"
      "请检查话题名是否与发布端一致");
  }

  std::unique_ptr<SafetyEvaluator> evaluator_;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr emergency_pub_;
  rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr distance_pub_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;
  rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr scan_sub_;
  rclcpp::TimerBase::SharedPtr scan_watchdog_;
  std::atomic<bool> scan_received_{false};
  bool allow_cmd_vel_override_{false};
};

}  // namespace patrol_robot_core

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  try {
    rclcpp::spin(std::make_shared<patrol_robot_core::SafetyGuardNode>());
  } catch (const std::exception & e) {
    RCLCPP_FATAL(rclcpp::get_logger("safety_guard_node"), "启动失败: %s", e.what());
    rclcpp::shutdown();
    return 1;
  }
  rclcpp::shutdown();
  return 0;
}
