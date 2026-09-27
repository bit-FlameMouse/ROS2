// =====================================================================
// 案例练习：用窗口1中乌龟的速度，控制窗口2中的乌龟一起运动
// 需求：订阅窗口1中乌龟的位姿信息（其中包含线速度和角速度），
//       再根据这些速度生成控制窗口2中乌龟运动的指令并发布。
// 运行方式：ros2 launch exercise_cpp exe01_pub_sub_launch.py
// 注意：配套的 launch 文件会先启动两只乌龟，并把第二只乌龟掉头 180°，
//       等掉头完成后再启动本节点，这样第二只乌龟才能正确模仿第一只的动作。
// =====================================================================

// 1.包含头文件；
#include "rclcpp/rclcpp.hpp"           // ROS2 C++ 核心库：Node、Publisher、Subscription 等
#include "turtlesim/msg/pose.hpp"      // 乌龟位姿消息：含 linear_velocity、angular_velocity
#include "geometry_msgs/msg/twist.hpp" // 速度指令消息：linear.x 线速度，angular.z 角速度

#include <functional> // std::bind
#include <memory>     // std::make_shared

using std::placeholders::_1; // std::bind 的占位符：回调函数的第 1 个参数（这里就是收到的位姿消息）

// 3.定义节点类；一个类就是一个 ROS2 节点
class ExePubSub : public rclcpp::Node
{
public:
  // 构造函数：创建节点对象时自动执行
  // : rclcpp::Node("demo01_pub_sub") 调用父类构造函数，给节点起名叫 demo01_pub_sub
  ExePubSub() : rclcpp::Node("demo01_pub_sub")
  {
    // 3-1.创建控制第二个窗体乌龟运动的发布方；
    //     话题名 /t2/turtle1/cmd_vel 是绝对路径：
    //       /t2 是 launch 文件中给第二只乌龟起的命名空间（等价于"窗口2"）
    //       turtle1/cmd_vel 是 turtlesim 内置的速度指令话题
    //     末尾的 1 是消息队列长度：回调处理不及时就只保留最新 1 条，保证只用最新速度
    twist_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("/t2/turtle1/cmd_vel", 1);

    // 3-2.创建订阅第一个窗体乌龟pose的订阅方；
    //     话题名 /turtle1/pose 也是绝对路径：第一只乌龟（默认命名空间）发布自己的位姿
    //     每收到一条消息就自动调用 poseCallback；
    //     std::bind 把"成员函数"和"当前对象 this"绑定成一个可调用对象
    pose_sub_ = this->create_subscription<turtlesim::msg::Pose>(
      "/turtle1/pose", 1, std::bind(&ExePubSub::poseCallback, this, _1));
  }

private:
  // 3-3.根据订阅的乌龟的速度生成控制窗口2乌龟运动的速度消息并发布。
  // 回调函数：每收到一条位姿消息就被自动调用一次
  // 参数用 const 引用：不拷贝共享指针，也保证不会修改收到的消息
  void poseCallback(const turtlesim::msg::Pose::ConstSharedPtr & pose)
  {
    geometry_msgs::msg::Twist twist; // 准备发给第二只乌龟的速度指令

    // 线速度：保持不变，让第二只乌龟以同样的快慢前进（或后退）
    twist.linear.x = pose->linear_velocity;
    // 角速度：取反。第二只乌龟朝向与第一只相反，取反后它才能做出与第一只
    // 左右镜像、动作一致的运动（例如第一只逆时针转弯，第二只就顺时针转弯）
    twist.angular.z = -(pose->angular_velocity);
    // 发布速度指令：第二只乌龟订阅到该话题后就会运动
    twist_pub_->publish(twist);
  }

  // 发布方、订阅方的共享指针。必须作为成员变量长期持有，
  // 否则构造函数结束后对象就被销毁，通信也就中断了。
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr twist_pub_;
  rclcpp::Subscription<turtlesim::msg::Pose>::SharedPtr pose_sub_;
};

int main(int argc, char * argv[])
{
  // 2.初始化 ROS2 客户端；
  rclcpp::init(argc, argv);
  // 4.调用spin函数，并传入节点对象指针；进入事件循环，等待并处理收到的位姿消息
  rclcpp::spin(std::make_shared<ExePubSub>());
  // 5.释放资源。
  rclcpp::shutdown();
  return 0;
}
