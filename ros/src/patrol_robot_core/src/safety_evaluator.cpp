// 文件用途：SafetyEvaluator 实现
// 版权：2026 patrol_robot developer，Apache-2.0 许可（许可证原文见仓库根目录 LICENSE 文件）
#include "patrol_robot_core/safety_evaluator.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace patrol_robot_core
{
namespace
{
constexpr double kDeg2Rad = 0.017453292519943295;
}  // namespace

SafetyEvaluator::SafetyEvaluator(const Config & config)
: config_(config)
{
  std::string error;
  if (!validateConfig(config_, error)) {
    throw std::invalid_argument("SafetyEvaluator 配置非法: " + error);
  }
}

bool SafetyEvaluator::validateConfig(const Config & config, std::string & error)
{
  error.clear();
  if (config.stop_distance <= 0.0) {
    error = "stop_distance 必须大于 0";
    return false;
  }
  if (config.resume_distance <= config.stop_distance) {
    std::ostringstream oss;
    oss << "resume_distance(" << config.resume_distance << ") 必须大于 stop_distance("
        << config.stop_distance << ")，否则会产生抖动";
    error = oss.str();
    return false;
  }
  if (config.fov_deg <= 0.0 || config.fov_deg > 360.0) {
    error = "fov_deg 必须在 (0, 360] 范围内";
    return false;
  }
  if (config.range_max <= config.range_min) {
    error = "range_max 必须大于 range_min";
    return false;
  }
  return true;
}

SafetyResult SafetyEvaluator::update(const LaserScanView & scan)
{
  SafetyResult result;

  // FR-10.1：空数据/无效角度增量不改变状态；min_distance 置 0 表示"未收到有效数据"
  // （契约见 05 文档 §2.4：inf = 扇区内无障碍，0.0 = 未收到有效数据）
  if (scan.ranges == nullptr || scan.ranges->empty() || scan.angle_increment == 0.0) {
    result.min_distance = 0.0;
    result.emergency = emergency_;
    return result;
  }

  const double half_fov = config_.fov_deg * 0.5 * kDeg2Rad;
  // 有效量程取"配置值"与"传感器自报值"的交集，避免用到传感器标称外的数据
  const double eff_min = std::max(config_.range_min, scan.range_min);
  const double eff_max = std::min(config_.range_max, scan.range_max);

  double min_dist = std::numeric_limits<double>::infinity();
  bool found = false;

  for (size_t i = 0; i < scan.ranges->size(); ++i) {
    const double angle = scan.angle_min + static_cast<double>(i) * scan.angle_increment;
    if (std::fabs(angle) > half_fov) {
      continue;
    }
    const double r = (*scan.ranges)[i];
    // 过滤 NaN / Inf / 超量程
    if (!std::isfinite(r) || r < eff_min || r > eff_max) {
      continue;
    }
    if (r < min_dist) {
      min_dist = r;
      found = true;
    }
  }

  result.has_valid_point = found;
  result.min_distance = found ? min_dist : std::numeric_limits<double>::infinity();

  const bool prev = emergency_;
  if (found && min_dist < config_.stop_distance) {
    emergency_ = true;
  } else if (!found || min_dist > config_.resume_distance) {
    emergency_ = false;
  }
  // 其余情况保持原状态（迟滞区间，见 04 文档 §7.2）

  result.emergency = emergency_;
  result.state_changed = (prev != emergency_);
  return result;
}

}  // namespace patrol_robot_core
