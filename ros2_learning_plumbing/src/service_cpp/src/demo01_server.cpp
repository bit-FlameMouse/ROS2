// =====================================================================
// 服务端（Server）：提供 add_ints 服务，接收两个整数并返回它们的和
// 服务是"请求—响应"模型：客户端发一次请求，服务端回一次响应（适合简单的问答）
// 运行方式：ros2 run service_cpp demo01_server
// 配套程序：demo02_client（或命令行 ros2 service call ...）
// =====================================================================

#include "rclcpp/rclcpp.hpp"                        // ROS2 C++ 核心库
#include "base_interfaces_demo/srv/add_ints.hpp"    // 自定义服务接口：里面包含 AddInts::Request（请求）和 AddInts::Response（响应）

using base_interfaces_demo::srv::AddInts;           // 简化类型名

using std::placeholders::_1;                        // std::bind 占位符：回调的第 1 个参数（请求）
using std::placeholders::_2;                        // std::bind 占位符：回调的第 2 个参数（响应）

// 3.定义节点类；
class MinimalService: public rclcpp::Node{
  public:
    MinimalService():Node("minimal_service"){       // 调用父类构造函数，节点名 minimal_service
      // 3-1.创建服务端；
      // "add_ints" 是服务名称（客户端必须用同样的名字才能连上）
      // 每收到一次请求，ROS2 就调用一次 add 函数
      server = this->create_service<AddInts>("add_ints",std::bind(&MinimalService::add, this, _1, _2));
      RCLCPP_INFO(this->get_logger(),"add_ints 服务端启动完毕，等待请求提交...");
    }
  private:
    rclcpp::Service<AddInts>::SharedPtr server;     // 服务端对象
    // 3-2.处理请求数据并响应结果。
    // req 是客户端发来的请求（只读），res 是要返回给客户端的响应（可写）
    void add(const AddInts::Request::SharedPtr req,const AddInts::Response::SharedPtr res){
      res->sum = req->num1 + req->num2;             // 把两个数相加，写入响应字段 sum
      RCLCPP_INFO(this->get_logger(),"请求数据:(%d,%d),响应结果:%d", req->num1, req->num2, res->sum);
    }
};

int main(int argc, char const *argv[])
{
  // 2.初始化 ROS2 客户端；
  rclcpp::init(argc,argv);

  // 4.调用spin函数，并传入节点对象指针；进入事件循环，等待客户端请求（按 Ctrl+C 退出）。
  auto server = std::make_shared<MinimalService>();
  rclcpp::spin(server);

  // 5.释放资源。
  rclcpp::shutdown();
  return 0;
}
