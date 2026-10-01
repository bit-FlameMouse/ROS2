// 文件用途：巡逻任务主节点
//   职责：① 加载航点参数 ② 100ms tick 驱动状态机 ③ 调用 Nav2 导航
//         ④ 发布状态与可视化 ⑤ 提供模式控制服务 ⑥ 响应安全守护
//   并发模型（16 文档 AS-54）：状态机只在 tick 线程中修改；
//   Action 回调只写 NavClient 内部状态；服务请求经 tick 串行处理。
// 版权：2026 patrol_robot developer，Apache-2.0 许可（许可证原文见仓库根目录 LICENSE 文件）
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include "patrol_interfaces/msg/patrol_status.hpp"
#include "patrol_interfaces/srv/set_patrol_mode.hpp"
#include "patrol_robot_core/nav_client.hpp"
#include "patrol_robot_core/patrol_state_machine.hpp"
#include "patrol_robot_core/waypoint_loader.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/bool.hpp"
#include "visualization_msgs/msg/marker.hpp"
#include "visualization_msgs/msg/marker_array.hpp"

namespace patrol_robot_core
{
using namespace std::chrono_literals;  // NOLINT(build/namespaces)

class PatrolNode : public rclcpp::Node
{
public:
  PatrolNode()
  : rclcpp::Node("patrol_node")
  {
    declareParameters();
    validateScalarParameters();
    loadWaypoints();
    buildStateMachine();   // 依赖就绪等待：Nav2 不可用则 FATAL 退出（AS-62）
    createInterfaces();

    RCLCPP_INFO(get_logger(), "patrol_node 已启动，tick=%d ms", tick_period_ms_);
    if (waypoints_valid_) {
      RCLCPP_INFO(get_logger(), "%s", WaypointLoader::describe(waypoints_).c_str());
    } else {
      RCLCPP_WARN(get_logger(), "节点以无航点模式运行，START 服务将返回 NO_WAYPOINTS");
    }

    if (auto_start_) {
      RCLCPP_INFO(get_logger(), "auto_start=true，自动开始巡逻");
      // 只置请求标志，真正的迁移由首次 tick 完成（AS-54 单点更新）
      enqueueModeEvent(PatrolEvent::START);
    }
  }

private:
  /// @brief 状态快照：用于跨线程读取状态机与错误信息（由 sm_mutex_ 保护）
  struct Snapshot
  {
    PatrolState state{PatrolState::IDLE};
    uint32_t index{0};
    uint32_t loop{0};
    uint32_t completed{0};
    int32_t retry{0};
    bool finished_all{false};
    double current_wait_sec{0.0};
    uint16_t sm_error_code{0};
    std::string sm_error_message;
    uint16_t node_error_code{0};
    std::string node_error_message;
  };

  /// @brief 模式请求的处理结果（由 req_mutex_ 保护）
  struct ModeRequestResult
  {
    bool success{false};
    std::string message;
    uint8_t mode{0};
  };

  // ---------------- 参数声明 ----------------
  void declareParameters()
  {
    waypoint_x_ = declare_parameter<std::vector<double>>("waypoint_x", std::vector<double>{});
    waypoint_y_ = declare_parameter<std::vector<double>>("waypoint_y", std::vector<double>{});
    waypoint_yaw_ = declare_parameter<std::vector<double>>("waypoint_yaw", std::vector<double>{});
    waypoint_wait_ = declare_parameter<std::vector<double>>("waypoint_wait", std::vector<double>{});

    loop_count_ = declare_parameter<int>("loop_count", 0);
    max_retries_ = declare_parameter<int>("max_retries", 2);
    skip_failed_waypoint_ = declare_parameter<bool>("skip_failed_waypoint", true);
    waypoint_timeout_sec_ = declare_parameter<double>("waypoint_timeout_sec", 60.0);
    auto_start_ = declare_parameter<bool>("auto_start", false);

    global_frame_ = declare_parameter<std::string>("global_frame", "map");
    action_name_ = declare_parameter<std::string>("action_name", "navigate_to_pose");
    status_topic_ = declare_parameter<std::string>("status_topic", "/patrol/status");
    markers_topic_ = declare_parameter<std::string>("markers_topic", "/patrol/markers");
    mode_service_ = declare_parameter<std::string>("mode_service", "/patrol/set_mode");
    safety_topic_ = declare_parameter<std::string>("safety_topic", "/safety/emergency_stop");
    use_safety_hold_ = declare_parameter<bool>("use_safety_hold", true);

    tick_period_ms_ = declare_parameter<int>("tick_period_ms", 100);
    status_publish_hz_ = declare_parameter<double>("status_publish_hz", 1.0);
  }

