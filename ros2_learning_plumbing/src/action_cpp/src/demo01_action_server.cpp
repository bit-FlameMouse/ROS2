// =====================================================================
// 动作服务端（Action Server）：提供 get_sum 动作，计算 1+2+...+num
// 动作 = 服务 + 连续反馈，适合"耗时任务"：
//   目标(Goal)：客户端发来的任务，例如 num=10
//   反馈(Feedback)：任务执行过程中不断汇报进度，例如 30%
//   结果(Result)：任务完成后返回最终结果，例如 sum=55
// 运行方式：ros2 run action_cpp demo01_action_server
// 配套程序：demo02_action_client
// =====================================================================

#include <memory>   // std::shared_ptr、std::make_shared
#include <thread>   // std::thread，用于另开线程执行耗时任务
#include <vector>   // std::vector，用于保存所有工作线程

#include "rclcpp/rclcpp.hpp"                            // ROS2 C++ 核心库
#include "rclcpp_action/rclcpp_action.hpp"              // 动作相关类（Server、GoalHandle 等）
#include "base_interfaces_demo/action/progress.hpp"     // 自定义动作接口 Progress（含 Goal/Feedback/Result）

using namespace std::placeholders;                      // 直接使用 _1、_2 等占位符
using base_interfaces_demo::action::Progress;           // 简化类型名
using GoalHandleProgress = rclcpp_action::ServerGoalHandle<Progress>; // 服务端管理某个目标的"句柄"，用它反馈进度/结束任务

// 3.定义节点类；
class MinimalActionServer : public rclcpp::Node
{
public:

