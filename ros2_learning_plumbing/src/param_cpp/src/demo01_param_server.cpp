// =====================================================================
// 参数服务端（Parameter Server）：声明参数，处理客户端的查询、修改、删除请求
// 参数是节点持有的"键值对"配置：服务端持有数据，客户端连接上来读写同一份数据
// 运行方式：ros2 run param_cpp demo01_param_server
// 配套程序：demo02_param_client（或命令行 ros2 param get/set/delete ...）
// =====================================================================

// 1.包含头文件；
#include "rclcpp/rclcpp.hpp"

#include <functional> // std::bind
#include <memory>     // std::make_shared
#include <vector>     // std::vector

using std::placeholders::_1; // 参数修改回调的第 1 个参数：本次被修改的参数列表

// 3.定义节点类；
class MinimalParamServer : public rclcpp::Node
{
public:
  // 构造函数：初始化父类节点。
  // 节点名 "minimal_param_server" 必须和参数客户端里写的服务端节点名一致，
  // 客户端才能通过这个名字找到本节点的参数服务。
  MinimalParamServer() : Node("minimal_param_server")
  {
    // 3-1.声明参数；
    // declare_parameter(参数名, 默认值, 参数描述符)：声明后参数才有名字和类型，
    // 客户端才能查询、修改它；未声明的参数，客户端设置时会被服务端拒绝。
    // 参数描述符可以给参数写一段说明，ros2 param describe 能查到。
    rcl_interfaces::msg::ParameterDescriptor car_type_descriptor;
    car_type_descriptor.description = "车辆类型";
    this->declare_parameter("car_type", "Tiger", car_type_descriptor); // 字符串类型

    rcl_interfaces::msg::ParameterDescriptor height_descriptor;
    height_descriptor.description = "车辆高度";
    this->declare_parameter("height", 1.50, height_descriptor); // 浮点类型

    rcl_interfaces::msg::ParameterDescriptor wheels_descriptor;
    wheels_descriptor.description = "车轮数量";
    this->declare_parameter("wheels", 4, wheels_descriptor); // 整型

    // 动态类型参数：dynamic_typing 置为 true 后，值可以是任意类型，也允许被删除；
    // 普通参数声明后类型固定、不能删除，所以想演示删除操作就得用动态类型参数。
    rcl_interfaces::msg::ParameterDescriptor temp_descriptor;
    temp_descriptor.description = "用于演示删除操作的动态参数";
    temp_descriptor.dynamic_typing = true;
    this->declare_parameter("temp_param", rclcpp::ParameterValue(100), temp_descriptor);

    // 3-2.注册参数修改回调；
    // 客户端每次 set_parameters 都会先调用这个回调：
    // 返回 successful=true 修改才真正生效，返回 false 则拒绝修改并说明原因。
    // 返回的句柄必须保存住，否则句柄析构后回调会被注销。
    param_callback_ = this->add_on_set_parameters_callback(
        std::bind(&MinimalParamServer::on_parameter_set, this, _1));

    // 3-3.查询参数（服务端本地读取，验证声明结果）；
    // get_parameters({...})：一次取多个参数，返回 vector<Parameter>，遍历打印即可。
    RCLCPP_INFO(this->get_logger(), "参数声明完毕，当前值如下：");
    for (const auto &param : this->get_parameters({"car_type", "height", "wheels", "temp_param"}))
    {
      RCLCPP_INFO(this->get_logger(), "%s = %s",
                  param.get_name().c_str(),         // 参数名
                  param.value_to_string().c_str()); // 参数值（统一转成字符串，不用关心具体类型）
    }
  }

private:
  // 参数修改回调的句柄：add_on_set_parameters_callback 的返回值，必须保存住
  rclcpp::Node::OnSetParametersCallbackHandle::SharedPtr param_callback_;

  // 参数修改回调：客户端每修改一个参数就被调用一次
  rcl_interfaces::msg::SetParametersResult on_parameter_set(
      const std::vector<rclcpp::Parameter> &parameters)
  {
    rcl_interfaces::msg::SetParametersResult result;
    result.successful = true;

    for (const auto &param : parameters)
    {
      // 删除参数时也会走到这个回调，此时参数类型是"未设置"
      if (param.get_type() == rclcpp::ParameterType::PARAMETER_NOT_SET)
      {
        RCLCPP_INFO(this->get_logger(), "参数 %s 被删除", param.get_name().c_str());
        continue;
      }

      RCLCPP_INFO(this->get_logger(), "收到参数修改：%s = %s",
                  param.get_name().c_str(), param.value_to_string().c_str());

      // 示例：height 必须大于 0，否则拒绝这次修改
      if (param.get_name() == "height" && param.as_double() <= 0.0)
      {
        result.successful = false;
        result.reason = "height 必须大于 0";
        return result;
      }
    }
    return result;
  }
};

int main(int argc, char **argv)
{
  // 2.初始化 ROS2 客户端；
  rclcpp::init(argc, argv);

  // 4.创建节点对象指针，并进入事件循环，等待参数客户端的查询/修改/删除请求（按 Ctrl+C 退出）。
  auto param_server = std::make_shared<MinimalParamServer>();
  rclcpp::spin(param_server);

  // 5.释放资源。
  rclcpp::shutdown();
  return 0;
}
