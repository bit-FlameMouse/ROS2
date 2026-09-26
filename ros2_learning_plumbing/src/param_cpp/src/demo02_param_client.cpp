// =====================================================================
// 参数客户端（Parameter Client）：连接参数服务端，查询、修改、删除参数
// 参数客户端只能操作服务端已声明的参数，不能新增参数（新增要在服务端声明）
// 运行方式：ros2 run param_cpp demo02_param_client
// 配套程序：demo01_param_server（需要先启动参数服务端）
// =====================================================================

// 1.包含头文件；
#include "rclcpp/rclcpp.hpp"

#include <memory> // std::make_shared
#include <string> // std::string
#include <vector> // std::vector

using namespace std::chrono_literals; // 让 1s 这种时间写法可用

// 3.定义节点类；
class MinimalParamClient : public rclcpp::Node
{
public:
  MinimalParamClient() : Node("minimal_param_client")
  {
    // SyncParametersClient 是"同步参数客户端"：调用函数后会阻塞等待结果，代码简单直观。
    // 第 1 个参数 this：用当前节点去连接；
    // 第 2 个参数 "minimal_param_server"：要连接的参数服务端节点名(必须和服务端一致)。
    param_client_ = std::make_shared<rclcpp::SyncParametersClient>(this, "minimal_param_server");
  }

  // 3-1.连接参数服务端；
  bool connect_server()
  {
    // wait_for_service(1s)：最多等 1 秒，连上了返回 true。
    // 循环等待，直到连上或者按 Ctrl+C 退出(rclcpp::ok() 会变成 false)。
    while (!param_client_->wait_for_service(1s))
    {
      if (!rclcpp::ok())
      {
        return false;
      }
      RCLCPP_INFO(this->get_logger(), "参数服务端未连接，等待中...");
    }
    RCLCPP_INFO(this->get_logger(), "参数服务端已连接");
    return true;
  }

  // 3-2.新增参数（会被服务端拒绝）；
  void add_param()
  {
    RCLCPP_INFO(this->get_logger(), "-----------新增参数-----------");

    // ROS2 的规范做法是：参数必须由服务端 declare_parameter 声明，
    // 客户端不能凭空新增参数。这里故意设置一个服务端未声明的 width，
    // 观察服务端拒绝的结果和原因。
    auto results = param_client_->set_parameters({rclcpp::Parameter("width", 0.15)});
    RCLCPP_INFO(this->get_logger(), "新增 width：%s",
                results[0].successful ? "成功" : results[0].reason.c_str());
  }

  // 3-3.查询参数；
  void get_param()
  {
    RCLCPP_INFO(this->get_logger(), "-----------查询参数-----------");

    // has_parameter(参数名)：判断服务端是否存在该参数，返回 bool。
    RCLCPP_INFO(this->get_logger(), "car_type 存在吗？%s",
                param_client_->has_parameter("car_type") ? "是" : "否");
    RCLCPP_INFO(this->get_logger(), "car_typexxxx 存在吗？%s",
                param_client_->has_parameter("car_typexxxx") ? "是" : "否");

    // list_parameters({}, DEPTH_RECURSIVE)：列出服务端全部参数名。
    // 第 1 个参数是参数名前缀(传空表示不筛选)，第 2 个参数是层级(0 表示不限制层级)。
    auto list_result = param_client_->list_parameters(
        {}, rcl_interfaces::srv::ListParameters::Request::DEPTH_RECURSIVE);
    for (const auto &name : list_result.names)
    {
      RCLCPP_INFO(this->get_logger(), "服务端参数：%s", name.c_str());
    }

    // get_parameter<类型>(参数名)：查询单个参数，类型必须和服务端一致，否则会抛异常。
    double height = param_client_->get_parameter<double>("height");
    RCLCPP_INFO(this->get_logger(), "height = %.2f", height);

    // get_parameters({...})：一次查询多个参数，返回 vector<Parameter>，遍历打印即可。
    auto params = param_client_->get_parameters({"car_type", "height", "wheels"});
    for (const auto &param : params)
    {
      RCLCPP_INFO(this->get_logger(), "%s = %s",
                  param.get_name().c_str(),
                  param.value_to_string().c_str());
    }

    // describe_parameters({...})：查询参数的类型和说明(声明时写的参数描述符)。
    auto descriptors = param_client_->describe_parameters({"car_type", "height", "wheels"});
    for (const auto &descriptor : descriptors)
    {
      RCLCPP_INFO(this->get_logger(), "%s：类型=%s，说明=%s",
                  descriptor.name.c_str(),
                  rclcpp::to_string(static_cast<rclcpp::ParameterType>(descriptor.type)).c_str(),
                  descriptor.description.c_str());
    }
  }

