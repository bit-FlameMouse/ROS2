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
// 文件用途：WaypointLoader 实现
#include "patrol_robot_core/waypoint_loader.hpp"

#include <cmath>
#include <sstream>

namespace patrol_robot_core
{
namespace
{
constexpr double kPi = 3.14159265358979323846;
}  // namespace

double WaypointLoader::normalizeAngle(double yaw)
{
  // 归一化到 (-pi, pi]；与 06 文档 §7.2 一致：pi 归一化后仍为 pi（不转成 -pi）
  double a = std::fmod(yaw, 2.0 * kPi);
  if (a > kPi) {
    a -= 2.0 * kPi;
  } else if (a <= -kPi) {
    a += 2.0 * kPi;
  }
  return a;
}

bool WaypointLoader::fromArrays(
  const std::vector<double> & xs,
  const std::vector<double> & ys,
  const std::vector<double> & yaws,
  const std::vector<double> & waits,
  const std::string & frame_id,
  WaypointSet & out,
  std::string & error)
{
  error.clear();
  out.waypoints.clear();
  out.frame_id = frame_id;

  // VL-01：长度一致性
  if (xs.size() != ys.size() || xs.size() != yaws.size() || xs.size() != waits.size()) {
    std::ostringstream oss;
    oss << "航点参数数组长度不一致: x=" << xs.size() << ", y=" << ys.size()
        << ", yaw=" << yaws.size() << ", wait=" << waits.size();
    error = oss.str();
    return false;
  }

  // VL-02：至少一个航点
  if (xs.empty()) {
    error = "未配置任何航点（waypoint_x 为空）";
    return false;
  }

  for (size_t i = 0; i < xs.size(); ++i) {
    // VL-05：有限性
    if (!std::isfinite(xs[i]) || !std::isfinite(ys[i]) ||
      !std::isfinite(yaws[i]) || !std::isfinite(waits[i]))
    {
      std::ostringstream oss;
      oss << "航点 " << i << " 包含非法数值（NaN/Inf）";
      error = oss.str();
      return false;
    }
    // VL-03：停留时间非负
    if (waits[i] < 0.0) {
      std::ostringstream oss;
      oss << "航点 " << i << " 的停留时间不能为负: " << waits[i];
      error = oss.str();
      return false;
    }

    Waypoint wp;
    wp.x = xs[i];
    wp.y = ys[i];
    wp.yaw = normalizeAngle(yaws[i]);  // VL-04
    wp.wait_sec = waits[i];
    wp.index = static_cast<uint32_t>(i);
    out.waypoints.push_back(wp);
  }

  return validate(out, error);
}

bool WaypointLoader::validate(const WaypointSet & set, std::string & error)
{
  error.clear();
  if (set.waypoints.empty()) {
    error = "航点集合为空";
    return false;
  }
  if (set.frame_id.empty()) {
    error = "frame_id 不能为空";
    return false;
  }
  for (const auto & wp : set.waypoints) {
    if (!std::isfinite(wp.x) || !std::isfinite(wp.y) ||
      !std::isfinite(wp.yaw) || !std::isfinite(wp.wait_sec))
    {
      error = "航点包含非法数值";
      return false;
    }
    if (wp.wait_sec < 0.0) {
      error = "航点停留时间为负";
      return false;
    }
  }
  return true;
}

std::string WaypointLoader::describe(const WaypointSet & set)
{
  std::ostringstream oss;
  oss << "航点集合 frame=" << set.frame_id << " count=" << set.waypoints.size();
  for (const auto & wp : set.waypoints) {
    oss << "\n  [" << wp.index << "] x=" << wp.x << " y=" << wp.y
        << " yaw=" << wp.yaw << " wait=" << wp.wait_sec << "s";
  }
  return oss.str();
}

}  // namespace patrol_robot_core