  explicit MinimalActionServer(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : Node("minimal_action_server", options)              // 节点名 minimal_action_server
  {
    // 3-1.创建动作服务端；
    // 第 1 个参数 this 是节点，第 2 个 "get_sum" 是动作名（客户端必须一致），
    // 后面 3 个是回调函数：收到目标、收到取消请求、目标被接受后开始执行
    this->action_server_ = rclcpp_action::create_server<Progress>(
      this,
      "get_sum",
      std::bind(&MinimalActionServer::handle_goal, this, _1, _2),
      std::bind(&MinimalActionServer::handle_cancel, this, _1),
      std::bind(&MinimalActionServer::handle_accepted, this, _1));
    RCLCPP_INFO(this->get_logger(),"动作服务端创建，等待请求...");
  }

  // 析构函数：节点销毁前，先等待所有工作线程结束。
  // 否则按 Ctrl+C 退出时，工作线程可能还在调用已被销毁的动作服务端，导致程序崩溃。
  ~MinimalActionServer() override
  {
    for (auto & worker : worker_threads_) {
      if (worker.joinable()) {
        worker.join(); // 阻塞等待该线程执行完（退出时 rclcpp::ok() 为 false，任务会很快结束）
      }
    }
  }

private:
  rclcpp_action::Server<Progress>::SharedPtr action_server_; // 动作服务端对象
  std::vector<std::thread> worker_threads_;                  // 执行任务的子线程列表，构造时保存、析构时统一回收

  // 3-2.处理请求数据；客户端发来目标时调用，决定"接受"还是"拒绝"
  rclcpp_action::GoalResponse handle_goal(const rclcpp_action::GoalUUID & uuid,std::shared_ptr<const Progress::Goal> goal)
  {
    (void)uuid;                                          // uuid 是目标的唯一编号，这里用不到，加 (void) 避免编译告警
    RCLCPP_INFO(this->get_logger(), "接收到动作客户端请求，请求数字为 %ld", goal->num); // %ld 对应 int64_t
    if (goal->num < 1) {                                 // 非法目标（小于 1）就拒绝
      return rclcpp_action::GoalResponse::REJECT;
    }
    return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE; // 接受目标并开始执行
  }

  // 3-3.处理取消任务请求；客户端请求取消时调用
  rclcpp_action::CancelResponse handle_cancel(
    const std::shared_ptr<GoalHandleProgress> goal_handle)
  {
    (void)goal_handle;
    RCLCPP_INFO(this->get_logger(), "接收到任务取消请求");
    return rclcpp_action::CancelResponse::ACCEPT;        // 同意取消
  }

  // 真正执行任务的函数（耗时操作都在这里）
  void execute(const std::shared_ptr<GoalHandleProgress> goal_handle)
  {
    RCLCPP_INFO(this->get_logger(), "开始执行任务");
    rclcpp::Rate loop_rate(10.0);                        // 频率 10Hz：每次循环自动 sleep 0.1 秒
    const auto goal = goal_handle->get_goal();           // 取出客户端发来的目标
    auto feedback = std::make_shared<Progress::Feedback>(); // 创建反馈对象
    auto result = std::make_shared<Progress::Result>();     // 创建结果对象
    int64_t sum= 0;                                      // 累加和
    for (int i = 1; (i <= goal->num) && rclcpp::ok(); i++) { // 从 1 加到 num；rclcpp::ok() 保证退出程序时循环能停下
      sum += i;
      // Check if there is a cancel request
      if (goal_handle->is_canceling()) {                 // 如果客户端取消了任务
        result->sum = sum;                               // 返回当前累加结果
        goal_handle->canceled(result);                   // 告诉客户端任务已取消
        RCLCPP_INFO(this->get_logger(), "任务取消");
        return;
      }
      feedback->progress = (double_t)i / goal->num;      // 计算进度：已算的数 / 总数，例如 3/10 = 0.3
      goal_handle->publish_feedback(feedback);           // 把进度反馈给客户端
      RCLCPP_INFO(this->get_logger(), "连续反馈中，进度：%.2f", feedback->progress);

      loop_rate.sleep();                                 // 睡 0.1 秒，让进度看起来是连续变化的
    }

    if (rclcpp::ok()) {                                  // 正常算完：返回最终结果，任务成功结束
      result->sum = sum;
      goal_handle->succeed(result);
      RCLCPP_INFO(this->get_logger(), "任务完成！");
    } else {
      // 收到退出信号（Ctrl+C）导致任务被中断。
      // 必须显式把目标标记为"中止"（abort）：abort() 内部先切换目标状态，
      // 再发布结果；状态一旦变成"中止"，工作线程结束时析构 goal_handle 就不会再
      // 自动尝试取消并发布结果（那一步会访问已在关闭中的服务端，导致进程崩溃）。
      // 但此时 ROS2 通信正在关闭，发布结果本身可能失败并抛异常，用 try/catch 兜底。
      result->sum = sum;
      try {
        goal_handle->abort(result);
        RCLCPP_INFO(this->get_logger(), "任务被中断，已中止");
      } catch (const std::exception & e) {
        RCLCPP_WARN(this->get_logger(), "任务被中断，通知客户端失败：%s", e.what());
      }
    }
  }

  // 3-4.生成连续反馈。
  // 目标被接受后调用：另开一个线程去执行 execute，并把线程保存到 worker_threads_。
  // 这样耗时计算不会卡住主线程的 spin（否则接收不到新的请求/取消指令）。
  // 注意不要用 detach()：线程脱离管理后，可能在节点销毁时还在访问服务端，导致崩溃。
  void handle_accepted(const std::shared_ptr<GoalHandleProgress> goal_handle)
  {
    worker_threads_.emplace_back(std::bind(&MinimalActionServer::execute, this, _1), goal_handle);
  }
}; 

int main(int argc, char ** argv)
{
  // 2.初始化 ROS2 客户端；
  rclcpp::init(argc, argv);
  // 4.调用spin函数，并传入节点对象指针；进入事件循环，等待动作请求。
  auto action_server = std::make_shared<MinimalActionServer>();
  rclcpp::spin(action_server);
  // 5.释放资源。
  rclcpp::shutdown();
  return 0;
}
