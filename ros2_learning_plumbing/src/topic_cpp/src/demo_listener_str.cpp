// 订阅方，解析并打印发布方发布的消息

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using std::placeholders::_1; // 用于std::bind

// 自定义节点类
class MinimalSubscriber : public rclcpp::Node
{
public:
    MinimalSubscriber()
        : Node("minimal_subscriber")
    {
        // 初始化订阅方

        //
        subscription_ = this->create_subscription<std_msgs::msg::String>("topic", 10, std::bind(&MinimalSubscriber::topic_callback, this, _1));
    }

private:
    // 回调函数，解析
    void topic_callback(const std_msgs::msg::String &msg) const
    {
        RCLCPP_INFO(this->get_logger(), "订阅的消息： '%s'", msg.data.c_str());
    }

    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_; // 订阅方对象
};

int main(int argc, char *argv[])
{
    // 2.初始化 ROS2 客户端；
    rclcpp::init(argc, argv);
    // 4.调用spin函数，并传入节点对象指针。
    rclcpp::spin(std::make_shared<MinimalSubscriber>());
    // 5.释放资源；
    rclcpp::shutdown();
    return 0;
}