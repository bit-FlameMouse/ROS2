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
// 文件用途：PatrolStateMachine 单元测试（TC-U-04a ~ TC-U-04l）
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "patrol_robot_core/patrol_state_machine.hpp"

using patrol_robot_core::PatrolEvent;
using patrol_robot_core::PatrolState;
using patrol_robot_core::PatrolStateMachine;
using patrol_robot_core::SmCommand;

namespace
{

PatrolStateMachine::Config makeConfig(
  uint32_t waypoint_count = 3,
  uint32_t loop_count = 1,
  uint32_t max_retries = 2,
  bool skip_failed = true)
{
  PatrolStateMachine::Config cfg;
  cfg.waypoint_count = waypoint_count;
  cfg.loop_count = loop_count;
  cfg.max_retries = max_retries;
  cfg.skip_failed_waypoint = skip_failed;
  cfg.wait_table = std::vector<double>(waypoint_count, 2.0);
  return cfg;
}

}  // namespace

// TC-U-04a 完成一轮（loop_count=1）
TEST(PatrolStateMachineTest, CompletesSingleLoop)
{
  PatrolStateMachine sm(makeConfig(3, 1));
  sm.handle(PatrolEvent::START);
  for (int i = 0; i < 3; ++i) {
    sm.handle(PatrolEvent::GOAL_SUCCEEDED);
    sm.handle(PatrolEvent::WAIT_ELAPSED);
  }
  EXPECT_EQ(sm.state(), PatrolState::IDLE);
  EXPECT_EQ(sm.completedWaypoints(), 3u);
  EXPECT_EQ(sm.loopCount(), 1u);
  EXPECT_TRUE(sm.finishedAllLoops());
}

// TC-U-04b 无限循环（loop_count=0）
TEST(PatrolStateMachineTest, RepeatsForeverWhenLoopZero)
{
  PatrolStateMachine sm(makeConfig(3, 0));
  sm.handle(PatrolEvent::START);
  for (int i = 0; i < 3; ++i) {
    sm.handle(PatrolEvent::GOAL_SUCCEEDED);
    const auto cmd = sm.handle(PatrolEvent::WAIT_ELAPSED);
    if (i == 2) {
      EXPECT_TRUE(cmd.send_goal);
    }
  }
  EXPECT_EQ(sm.state(), PatrolState::MOVING);
  EXPECT_EQ(sm.waypointIndex(), 0u);
  EXPECT_FALSE(sm.finishedAllLoops());
}

// TC-U-04c 重试超限（单航点、skip=false）
TEST(PatrolStateMachineTest, FailsAfterMaxRetries)
{
  PatrolStateMachine sm(makeConfig(1, 1, 2, false));
  sm.handle(PatrolEvent::START);
  sm.handle(PatrolEvent::GOAL_FAILED);
  EXPECT_EQ(sm.state(), PatrolState::MOVING);  // 尚有重试机会
  sm.handle(PatrolEvent::GOAL_FAILED);
  EXPECT_EQ(sm.state(), PatrolState::FAILED);
  EXPECT_EQ(sm.errorCode(), 4u);
  EXPECT_FALSE(sm.errorMessage().empty());
}

// TC-U-04d 跳过失败点
TEST(PatrolStateMachineTest, SkipsFailedWaypoint)
{
  PatrolStateMachine sm(makeConfig(3, 1, 2, true));
  sm.handle(PatrolEvent::START);
  sm.handle(PatrolEvent::GOAL_FAILED);
  const auto cmd = sm.handle(PatrolEvent::GOAL_FAILED);
  EXPECT_TRUE(cmd.send_goal);
  EXPECT_EQ(cmd.target_index, 1u);
  EXPECT_EQ(sm.state(), PatrolState::MOVING);
  EXPECT_EQ(sm.waypointIndex(), 1u);
  EXPECT_EQ(sm.retryCount(), 0);
}

// TC-U-04e 暂停 / 恢复
TEST(PatrolStateMachineTest, PauseAndResume)
{
  PatrolStateMachine sm(makeConfig(3, 1));
  sm.handle(PatrolEvent::START);
  const auto pause_cmd = sm.handle(PatrolEvent::PAUSE);
  EXPECT_EQ(sm.state(), PatrolState::PAUSED);
  EXPECT_TRUE(pause_cmd.cancel_goal);

  const auto resume_cmd = sm.handle(PatrolEvent::RESUME);
  EXPECT_EQ(sm.state(), PatrolState::MOVING);
  EXPECT_TRUE(resume_cmd.send_goal);
  EXPECT_EQ(resume_cmd.target_index, 0u);
}