  // ---------------- 标量参数范围校验（AS-49 / AS-78：越界即拒绝启动） ----------------
  void validateScalarParameters()
  {
    std::string error;
    if (tick_period_ms_ < 10 || tick_period_ms_ > 1000) {
      error = "tick_period_ms 必须在 [10, 1000] 范围内，实际为 " + std::to_string(tick_period_ms_);
    } else if (status_publish_hz_ < 0.1 || status_publish_hz_ > 20.0) {
      error = "status_publish_hz 必须在 [0.1, 20] 范围内，实际为 " +
        std::to_string(status_publish_hz_);
    } else if (max_retries_ < 1) {
      error = "max_retries 必须 >= 1，实际为 " + std::to_string(max_retries_);
    } else if (loop_count_ < 0) {
      error = "loop_count 必须 >= 0（0 表示无限循环），实际为 " + std::to_string(loop_count_);
    } else if (waypoint_timeout_sec_ < 0.0) {
      error = "waypoint_timeout_sec 必须 >= 0（0 表示不限制），实际为 " +
        std::to_string(waypoint_timeout_sec_);
    }

    if (!error.empty()) {
      RCLCPP_FATAL(get_logger(), "参数校验失败：%s（拒绝启动）", error.c_str());
      throw std::invalid_argument(error);
    }
  }

  // ---------------- 航点加载 ----------------
  void loadWaypoints()
  {
    std::string error;
    waypoints_valid_ = WaypointLoader::fromArrays(
      waypoint_x_, waypoint_y_, waypoint_yaw_, waypoint_wait_,
      global_frame_, waypoints_, error);
    if (!waypoints_valid_) {
      // 设计取舍（04 §3.6 / 05 §5.1）：航点问题不阻塞节点启动，
      // START 服务返回 NO_WAYPOINTS，用户修正 YAML 后重启即可
      RCLCPP_ERROR(get_logger(), "航点参数校验失败：%s", error.c_str());
    }
  }

  // ---------------- 状态机与导航客户端 ----------------
  void buildStateMachine()
  {
    PatrolStateMachine::Config cfg;
    cfg.waypoint_count = static_cast<uint32_t>(waypoints_.waypoints.size());
    cfg.loop_count = static_cast<uint32_t>(loop_count_);
    cfg.max_retries = static_cast<uint32_t>(max_retries_);
    cfg.skip_failed_waypoint = skip_failed_waypoint_;
    cfg.wait_table = waypoint_wait_;
    sm_ = std::make_unique<PatrolStateMachine>(cfg);

    nav_ = std::make_unique<NavClient>(this, action_name_, global_frame_);
    if (!nav_->waitForServer(10s)) {
      RCLCPP_FATAL(
        get_logger(), "等待 Nav2 Action Server '%s' 超时（10s），请确认 Nav2 已启动后再重启本节点",
        action_name_.c_str());
      throw std::runtime_error("Nav2 Action Server 不可用");
    }
    RCLCPP_INFO(get_logger(), "Nav2 Action Server '%s' 已就绪", action_name_.c_str());
  }

  // ---------------- 接口创建 ----------------
  void createInterfaces()
  {
    // 回调组划分（防止服务等待 tick 时互相饿死；见 16 文档 AS-52/AS-58）
    timer_callback_group_ = create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
    mode_callback_group_ = create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
    safety_callback_group_ = create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);

    // /patrol/markers：晚启动的 RViz 也要看到航点 → TRANSIENT_LOCAL + KEEP_LAST(1)
    rclcpp::QoS marker_qos(1);
    marker_qos.transient_local();
    marker_pub_ = create_publisher<visualization_msgs::msg::MarkerArray>(
      markers_topic_, marker_qos);

