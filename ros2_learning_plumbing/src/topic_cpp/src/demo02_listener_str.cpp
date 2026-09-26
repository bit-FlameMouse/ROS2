// =====================================================================
// 订阅方（Listener）：订阅 "topic" 话题，每收到一条消息就打印出来
// 运行方式：ros2 run topic_cpp demo02_listener_str
// 配套程序：demo01_talker_str（两个终端分别运行，即可看到发布/订阅效果）
// =====================================================================

#include "rclcpp/rclcpp.hpp"       // ROS2 C++ 核心库
#include "std_msgs/msg/string.hpp" // 消息类型必须和发布方一致

using std::placeholders::_1; // std::bind 的占位符：表示"回调函数的第 1 个参数"（这里就是收到的消息）

// 自定义节点类：继承 rclcpp::Node
class MinimalSubscriber : public rclcpp::Node
{
public:
    MinimalSubscriber()
        : Node("minimal_subscriber") // 给节点起名 minimal_subscriber
    {
        // 初始化订阅方：
        //   <std_msgs::msg::String> 订阅的消息类型
        //   "topic" 话题名称，必须和发布方一致
        //   10 消息队列长度
        //   收到消息后调用 topic_callback；std::bind 把成员函数与当前对象 this 绑定成可调用对象
        subscription_ = this->create_subscription<std_msgs::msg::String>("topic", 10, std::bind(&MinimalSubscriber::topic_callback, this, _1));
    }

private:
    // 回调函数：每收到一条消息，ROS2 就自动调用一次；msg 就是收到的消息
    void topic_callback(const std_msgs::msg::String &msg) const
    {
        RCLCPP_INFO(this->get_logger(), "订阅的消息： '%s'", msg.data.c_str()); // 打印消息内容
    }

    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_; // 订阅方对象
};

int main(int argc, char *argv[])
{
    // 2.初始化 ROS2 客户端；
    rclcpp::init(argc, argv);
    // 4.调用spin函数，并传入节点对象指针：进入事件循环，等待并处理收到的消息。
    rclcpp::spin(std::make_shared<MinimalSubscriber>());
    // 5.释放资源；
    rclcpp::shutdown();
    return 0;
}
