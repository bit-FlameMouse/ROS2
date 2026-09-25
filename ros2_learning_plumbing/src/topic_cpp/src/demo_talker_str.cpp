//发布方，每0.5s发布一个hello world，并附带计数器

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

// 自定义节点类；
class MinimalPublisher : public rclcpp::Node
{
public:
    // 构造函数,初始化父类node与成员变量count_
    MinimalPublisher()
        : Node("minimal_publisher"), count_(0)
    {
        // 初始化发布方对象
        publisher_ = this->create_publisher<std_msgs::msg::String>("topic", 10);
        
        
        // 初始化定时器对象
        //timer_callback是非静态成员函数，成员函数指针不携带对象
        //用std::bind绑定当前对象this，生成无参可调用对象作为定时器回调
        timer_ = this->create_wall_timer(500ms, std::bind(&MinimalPublisher::timer_callback, this));  
    }

private:
    // 定时器的回调函数，组织消息并发布
    void timer_callback()
    {
        auto message = std_msgs::msg::String(); // 使用msg功能包下面的原生string组织消息
        message.data = "Hello, world! " + std::to_string(count_++);  // 构造data字段内容：hello world+计数器的值
        
        RCLCPP_INFO(this->get_logger(), "发布的消息：'%s'", message.data.c_str());  
        publisher_->publish(message);  // 发布方发布任务
    }
    rclcpp::TimerBase::SharedPtr timer_;                            // 定时器对象
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_; // 发布方对象
    size_t count_;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);

    rclcpp::spin(std::make_shared<MinimalPublisher>());

    rclcpp::shutdown();
    return 0;
}