    // /patrol/status：周期状态，丢一帧可接受 → RELIABLE + VOLATILE + KEEP_LAST(10)
    rclcpp::QoS status_qos(10);
    status_pub_ = create_publisher<patrol_interfaces::msg::PatrolStatus>(status_topic_, status_qos);

    // /safety/emergency_stop：状态标志必须可靠且可回溯 → TRANSIENT_LOCAL + KEEP_LAST(1)
    rclcpp::SubscriptionOptions safety_options;
    safety_options.callback_group = safety_callback_group_;
    rclcpp::QoS safety_qos(1);
    safety_qos.transient_local();
    safety_sub_ = create_subscription<std_msgs::msg::Bool>(
      safety_topic_, safety_qos,
      std::bind(&PatrolNode::onSafetyEmergency, this, std::placeholders::_1),
      safety_options);

    mode_srv_ = create_service<patrol_interfaces::srv::SetPatrolMode>(
      mode_service_,
      std::bind(
        &PatrolNode::onSetMode, this,
        std::placeholders::_1, std::placeholders::_2),
      rmw_qos_profile_services_default,
      mode_callback_group_);

    // tick 与状态发布使用墙钟定时器：即使仿真暂停也能驱动任务层；
    // 业务计时（超时/停留）一律使用 this->now()（AS-37/AS-38）
    tick_timer_ = create_wall_timer(
      std::chrono::milliseconds(tick_period_ms_),
      std::bind(&PatrolNode::tick, this),
      timer_callback_group_);

    const double period_sec = 1.0 / std::max(0.1, status_publish_hz_);
    status_timer_ = create_wall_timer(
      std::chrono::milliseconds(static_cast<int>(period_sec * 1000.0)),
      std::bind(&PatrolNode::publishStatus, this),
      timer_callback_group_);

    publishMarkers();
  }

  // ---------------- 安全守护回调（只置原子标志，不做状态迁移） ----------------
  void onSafetyEmergency(const std_msgs::msg::Bool::SharedPtr msg)
  {
    if (!use_safety_hold_) {
      return;
    }
    const bool previous = emergency_.exchange(msg->data);
    if (previous != msg->data) {
      RCLCPP_WARN(get_logger(), "安全标志变化: emergency=%s", msg->data ? "true" : "false");
    }
  }

  // ---------------- 模式控制服务 ----------------
  void onSetMode(
    const std::shared_ptr<patrol_interfaces::srv::SetPatrolMode::Request> request,
    std::shared_ptr<patrol_interfaces::srv::SetPatrolMode::Response> response)
  {
    using Req = patrol_interfaces::srv::SetPatrolMode::Request;

    PatrolEvent event{};
    bool valid = true;

    switch (request->mode) {
      case Req::REQ_START: event = PatrolEvent::START; break;
      case Req::REQ_PAUSE: event = PatrolEvent::PAUSE; break;
      case Req::REQ_RESUME: event = PatrolEvent::RESUME; break;
      case Req::REQ_STOP: event = PatrolEvent::STOP; break;
      case Req::REQ_RESET: event = PatrolEvent::RESET; break;
      default: valid = false; break;
    }

    if (!valid) {
      response->success = false;
      response->message = "未知的模式取值: " + std::to_string(request->mode);
      response->current_mode = toModeConstant(snapshot().state);
      return;
    }

    if (event == PatrolEvent::START && !waypoints_valid_) {
      response->success = false;
      response->message = "NO_WAYPOINTS";
      response->current_mode = toModeConstant(snapshot().state);
      RCLCPP_ERROR(get_logger(), "收到 START 请求但航点无效，已拒绝");
      return;
    }

    const uint64_t seq = enqueueModeEvent(event);

    std::unique_lock<std::mutex> lock(req_mutex_);
    // 有界等待（≤1s）tick 处理模式请求：不等 Action 结果、不持锁阻塞 tick
    // （tick 位于独立回调组）；见 16 §17.1 例外条款。
    const bool done = req_cv_.wait_for(
      // AS-EXEMPT(AS-53): 计划 v0.3 评估替代方案
      lock, std::chrono::milliseconds(kModeRequestTimeoutMs),
      [this, seq]() {return req_done_seq_ >= seq;});
    if (!done) {
      response->success = false;
      response->message = "TIMEOUT";
      response->current_mode = toModeConstant(snapshot().state);
      RCLCPP_ERROR(
        get_logger(), "模式请求处理超时（tick 未在 %d ms 内响应）", kModeRequestTimeoutMs);
      return;
    }

    response->success = req_result_.success;
    response->message = req_result_.message;
    response->current_mode = req_result_.mode;
  }

