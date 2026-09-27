// =====================================================================
// 客户端（Client）：连接 add_ints 服务，发送两个整数并打印服务端返回的和
// 运行方式：ros2 run service_cpp demo02_client 3 5   （3 和 5 是两个命令行参数）
// 配套程序：demo01_server（需要先启动服务端）
// =====================================================================

#include "rclcpp/rclcpp.hpp"                     // ROS2 C++ 核心库
#include "base_interfaces_demo/srv/add_ints.hpp" // 自定义服务接口

#include <cstdlib>                               // atoi()：把命令行里的字符串转成整数
#include <memory>                                // std::make_shared

using base_interfaces_demo::srv::AddInts;        // 简化类型名
using namespace std::chrono_literals;            // 允许写 1s

// 3.定义节点类；
class MinimalClient: public rclcpp::Node{
  public:
    MinimalClient():Node("minimal_client"){      // 节点名 minimal_client
      // 3-1.创建客户端；服务名 "add_ints"，必须和服务端一致
      client = this->create_client<AddInts>("add_ints");
      RCLCPP_INFO(this->get_logger(),"客户端创建，等待连接服务端！");
    }
    // 3-2.等待服务连接；
    bool connect_server(){
      // wait_for_service(1s)：最多等 1 秒，连上返回 true，超时返回 false；
      // 前面的 ! 表示"还没连上就继续循环重试"
      while (!client->wait_for_service(1s))
      {
        if (!rclcpp::ok())                       // rclcpp::ok() 为 false 说明程序被要求退出（例如 Ctrl+C）
        {
          RCLCPP_INFO(rclcpp::get_logger("rclcpp"),"强制退出！");
          return false;
        }

        RCLCPP_INFO(this->get_logger(),"服务连接中，请稍候...");
      }
      return true;
    }
    // 3-3.组织请求数据并发送；
    // 返回 SharedFuture（"未来的结果"）：请求是异步发出的，函数立刻返回，之后可以用它等待响应
    rclcpp::Client<AddInts>::SharedFuture send_request(int32_t num1, int32_t num2){
      auto request = std::make_shared<AddInts::Request>(); // 创建请求对象
      request->num1 = num1;                                // 填充请求字段
      request->num2 = num2;
      return client->async_send_request(request).future.share(); // 异步发送请求，不阻塞当前线程
    }


  private:
    rclcpp::Client<AddInts>::SharedPtr client;             // 客户端对象
};

int main(int argc, char ** argv)
{
  // 先检查命令行参数：argc 是参数个数，程序名本身算 1 个，所以加上两个整数共 3 个
  if (argc != 3){
    RCLCPP_INFO(rclcpp::get_logger("rclcpp"),"请提交两个整型数据！");
    return 1;                                              // 参数不对，返回非 0 表示出错
  }

  // 2.初始化 ROS2 客户端；
  rclcpp::init(argc,argv);

  // 4.创建对象指针并调用其功能；
  auto client = std::make_shared<MinimalClient>();
  bool flag = client->connect_server();                    // 等待服务端上线
  if (!flag)
  {
    RCLCPP_INFO(rclcpp::get_logger("rclcpp"),"服务连接失败！");
    return 0;
  }

  auto response = client->send_request(atoi(argv[1]),atoi(argv[2])); // atoi 把 "3" "5" 转成整数后发请求

  // 处理响应：
  // spin_until_future_complete 会一边处理回调一边等待 future 完成，成功则返回 SUCCESS
  if (rclcpp::spin_until_future_complete(client,response) == rclcpp::FutureReturnCode::SUCCESS)
  {
    RCLCPP_INFO(client->get_logger(),"请求正常处理");
    RCLCPP_INFO(client->get_logger(),"响应结果:%d!", response.get()->sum); // get() 取出服务端返回的响应对象

  } else {
    RCLCPP_INFO(client->get_logger(),"请求异常");
  }

  // 5.释放资源。
  rclcpp::shutdown();
  return 0;
}
