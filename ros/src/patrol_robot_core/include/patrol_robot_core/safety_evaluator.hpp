// 文件用途：激光安全判定（纯逻辑，不依赖 ROS 运行时，可独立单元测试）
// 说明：为满足 AS-65（纯逻辑类不得 include 任何 ROS 头文件），
//       本类不直接接收 sensor_msgs/LaserScan，而是通过 LaserScanView
//       接收从消息中抽取的必要字段（依赖注入，见 AS-66）。
// 版权：2026 patrol_robot developer，Apache-2.0 许可（许可证原文见仓库根目录 LICENSE 文件）
#ifndef PATROL_ROBOT_CORE__SAFETY_EVALUATOR_HPP_
#define PATROL_ROBOT_CORE__SAFETY_EVALUATOR_HPP_

#include <limits>
#include <string>
#include <vector>

namespace patrol_robot_core
{

/// @brief 从激光消息中抽取的输入视图（角度单位：rad，距离单位：m）
struct LaserScanView
{
  const std::vector<float> * ranges{nullptr};  ///< 距离数组（可能含 NaN/Inf）
  double angle_min{0.0};        ///< 第一个点的角度
  double angle_increment{0.0};  ///< 相邻点角度增量
  double range_min{0.0};        ///< 传感器有效量程下限
  double range_max{0.0};        ///< 传感器有效量程上限
};

/// @brief 一次安全评估的结果
struct SafetyResult
{
  double min_distance{std::numeric_limits<double>::infinity()};  ///< 扇区内最小距离
  bool emergency{false};        ///< 评估后的紧急标志
  bool state_changed{false};    ///< 与上次相比是否翻转
  bool has_valid_point{false};  ///< 扇区内是否存在有效点
};

/// @brief 前向扇区障碍检测 + 迟滞判定
class SafetyEvaluator
{
public:
  struct Config
  {
    double stop_distance{0.30};    ///< 触发紧急停车的距离阈值（m）
    double resume_distance{0.60};  ///< 解除紧急状态的距离阈值（m），必须 > stop_distance
    double fov_deg{60.0};          ///< 前向扇区总视场角（度）
    double range_min{0.02};        ///< 有效距离下限（m）
    double range_max{3.50};        ///< 有效距离上限（m）
  };

  /// @brief 构造函数；配置非法时抛 std::invalid_argument（显式失败，NFR-09）
  /// @param config 安全判定配置
  explicit SafetyEvaluator(const Config & config);

  /// @brief 校验配置合法性
  /// @param config 待校验配置
  /// @param[out] error 失败原因（成功时被清空）
  /// @return 配置合法返回 true
  static bool validateConfig(const Config & config, std::string & error);

  /// @brief 处理一帧激光数据
  /// @param scan 激光输入视图
  /// @return 本次评估结果（含迟滞后的紧急标志）
  SafetyResult update(const LaserScanView & scan);

  /// @brief 当前紧急标志
  bool emergency() const {return emergency_;}
  /// @brief 复位紧急标志（不改变配置）
  void reset() {emergency_ = false;}
  /// @brief 当前配置
  const Config & config() const {return config_;}

private:
  Config config_;
  bool emergency_{false};
};

}  // namespace patrol_robot_core

#endif  // PATROL_ROBOT_CORE__SAFETY_EVALUATOR_HPP_