  /// @brief 把模式事件放入待处理槽（由 tick 消费），返回请求序号
  uint64_t enqueueModeEvent(PatrolEvent event)
  {
    std::lock_guard<std::mutex> lock(req_mutex_);
    req_event_ = event;
    req_pending_ = true;
    return ++req_seq_;
  }

  /// @brief 在 tick 中处理挂起的模式请求（单点更新，AS-54）
  void processPendingModeRequest()
  {
    PatrolEvent event{};
    uint64_t seq = 0;
    {
      std::lock_guard<std::mutex> lock(req_mutex_);
      if (!req_pending_) {
        return;
      }
      event = req_event_;
      seq = req_seq_;
      req_pending_ = false;
    }

    const auto before = snapshot();
    applyCommand(handleEvent(event));
    if (event == PatrolEvent::RESET) {
      clearNodeError();  // FR-07：RESET 清零统计与错误
    }
    const auto after = snapshot();

    const bool changed = (after.state != before.state);
    bool success = changed || (event == PatrolEvent::RESET);
    std::string message = success ? "OK" : "NO_STATE_CHANGE";

    // START 因依赖不可用而直接失败时，明确返回失败原因（FR-07 错误语义）
    if (success && event == PatrolEvent::START && after.state == PatrolState::FAILED) {
      success = false;
      message = (after.node_error_code ==
        patrol_interfaces::msg::PatrolStatus::ERR_NAV_SERVER_UNAVAILABLE) ?
        "NAV_SERVER_UNAVAILABLE" : "NAV_FAILED";
    }

    {
      std::lock_guard<std::mutex> lock(req_mutex_);
      req_result_.success = success;
      req_result_.message = message;
      req_result_.mode = toModeConstant(after.state);
      req_done_seq_ = seq;
    }
    req_cv_.notify_all();

    RCLCPP_INFO(
      get_logger(), "模式请求 %s: %s → %s (%s)",
      toString(event), toString(before.state), toString(after.state), message.c_str());
  }

  // ---------------- 主 tick ----------------
  void tick()
  {
    processPendingModeRequest();

    const auto snap = snapshot();

    if (snap.state == PatrolState::IDLE || snap.state == PatrolState::PAUSED ||
      snap.state == PatrolState::FAILED)
    {
      return;
    }

    if (snap.state == PatrolState::SAFETY_HOLD) {
      if (!emergency_.load()) {
        applyCommand(handleEvent(PatrolEvent::SAFETY_CLEARED));
      }
      return;
    }

    if (emergency_.load() && use_safety_hold_) {
      // 安全优先级最高：立即取消当前导航并进入 SAFETY_HOLD
      applyCommand(handleEvent(PatrolEvent::SAFETY_TRIGGERED));
      return;
    }

    if (snap.state == PatrolState::MOVING) {
      const double elapsed = elapsedSince(goal_start_);
      if (waypoint_timeout_sec_ > 0.0 && elapsed > waypoint_timeout_sec_) {
        RCLCPP_WARN(
          get_logger(), "航点 %u 导航超时（%.1fs > %.1fs），触发重试",
          snap.index, elapsed, waypoint_timeout_sec_);
        setNodeError(
          patrol_interfaces::msg::PatrolStatus::ERR_WAYPOINT_TIMEOUT, "航点导航超时");
        cancel_expected_ = true;
        nav_->cancel();
        applyCommand(handleEvent(PatrolEvent::GOAL_FAILED));
        return;
      }

      switch (nav_->state()) {
        case NavTaskState::SUCCEEDED:
          applyCommand(handleEvent(PatrolEvent::GOAL_SUCCEEDED));
          wait_start_ = now();
          break;
        case NavTaskState::ABORTED:
          applyCommand(handleEvent(PatrolEvent::GOAL_FAILED));
          break;
        case NavTaskState::REJECTED:
          // FR-07 / 05 §6：目标被拒绝映射为 ERR_GOAL_REJECTED
          setNodeError(
            patrol_interfaces::msg::PatrolStatus::ERR_GOAL_REJECTED, "目标被 Nav2 拒绝");
          applyCommand(handleEvent(PatrolEvent::GOAL_FAILED));
          break;
        case NavTaskState::CANCELED:
          if (!cancel_expected_) {
            RCLCPP_WARN(get_logger(), "导航目标被外部取消，按失败处理");
            applyCommand(handleEvent(PatrolEvent::GOAL_FAILED));
          }
          break;
        default:
          break;
      }
      return;
    }

    if (snap.state == PatrolState::WAITING) {
      const double waited = elapsedSince(wait_start_);
      if (waited >= snap.current_wait_sec) {
        applyCommand(handleEvent(PatrolEvent::WAIT_ELAPSED));
        const auto post = snapshot();
        if (post.finished_all) {
          RCLCPP_INFO(
            get_logger(), "巡逻任务全部完成：共 %u 个航点，%u 轮",
            post.completed, post.loop);
        }
      }
    }
  }

