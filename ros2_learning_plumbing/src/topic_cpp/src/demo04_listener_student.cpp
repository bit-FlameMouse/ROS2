// =====================================================================
// 订阅方（自定义消息版）：订阅 "topic_stu" 话题，打印收到的 Student 消息
// 运行方式：ros2 run topic_cpp demo04_listener_student
// 配套程序：demo03_talker_student
// =====================================================================

#include "rclcpp/rclcpp.hpp"                    // ROS2 C++ 核心库
#include "base_interfaces_demo/msg/student.hpp" // 自定义消息 Student

#include <functional>                           // std::bind
#include <memory>                               // std::make_shared

using std::placeholders::_1;                     // std::bind 占位符：回调的第 1 个参数（收到的消息）
using base_interfaces_demo::msg::Student;        // 简化类型名

// 3.定义节点类；
class MinimalSubscriber : public rclcpp::Node
{
  public:
    MinimalSubscriber()
    : Node("student_subscriber")                 // 节点名 student_subscriber
    {
      // 3-1.创建订阅方；消息类型 Student，话题名 topic_stu（要和发布方一致），队列长度 10
      subscription_ = this->create_subscription<Student>("topic_stu", 10, std::bind(&MinimalSubscriber::topic_callback, this, _1));
    }

  private:
    // 3-2.处理订阅到的消息；msg 就是发布方发来的 Student 消息
    void topic_callback(const Student & msg) const
    {
      // 依次打印消息里的姓名、年龄、身高
      RCLCPP_INFO(this->get_logger(), "订阅的学生消息：name=%s,age=%d,height=%.2f", msg.name.c_str(),msg.age, msg.height);
    }
    rclcpp::Subscription<Student>::SharedPtr subscription_; // 订阅方对象
};

int main(int argc, char * argv[])
{
  // 2.初始化 ROS2 客户端；
  rclcpp::init(argc, argv);
  // 4.调用spin函数，并传入节点对象指针：进入事件循环，等待消息到达。
  rclcpp::spin(std::make_shared<MinimalSubscriber>());
  // 5.释放资源；
  rclcpp::shutdown();
  return 0;
}
