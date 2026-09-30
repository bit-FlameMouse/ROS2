// 文件用途：SafetyEvaluator 单元测试（TC-U-06a ~ TC-U-06i）
// 版权：2026 patrol_robot developer，Apache-2.0 许可（许可证原文见仓库根目录 LICENSE 文件）
#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include "patrol_robot_core/safety_evaluator.hpp"

using patrol_robot_core::LaserScanView;
using patrol_robot_core::SafetyEvaluator;

namespace
{
constexpr double kPi = 3.14159265358979323846;

/// @brief 构造激光输入视图；ranges 的生存期由调用方保证
LaserScanView makeView(const std::vector<float> & ranges)
{
  LaserScanView view;
  view.ranges = &ranges;
  view.angle_min = -kPi;
  view.angle_increment =
    ranges.empty() ? 0.0 : (2.0 * kPi / static_cast<double>(ranges.size()));
  view.range_min = 0.12;
  view.range_max = 3.5;
  return view;
}

/// @brief n 点扫描，仅正前方（索引中点）为 front 距离，其余为 other
std::vector<float> centeredObstacle(size_t n, float front, float other = 5.0f)
{
  std::vector<float> ranges(n, other);  // 5.0 > range_max，视为无效点
  ranges[n / 2] = front;
  return ranges;
}
}  // namespace

// TC-U-06a 无障碍
TEST(SafetyEvaluatorTest, NoObstacle)
{
  SafetyEvaluator ev(SafetyEvaluator::Config{});
  const auto ranges = centeredObstacle(361, 5.0f, 5.0f);
  const auto result = ev.update(makeView(ranges));
  EXPECT_FALSE(result.emergency);
  EXPECT_FALSE(result.has_valid_point);
  EXPECT_FALSE(result.state_changed);
}

// TC-U-06b 正前方 0.2 m 障碍
TEST(SafetyEvaluatorTest, TriggersOnCloseObstacle)
{
  SafetyEvaluator ev(SafetyEvaluator::Config{});
  const auto ranges = centeredObstacle(361, 0.20f);
  const auto result = ev.update(makeView(ranges));
  EXPECT_TRUE(result.emergency);
  EXPECT_TRUE(result.state_changed);
  EXPECT_TRUE(result.has_valid_point);
  EXPECT_NEAR(result.min_distance, 0.20, 0.02);
}

// TC-U-06c 迟滞区间内不翻转
TEST(SafetyEvaluatorTest, HysteresisHoldsState)
{
  SafetyEvaluator ev(SafetyEvaluator::Config{});
  const auto trigger = centeredObstacle(361, 0.20f);
  ev.update(makeView(trigger));

  const auto in_band = centeredObstacle(361, 0.45f);  // 0.3 < 0.45 < 0.6
  const auto result = ev.update(makeView(in_band));
  EXPECT_TRUE(result.emergency);
  EXPECT_FALSE(result.state_changed);
}

// TC-U-06d 超过 resume 解除
TEST(SafetyEvaluatorTest, ClearsBeyondResume)
{
  SafetyEvaluator ev(SafetyEvaluator::Config{});
  const auto trigger = centeredObstacle(361, 0.20f);
  ev.update(makeView(trigger));

  const auto clear = centeredObstacle(361, 0.80f);
  const auto result = ev.update(makeView(clear));
  EXPECT_FALSE(result.emergency);
  EXPECT_TRUE(result.state_changed);
}

// TC-U-06e 侧向障碍不触发（fov=60°，90° 不在扇区内）
TEST(SafetyEvaluatorTest, IgnoresSideObstacle)
{
  SafetyEvaluator ev(SafetyEvaluator::Config{});
  std::vector<float> ranges(361, 5.0f);
  const size_t index_90deg = 270;  // (-pi + 270 * 2pi/361) ≈ +90°
  ranges[index_90deg] = 0.20f;
  const auto result = ev.update(makeView(ranges));
  EXPECT_FALSE(result.emergency);
  EXPECT_FALSE(result.has_valid_point);
}

// TC-U-06f NaN 过滤
TEST(SafetyEvaluatorTest, FiltersNaN)
{
  SafetyEvaluator ev(SafetyEvaluator::Config{});
  std::vector<float> ranges(361, 5.0f);
  ranges[180] = std::numeric_limits<float>::quiet_NaN();
  const auto result = ev.update(makeView(ranges));
  EXPECT_FALSE(result.emergency);
  EXPECT_FALSE(result.has_valid_point);
}

// TC-U-06g 空数组：不崩溃、保持状态、距离置 0（05 §2.4 契约）
TEST(SafetyEvaluatorTest, KeepsStateOnEmptyScan)
{
  SafetyEvaluator ev(SafetyEvaluator::Config{});
  const auto trigger = centeredObstacle(361, 0.20f);
  ev.update(makeView(trigger));  // 先进入紧急

  const std::vector<float> empty;
  const auto result = ev.update(makeView(empty));
  EXPECT_TRUE(result.emergency);
  EXPECT_FALSE(result.state_changed);
  EXPECT_DOUBLE_EQ(result.min_distance, 0.0);
}

// TC-U-06h 参数非法
TEST(SafetyEvaluatorTest, RejectsInvalidConfig)
{
  SafetyEvaluator::Config cfg;
  cfg.stop_distance = 0.8;
  cfg.resume_distance = 0.6;
  std::string error;
  EXPECT_FALSE(SafetyEvaluator::validateConfig(cfg, error));
  EXPECT_FALSE(error.empty());
  EXPECT_THROW(SafetyEvaluator{cfg}, std::invalid_argument);
}

// TC-U-06i 边界值：恰好等于 stop_distance 不触发（严格小于才触发）
TEST(SafetyEvaluatorTest, BoundaryExactlyStopDistance)
{
  SafetyEvaluator ev(SafetyEvaluator::Config{});
  const auto ranges = centeredObstacle(361, 0.30f);
  const auto result = ev.update(makeView(ranges));
  EXPECT_FALSE(result.emergency);
}

// 附加：量程过滤（低于 range_min 的点不参与判定）
TEST(SafetyEvaluatorTest, FiltersBelowSensorRangeMin)
{
  SafetyEvaluator ev(SafetyEvaluator::Config{});
  const auto ranges = centeredObstacle(361, 0.05f);  // < scan.range_min=0.12
  const auto result = ev.update(makeView(ranges));
  EXPECT_FALSE(result.emergency);
  EXPECT_FALSE(result.has_valid_point);
}

// 附加：fov 参数校验
TEST(SafetyEvaluatorTest, RejectsInvalidFov)
{
  SafetyEvaluator::Config cfg;
  cfg.fov_deg = 0.0;
  std::string error;
  EXPECT_FALSE(SafetyEvaluator::validateConfig(cfg, error));

  SafetyEvaluator::Config cfg2;
  cfg2.range_max = cfg2.range_min;
  EXPECT_FALSE(SafetyEvaluator::validateConfig(cfg2, error));
}