  // ---------------- 副作用执行（仅 tick 线程调用） ----------------
  void applyCommand(const SmCommand & cmd)
  {
    if (cmd.cancel_goal) {
      cancel_expected_ = true;
      nav_->cancel();
    }

    if (cmd.restart_wait) {
      // 从暂停/安全暂停恢复到 WAITING：重新开始停留计时
      wait_start_ = now();
    }

    if (cmd.send_goal) {
      const auto & wps = waypoints_.waypoints;  // 构造后只读
      if (cmd.target_index >= wps.size()) {
        RCLCPP_ERROR(get_logger(), "目标索引越界: %u (共 %zu)", cmd.target_index, wps.size());
        return;
      }
      const auto & wp = wps[cmd.target_index];

      nav_->detach();  // 清掉上一轮结果，避免误判（04 §12 K-16）
      const bool ok = nav_->sendGoal(wp.x, wp.y, wp.yaw);
      goal_start_ = now();
      cancel_expected_ = false;

      if (!ok) {
        setNodeError(
          patrol_interfaces::msg::PatrolStatus::ERR_NAV_SERVER_UNAVAILABLE,
          "Nav2 Action Server 不可用");
        // 递归深度受 max_retries 上限保护（04 §6.4），不会栈溢出
        applyCommand(handleEvent(PatrolEvent::GOAL_FAILED));
        return;
      }
      RCLCPP_INFO(
        get_logger(), "前往航点 [%u] x=%.2f y=%.2f yaw=%.2f (wait=%.1fs)",
        cmd.target_index, wp.x, wp.y, wp.yaw, wp.wait_sec);
    }

    publishMarkers();
  }

  // ---------------- 状态发布 ----------------
  void publishStatus()
  {
    const auto snap = snapshot();

    patrol_interfaces::msg::PatrolStatus msg;
    msg.header.stamp = now();
    msg.header.frame_id = global_frame_;
    msg.mode = toModeConstant(snap.state);
    msg.current_waypoint_index = snap.index;
    msg.total_waypoints = static_cast<uint32_t>(waypoints_.waypoints.size());
    msg.completed_waypoints = snap.completed;
    msg.current_loop = snap.loop;
    msg.retry_count = snap.retry;
    msg.distance_remaining = static_cast<float>(nav_->distanceRemaining());

    // 错误展示优先级：状态机错误（如重试超限）优先于节点级错误（如超时）
    if (snap.sm_error_code != 0) {
      msg.error_code = snap.sm_error_code;
      msg.error_message = snap.sm_error_message;
    } else if (snap.node_error_code != 0) {
      msg.error_code = snap.node_error_code;
      msg.error_message = snap.node_error_message;
    } else {
      msg.error_code = patrol_interfaces::msg::PatrolStatus::ERR_NONE;
      msg.error_message.clear();
    }
    status_pub_->publish(msg);
  }

