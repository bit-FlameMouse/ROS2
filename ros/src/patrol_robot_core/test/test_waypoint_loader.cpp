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
// 文件用途：WaypointLoader 单元测试（TC-U-01 ~ TC-U-03f）
#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include "patrol_robot_core/waypoint_loader.hpp"

using patrol_robot_core::WaypointLoader;
using patrol_robot_core::WaypointSet;

// TC-U-01 正常解析
TEST(WaypointLoaderTest, ParseValidArrays)
{
  WaypointSet set;
  std::string error;
  const bool ok = WaypointLoader::fromArrays(
    {1.0, 2.0}, {3.0, 4.0}, {0.0, 1.57}, {2.0, 3.0}, "map", set, error);
  ASSERT_TRUE(ok) << error;
  ASSERT_EQ(set.waypoints.size(), 2u);
  EXPECT_DOUBLE_EQ(set.waypoints[0].x, 1.0);
  EXPECT_DOUBLE_EQ(set.waypoints[0].y, 3.0);
  EXPECT_DOUBLE_EQ(set.waypoints[1].wait_sec, 3.0);
  EXPECT_EQ(set.waypoints[1].index, 1u);
  EXPECT_EQ(set.frame_id, "map");
}

// TC-U-02 长度不一致
TEST(WaypointLoaderTest, RejectMismatchedLengths)
{
  WaypointSet set;
  std::string error;
  const bool ok = WaypointLoader::fromArrays(
    {1.0, 2.0}, {3.0}, {0.0, 1.0}, {2.0, 3.0}, "map", set, error);
  EXPECT_FALSE(ok);
  EXPECT_NE(error.find("长度不一致"), std::string::npos);
}

// TC-U-03 空数组
TEST(WaypointLoaderTest, RejectEmptyArrays)
{
  WaypointSet set;
  std::string error;
  EXPECT_FALSE(WaypointLoader::fromArrays({}, {}, {}, {}, "map", set, error));
  EXPECT_NE(error.find("未配置任何航点"), std::string::npos);
}

// TC-U-03a 负停留时间
TEST(WaypointLoaderTest, RejectNegativeWait)
{
  WaypointSet set;
  std::string error;
  EXPECT_FALSE(WaypointLoader::fromArrays({1.0}, {2.0}, {0.0}, {-1.0}, "map", set, error));
  EXPECT_NE(error.find("不能为负"), std::string::npos);
}

// TC-U-03b NaN 值
TEST(WaypointLoaderTest, RejectNaN)
{
  WaypointSet set;
  std::string error;
  const double nan_value = std::numeric_limits<double>::quiet_NaN();
  EXPECT_FALSE(WaypointLoader::fromArrays({nan_value}, {2.0}, {0.0}, {1.0}, "map", set, error));
  EXPECT_NE(error.find("非法数值"), std::string::npos);
}

// TC-U-03c yaw 归一化（3π → π）
TEST(WaypointLoaderTest, NormalizePositiveYaw)
{
  WaypointSet set;
  std::string error;
  const double three_pi = 3.0 * 3.14159265358979323846;
  ASSERT_TRUE(WaypointLoader::fromArrays({0.0}, {0.0}, {three_pi}, {0.0}, "map", set, error));
  EXPECT_NEAR(set.waypoints[0].yaw, 3.14159265358979323846, 1e-9);
}

// TC-U-03d yaw 归一化（-3π → 幅度为 π）
TEST(WaypointLoaderTest, NormalizeNegativeYaw)
{
  WaypointSet set;
  std::string error;
  const double minus_three_pi = -3.0 * 3.14159265358979323846;
  ASSERT_TRUE(WaypointLoader::fromArrays({0.0}, {0.0}, {minus_three_pi}, {0.0}, "map", set, error));
  EXPECT_NEAR(std::fabs(set.waypoints[0].yaw), 3.14159265358979323846, 1e-9);
}

// TC-U-03e 单航点
TEST(WaypointLoaderTest, AcceptSingleWaypoint)
{
  WaypointSet set;
  std::string error;
  ASSERT_TRUE(WaypointLoader::fromArrays({1.0}, {2.0}, {0.5}, {1.0}, "map", set, error));
  EXPECT_EQ(set.waypoints.size(), 1u);
  EXPECT_EQ(set.waypoints[0].index, 0u);
}

// TC-U-03f Inf 值
TEST(WaypointLoaderTest, RejectInf)
{
  WaypointSet set;
  std::string error;
  const double inf_value = std::numeric_limits<double>::infinity();
  EXPECT_FALSE(WaypointLoader::fromArrays({0.0}, {inf_value}, {0.0}, {1.0}, "map", set, error));
}

// 附加：归一化边界（π 保持为 π，-π 变为 π）
TEST(WaypointLoaderTest, NormalizeBoundaryPi)
{
  const double pi = 3.14159265358979323846;
  EXPECT_NEAR(WaypointLoader::normalizeAngle(pi), pi, 1e-12);
  EXPECT_NEAR(WaypointLoader::normalizeAngle(-pi), pi, 1e-12);
  EXPECT_NEAR(WaypointLoader::normalizeAngle(0.0), 0.0, 1e-12);
}