// TC-U-04f 安全暂停 / 恢复
TEST(PatrolStateMachineTest, SafetyHoldAndClear)
{
  PatrolStateMachine sm(makeConfig(3, 1));
  sm.handle(PatrolEvent::START);
  const auto hold_cmd = sm.handle(PatrolEvent::SAFETY_TRIGGERED);
  EXPECT_EQ(sm.state(), PatrolState::SAFETY_HOLD);
  EXPECT_TRUE(hold_cmd.cancel_goal);

  const auto clear_cmd = sm.handle(PatrolEvent::SAFETY_CLEARED);
  EXPECT_EQ(sm.state(), PatrolState::MOVING);
  EXPECT_TRUE(clear_cmd.send_goal);
  EXPECT_EQ(clear_cmd.target_index, 0u);
}

// TC-U-04g STOP 归零
TEST(PatrolStateMachineTest, StopResetsIndex)
{
  PatrolStateMachine sm(makeConfig(3, 1));
  sm.handle(PatrolEvent::START);
  const auto cmd = sm.handle(PatrolEvent::STOP);
  EXPECT_EQ(sm.state(), PatrolState::IDLE);
  EXPECT_EQ(sm.waypointIndex(), 0u);
  EXPECT_TRUE(cmd.cancel_goal);
}

// TC-U-04h 空航点 START
TEST(PatrolStateMachineTest, RejectsStartWithoutWaypoints)
{
  PatrolStateMachine sm(makeConfig(0, 1));
  const auto cmd = sm.handle(PatrolEvent::START);
  EXPECT_EQ(sm.state(), PatrolState::IDLE);
  EXPECT_FALSE(cmd.send_goal);
  EXPECT_EQ(sm.errorCode(), 1u);
}

// TC-U-04i 幂等 PAUSE（IDLE 下无效）
TEST(PatrolStateMachineTest, PauseFromIdleIsNoop)
{
  PatrolStateMachine sm(makeConfig(3, 1));
  const auto cmd = sm.handle(PatrolEvent::PAUSE);
  EXPECT_EQ(sm.state(), PatrolState::IDLE);
  EXPECT_FALSE(cmd.send_goal);
  EXPECT_FALSE(cmd.cancel_goal);
}

// TC-U-04j 非法事件（IDLE + GOAL_SUCCEEDED）
TEST(PatrolStateMachineTest, UnexpectedEventIsNoop)
{
  PatrolStateMachine sm(makeConfig(3, 1));
  const auto cmd = sm.handle(PatrolEvent::GOAL_SUCCEEDED);
  EXPECT_EQ(sm.state(), PatrolState::IDLE);
  EXPECT_EQ(sm.completedWaypoints(), 0u);
  EXPECT_FALSE(cmd.send_goal);
}

// TC-U-04k RESET 清统计
TEST(PatrolStateMachineTest, ResetClearsStatistics)
{
  PatrolStateMachine sm(makeConfig(3, 1));
  sm.handle(PatrolEvent::START);
  for (int i = 0; i < 3; ++i) {
    sm.handle(PatrolEvent::GOAL_SUCCEEDED);
    sm.handle(PatrolEvent::WAIT_ELAPSED);
  }
  ASSERT_EQ(sm.loopCount(), 1u);
  sm.handle(PatrolEvent::RESET);
  EXPECT_EQ(sm.completedWaypoints(), 0u);
  EXPECT_EQ(sm.loopCount(), 0u);
  EXPECT_EQ(sm.errorCode(), 0u);
}

// TC-U-04l FAILED 后 RESET
TEST(PatrolStateMachineTest, ResetFromFailedReturnsIdle)
{
  PatrolStateMachine sm(makeConfig(1, 1, 1, false));
  sm.handle(PatrolEvent::START);
  sm.handle(PatrolEvent::GOAL_FAILED);
  ASSERT_EQ(sm.state(), PatrolState::FAILED);
  sm.handle(PatrolEvent::RESET);
  EXPECT_EQ(sm.state(), PatrolState::IDLE);
  EXPECT_EQ(sm.waypointIndex(), 0u);
  EXPECT_EQ(sm.errorCode(), 0u);
}

// 附加：停留时间读取
TEST(PatrolStateMachineTest, CurrentWaitSecFollowsIndex)
{
  PatrolStateMachine::Config cfg = makeConfig(3, 1);
  cfg.wait_table = {1.0, 2.0, 3.0};
  PatrolStateMachine sm(cfg);
  sm.handle(PatrolEvent::START);
  EXPECT_DOUBLE_EQ(sm.currentWaitSec(), 1.0);
  sm.handle(PatrolEvent::GOAL_SUCCEEDED);
  sm.handle(PatrolEvent::WAIT_ELAPSED);
  EXPECT_DOUBLE_EQ(sm.currentWaitSec(), 2.0);
}