  // ---------------- 可视化 ----------------
  void publishMarkers()
  {
    using visualization_msgs::msg::Marker;

    visualization_msgs::msg::MarkerArray arr;
    const auto stamp = now();
    const auto snap = snapshot();

    // 先清理，避免航点数量减少时残留旧 Marker
    Marker clear;
    clear.header.frame_id = global_frame_;
    clear.header.stamp = stamp;
    clear.ns = "patrol_waypoints";
    clear.action = Marker::DELETEALL;
    arr.markers.push_back(clear);

    const uint32_t current = snap.index;
    const bool running = (snap.state == PatrolState::MOVING || snap.state == PatrolState::WAITING ||
      snap.state == PatrolState::PAUSED || snap.state == PatrolState::SAFETY_HOLD);
    const bool all_finished = snap.finished_all && snap.state == PatrolState::IDLE;

    for (const auto & wp : waypoints_.waypoints) {
      Marker m;
      m.header.frame_id = global_frame_;
      m.header.stamp = stamp;
      m.ns = "patrol_waypoints";
      m.id = static_cast<int>(wp.index);
      m.type = Marker::SPHERE;
      m.action = Marker::ADD;
      m.pose.position.x = wp.x;
      m.pose.position.y = wp.y;
      m.pose.position.z = 0.15;
      m.pose.orientation.w = 1.0;  // 必须显式初始化（04 §8 关键设计点 5）
      m.scale.x = m.scale.y = m.scale.z = 0.25;
      m.lifetime = rclcpp::Duration(0, 0);  // 0 = 永不过期

      if (all_finished) {
        m.color.r = 0.20f; m.color.g = 0.85f; m.color.b = 0.30f;  // 绿：全部完成
      } else if (running && wp.index == current) {
        m.color.r = 1.00f; m.color.g = 0.85f; m.color.b = 0.00f;  // 黄：当前目标
      } else if (running && wp.index < current) {
        m.color.r = 0.20f; m.color.g = 0.85f; m.color.b = 0.30f;  // 绿：本轮已完成
      } else {
        m.color.r = 0.60f; m.color.g = 0.60f; m.color.b = 0.60f;  // 灰：未访问
      }
      m.color.a = 0.9f;
      arr.markers.push_back(m);

      Marker label = m;
      label.ns = "patrol_labels";     // 必须换 ns，否则 ID 与球体冲突（K-13）
      label.type = Marker::TEXT_VIEW_FACING;
      label.scale.z = 0.22;
      label.pose.position.z = 0.45;
      label.color.r = label.color.g = label.color.b = 1.0f;
      label.color.a = 0.95f;
      label.text = "WP" + std::to_string(wp.index);
      arr.markers.push_back(label);
    }

    marker_pub_->publish(arr);
  }

  // ---------------- 工具 ----------------
  /// @brief 状态枚举转 PatrolStatus 的 MODE_* 常量（数值一一对应）
  static uint8_t toModeConstant(PatrolState s)
  {
    return static_cast<uint8_t>(s);
  }

  /// @brief 读取状态快照（sm_mutex_ 保护，可在任意回调组调用）
  Snapshot snapshot() const
  {
    std::lock_guard<std::mutex> lock(sm_mutex_);
    Snapshot snap;
    snap.state = sm_->state();
    snap.index = sm_->waypointIndex();
    snap.loop = sm_->loopCount();
    snap.completed = sm_->completedWaypoints();
    snap.retry = sm_->retryCount();
    snap.finished_all = sm_->finishedAllLoops();
    snap.current_wait_sec = sm_->currentWaitSec();
    snap.sm_error_code = sm_->errorCode();
    snap.sm_error_message = sm_->errorMessage();
    snap.node_error_code = node_error_code_;
    snap.node_error_message = node_error_message_;
    return snap;
  }

  /// @brief 调用状态机（sm_mutex_ 保护；只允许 tick 线程调用）
  SmCommand handleEvent(PatrolEvent event)
  {
    std::lock_guard<std::mutex> lock(sm_mutex_);
    return sm_->handle(event);
  }

  /// @brief 记录节点级错误（sm_mutex_ 保护）
  void setNodeError(uint16_t code, const std::string & message)
  {
    std::lock_guard<std::mutex> lock(sm_mutex_);
    node_error_code_ = code;
    node_error_message_ = message;
  }

  /// @brief 清除节点级错误（sm_mutex_ 保护）
  void clearNodeError()
  {
    std::lock_guard<std::mutex> lock(sm_mutex_);
    node_error_code_ = 0;
    node_error_message_.clear();
  }

