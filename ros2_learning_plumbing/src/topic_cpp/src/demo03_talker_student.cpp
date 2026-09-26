// =====================================================================
// 发布方（自定义消息版）：每隔 0.5 秒发布一条 Student 消息
// 运行方式：ros2 run topic_cpp demo03_talker_student
// 配套程序：demo04_listener_student
// Student 是自定义消息，定义在 base_interfaces_demo/msg/Student.msg
// =====================================================================

#include "rclcpp/rclcpp.hpp"                         // ROS2 C++ 核心库 
#include "base_interfaces_demo/msg/student.hpp"      // 自定义消息 Student，编译接口包时自动生成

using namespace std::chrono_literals;                // 允许写 500ms
using base_interfaces_demo::msg::Student;            // 简化类型名，后面直接写 Student

// 3.定义节点类；
class MinimalPublisher : public rclcpp::Node
{
  public:
    MinimalPublisher()
    : Node("student_publisher"), count_(0)           // 节点名 student_publisher，计数器初始化为 0
    {
      // 3-1.创建发布方；消息类型 Student，话题名 topic_stu，队列长度 10
      publisher_ = this->create_publisher<Student>("topic_stu", 10);
      // 3-2.创建定时器；每 500ms 调用一次 timer_callback
      timer_ = this->create_wall_timer(500ms, std::bind(&MinimalPublisher::timer_callback, this));
    }

  private:
    void timer_callback()
    {
      // 3-3.组织消息并发布。
      auto stu = Student();                          // 创建一个空的 Student 消息对象
      stu.name = "张三";                             // 给 name 字段赋值（string）
      stu.age = count_++;                            // 给 age 字段赋值，每发一次年龄加 1（count_ 是 size_t，会隐式转成 int32）
      stu.height = 1.65;                             // 给 height 字段赋值（float64）
      // 日志格式说明：%s 字符串、%d 整数、%.2f 保留两位小数的浮点数
      RCLCPP_INFO(this->get_logger(), "学生信息:name=%s,age=%d,height=%.2f", stu.name.c_str(),stu.age,stu.height);
      publisher_->publish(stu);                      // 把消息发布到话题 topic_stu
    }
    rclcpp::TimerBase::SharedPtr timer_;             // 定时器对象
    rclcpp::Publisher<Student>::SharedPtr publisher_;// 发布方对象
    size_t count_;                                   // 计数器
};

int main(int argc, char * argv[])
{
  // 2.初始化 ROS2 客户端；
  rclcpp::init(argc, argv);
  // 4.调用spin函数，并传入节点对象指针：进入事件循环，定时器到点就发布。
  rclcpp::spin(std::make_shared<MinimalPublisher>());
  // 5.释放资源；
  rclcpp::shutdown();
  return 0;
}