  // 3-4.修改参数；
  void update_param()
  {
    RCLCPP_INFO(this->get_logger(), "-----------修改参数-----------");

    // set_parameters({...})：一次修改多个已经声明的参数，返回值是每个参数的设置结果。
    auto results = param_client_->set_parameters(
        {rclcpp::Parameter("car_type", "Mouse"),
         rclcpp::Parameter("height", 1.75),
         rclcpp::Parameter("wheels", 6)});
    for (const auto &result : results)
    {
      if (result.successful)
      {
        RCLCPP_INFO(this->get_logger(), "参数修改成功");
      }
      else
      {
        RCLCPP_WARN(this->get_logger(), "参数修改失败：%s", result.reason.c_str());
      }
    }

    // 服务端的参数回调会校验数值：height 必须大于 0，这次修改会被拒绝
    auto refused_results = param_client_->set_parameters({rclcpp::Parameter("height", -1.0)});
    RCLCPP_INFO(this->get_logger(), "把 height 改为 -1.0：%s",
                refused_results[0].successful ? "成功" : refused_results[0].reason.c_str());
  }

  // 3-5.删除参数；
  void del_param()
  {
    RCLCPP_INFO(this->get_logger(), "-----------删除参数-----------");

    // delete_parameters({...})：删除参数(本质是把参数设置成"未设置"状态)。
    // 只有声明为动态类型的参数(dynamic_typing=true)才能删除。
    auto results = param_client_->delete_parameters({"temp_param"});
    RCLCPP_INFO(this->get_logger(), "删除 temp_param：%s",
                results[0].successful ? "成功" : results[0].reason.c_str());

    // 删除后再判断一次：返回"否"说明参数确实已经不存在了。
    RCLCPP_INFO(this->get_logger(), "删除后，temp_param 存在吗？%s",
                param_client_->has_parameter("temp_param") ? "是" : "否");

    // 普通(静态类型)参数声明后不能删除，服务端会返回失败和原因
    auto refused_results = param_client_->delete_parameters({"car_type"});
    RCLCPP_INFO(this->get_logger(), "删除 car_type：%s",
                refused_results[0].successful ? "成功" : refused_results[0].reason.c_str());
  }

private:
  rclcpp::SyncParametersClient::SharedPtr param_client_; // 参数客户端对象
};

int main(int argc, char **argv)
{
  // 2.初始化 ROS2 客户端；
  rclcpp::init(argc, argv);

  // 4.创建节点对象指针，依次调用参数操作函数；
  auto param_client = std::make_shared<MinimalParamClient>();
  if (!param_client->connect_server())
  {
    RCLCPP_ERROR(param_client->get_logger(), "未能连接参数服务端，程序退出!");
    rclcpp::shutdown();
    return 1;
  }
  param_client->add_param();    // 增：尝试新增未声明的参数(会被拒绝)
  param_client->get_param();    // 查：看服务端当前参数
  param_client->update_param(); // 改：修改已有参数
  param_client->del_param();    // 删：删除动态参数
  param_client->get_param();    // 最后再查一次，验证修改是否生效

  // 5.释放资源。
  rclcpp::shutdown();
  return 0;
}