  /// @brief 计算自 start 起经过的仿真时间（s）
  /// @details AS-39 防御：/clock 未发布时 now() 为 0，或时钟回退时，
  ///          把起点重置为当前时刻并返回 0，避免"瞬间超时"。
  double elapsedSince(rclcpp::Time & start)
  {
    const rclcpp::Time current = now();
    if (current.nanoseconds() == 0) {
      return 0.0;
    }
    if (start.nanoseconds() == 0 || current < start) {
      start = current;
      return 0.0;
    }
    return (current - start).seconds();
  }

  // ---------------- 参数缓存（构造后只读；默认值只在 declare_parameters 中出现） ----------------
  std::vector<double> waypoint_x_, waypoint_y_, waypoint_yaw_, waypoint_wait_;
  int loop_count_{0};
  int max_retries_{2};
  bool skip_failed_waypoint_{true};
  double waypoint_timeout_sec_{60.0};
  bool auto_start_{false};
  std::string global_frame_;
  std::string action_name_;
  std::string status_topic_;
  std::string markers_topic_;
  std::string mode_service_;
  std::string safety_topic_;
  int tick_period_ms_{100};
  double status_publish_hz_{1.0};
  bool use_safety_hold_{true};

  // ---------------- 运行时状态 ----------------
  WaypointSet waypoints_;                 ///< 构造后只读
  bool waypoints_valid_{false};           ///< 构造后只读
  std::unique_ptr<PatrolStateMachine> sm_;  ///< 由 sm_mutex_ 保护
  std::unique_ptr<NavClient> nav_;          ///< NavClient 内部自带互斥保护

  rclcpp::Publisher<patrol_interfaces::msg::PatrolStatus>::SharedPtr status_pub_;
  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr marker_pub_;
  rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr safety_sub_;
  rclcpp::Service<patrol_interfaces::srv::SetPatrolMode>::SharedPtr mode_srv_;
  rclcpp::TimerBase::SharedPtr tick_timer_;
  rclcpp::TimerBase::SharedPtr status_timer_;
  rclcpp::CallbackGroup::SharedPtr timer_callback_group_;
  rclcpp::CallbackGroup::SharedPtr mode_callback_group_;
  rclcpp::CallbackGroup::SharedPtr safety_callback_group_;

  std::atomic<bool> emergency_{false};    ///< 安全标志（订阅回调写，tick 读）
  bool cancel_expected_{false};           ///< 仅 tick 线程访问
  rclcpp::Time wait_start_{0, 0, RCL_ROS_TIME};  ///< 仅 tick 线程访问
  rclcpp::Time goal_start_{0, 0, RCL_ROS_TIME};  ///< 仅 tick 线程访问

  mutable std::mutex sm_mutex_;           ///< 保护 sm_ / node_error_*
  uint16_t node_error_code_{0};           ///< 由 sm_mutex_ 保护
  std::string node_error_message_;        ///< 由 sm_mutex_ 保护

  std::mutex req_mutex_;                  ///< 保护以下模式请求槽（tick 消费，服务等待）
  std::condition_variable req_cv_;
  bool req_pending_{false};
  PatrolEvent req_event_{PatrolEvent::START};
  uint64_t req_seq_{0};
  uint64_t req_done_seq_{0};
  ModeRequestResult req_result_;

  static constexpr int kModeRequestTimeoutMs = 1000;  ///< 服务等待 tick 的上限（ms）
};

}  // namespace patrol_robot_core

// ---------------- 入口：使用多线程执行器 ----------------
// 理由：服务回调、Action 回调与定时器需要并发执行；单线程执行器下
//       任一回调阻塞会拖垮整个节点（04 §12 K-02，16 文档 AS-52）。
int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  try {
    auto node = std::make_shared<patrol_robot_core::PatrolNode>();
    rclcpp::executors::MultiThreadedExecutor executor(rclcpp::ExecutorOptions(), 2);
    executor.add_node(node);
    executor.spin();
  } catch (const std::exception & e) {
    RCLCPP_FATAL(rclcpp::get_logger("patrol_node"), "启动失败: %s", e.what());
    rclcpp::shutdown();
    return 1;
  }
  rclcpp::shutdown();
  return 0;
}
