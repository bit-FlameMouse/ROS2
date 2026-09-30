// 文件用途：航点数据结构定义与参数解析（纯逻辑，不依赖 ROS 运行时）
// 版权：2026 patrol_robot developer，Apache-2.0 许可（许可证原文见仓库根目录 LICENSE 文件）
#ifndef PATROL_ROBOT_CORE__WAYPOINT_LOADER_HPP_
#define PATROL_ROBOT_CORE__WAYPOINT_LOADER_HPP_

#include <cstdint>
#include <string>
#include <vector>

namespace patrol_robot_core
{

/// @brief 单个航点：地图坐标系下的目标位姿与停留时间
struct Waypoint
{
  double x{0.0};         ///< 目标 x（m）
  double y{0.0};         ///< 目标 y（m）
  double yaw{0.0};       ///< 目标偏航角（rad），归一化到 (-pi, pi]
  double wait_sec{0.0};  ///< 到达后停留时间（s）
  uint32_t index{0};     ///< 序号，由加载器填充
};

/// @brief 一次巡逻任务的航点集合
struct WaypointSet
{
  std::vector<Waypoint> waypoints;
  std::string frame_id{"map"};
  std::string name{"default"};
};

/// @brief 航点加载器：纯函数式解析 + 校验
class WaypointLoader
{
public:
  /// @brief 由平行数组构造航点集合
  /// @param xs        x 坐标数组
  /// @param ys        y 坐标数组
  /// @param yaws      偏航角数组（rad）
  /// @param waits     停留时间数组（s）
  /// @param frame_id  参考坐标系
  /// @param[out] out   解析结果（失败时不保证有效）
  /// @param[out] error 失败原因（成功时被清空）
  /// @return 解析并校验成功返回 true
  static bool fromArrays(
    const std::vector<double> & xs,
    const std::vector<double> & ys,
    const std::vector<double> & yaws,
    const std::vector<double> & waits,
    const std::string & frame_id,
    WaypointSet & out,
    std::string & error);

  /// @brief 校验航点集合的合法性
  static bool validate(const WaypointSet & set, std::string & error);

  /// @brief 生成人类可读的描述（用于启动日志）
  static std::string describe(const WaypointSet & set);

  /// @brief 把角度归一化到 (-pi, pi]（pi 归一化后仍为 pi，见 06 文档 §7.2）
  static double normalizeAngle(double yaw);
};

}  // namespace patrol_robot_core

#endif  // PATROL_ROBOT_CORE__WAYPOINT_LOADER_HPP_
