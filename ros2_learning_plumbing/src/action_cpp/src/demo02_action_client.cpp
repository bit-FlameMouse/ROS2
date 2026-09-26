// =====================================================================
// 动作客户端（Action Client）：向 get_sum 动作发送目标 num=10，等待结果
// 一次完整的动作交互有 3 个回调：
//   1. goal_response_callback：服务端是否接受了目标
//   2. feedback_callback    ：执行过程中的连续进度（0%~100%）
//   3. result_callback      ：最终结果（1+2+...+10=55）
// 运行方式：ros2 run action_cpp demo02_action_client
// 配套程序：demo01_action_server（需要先启动动作服务端）
// =====================================================================

#include "rclcpp/rclcpp.hpp"                        // ROS2 C++ 核心库
#include "rclcpp_action/rclcpp_action.hpp"          // 动作相关类（Client、ClientGoalHandle 等）
#include "base_interfaces_demo/action/progress.hpp" // 自定义动作接口 Progress

using base_interfaces_demo::action::Progress;                         // 简化类型名
using GoalHandleProgress = rclcpp_action::ClientGoalHandle<Progress>; // 客户端管理某个目标的"句柄"
using namespace std::placeholders;                                    // 直接使用 _1、_2 占位符

// 3.定义节点类；
class MinimalActionClient : public rclcpp::Node
{
public:
  explicit MinimalActionClient(const rclcpp::NodeOptions &node_options = rclcpp::NodeOptions())
      : Node("minimal_action_client", node_options) // 节点名 minimal_action_client
  {
    // 初始化动作客户端，动作名" get_sum" 必须和服务端保持一致
    this->client_ptr_ = rclcpp_action::create_client<Progress>(this, "get_sum");
  }

  // 客户端发送请求函数，成功发送返回true，否则返回false
  bool send_goal(int64_t num)
  {

    if (!this->client_ptr_)
    { // 防御性检查：客户端指针为空说明创建失败
      RCLCPP_ERROR(this->get_logger(), "动作客户端未被初始化。");
      return false; // 必须返回，否则下面解引用空指针会崩溃
    }

    // 等待动作服务端上线，最多等 10 秒
    if (!this->client_ptr_->wait_for_action_server(std::chrono::seconds(10)))
    {
      RCLCPP_ERROR(this->get_logger(), "服务连接失败！");
      return false;
    }

    auto goal_msg = Progress::Goal(); // 创建目标对象
    goal_msg.num = num;               // 填入目标数字
    RCLCPP_INFO(this->get_logger(), "发送请求数据！");

    // SendGoalOptions 用来注册三个回调函数，分别处理：目标响应、连续反馈、最终结果
    auto send_goal_options = rclcpp_action::Client<Progress>::SendGoalOptions();
    send_goal_options.goal_response_callback = std::bind(&MinimalActionClient::goal_response_callback, this, _1);
    send_goal_options.feedback_callback = std::bind(&MinimalActionClient::feedback_callback, this, _1, _2);
    send_goal_options.result_callback = std::bind(&MinimalActionClient::result_callback, this, _1);
    // 异步发送目标，函数立刻返回；结果全部由上面注册的回调处理，所以不需要保存返回值
    this->client_ptr_->async_send_goal(goal_msg, send_goal_options);
    return true;
  }

private:
  rclcpp_action::Client<Progress>::SharedPtr client_ptr_; // 动作客户端对象

  // 处理目标发送后反馈的回调函数，服务端回应“接受、拒绝”的时候调用
  void goal_response_callback(GoalHandleProgress::SharedPtr goal_handle)
  {
    if (!goal_handle)
    { // 空指针说明目标被拒绝
      RCLCPP_ERROR(this->get_logger(), "目标请求被服务器拒绝！");
    }
    else
    {
      RCLCPP_INFO(this->get_logger(), "目标被接收，等待结果中");
    }
  }

  // 处理连续反馈回调函数，服务端每发一次进度就调用一次
  void feedback_callback(GoalHandleProgress::SharedPtr, const std::shared_ptr<const Progress::Feedback> feedback)
  {
    int32_t progress = (int32_t)(feedback->progress * 100); // 0.3 -> 30
    RCLCPP_INFO(this->get_logger(), "当前进度: %d%%", progress);
  }

  // 最终响应回调函数，任务结束时调用一次
  void result_callback(const GoalHandleProgress::WrappedResult &result)
  {
    switch (result.code)
    {                                          // result.code 表示任务结束状态
    case rclcpp_action::ResultCode::SUCCEEDED: // 成功：继续往下打印结果
      break;
    case rclcpp_action::ResultCode::ABORTED:
      RCLCPP_ERROR(this->get_logger(), "任务被中止");
      return;
    case rclcpp_action::ResultCode::CANCELED:
      RCLCPP_ERROR(this->get_logger(), "任务被取消");
      return;
    default:
      RCLCPP_ERROR(this->get_logger(), "未知异常");
      return;
    }

    RCLCPP_INFO(this->get_logger(), "任务执行完毕，最终结果: %ld", result.result->sum); // result.result 里是服务端返回的结果
  }
};

int main(int argc, char **argv)
{
  // 2.初始化 ROS2 客户端；
  rclcpp::init(argc, argv);

  // 4.创建节点并发送目标：计算 1+2+...+10。
  auto action_client = std::make_shared<MinimalActionClient>();
  if (!action_client->send_goal(10))
  { // 发送失败（客户端异常或服务端未上线）就直接退出，不再空等
    rclcpp::shutdown();
    return 1;
  }
  // 5.进入事件循环，等待反馈和结果，直到 Ctrl+C。
  rclcpp::spin(action_client);
  // 6.释放资源。
  rclcpp::shutdown();
  return 0;
}
