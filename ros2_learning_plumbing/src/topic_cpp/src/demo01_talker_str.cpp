// =====================================================================
// 发布方（Talker）：每隔 0.5 秒发布一条字符串消息，并附带一个不断自增的计数器
// 运行方式：ros2 run topic_cpp demo01_talker_str
// 配套程序：demo02_listener_str（先运行本程序，再运行订阅方即可看到消息）
// =====================================================================

#include "rclcpp/rclcpp.hpp"       // ROS2 的 C++ 核心库，Node、Publisher、Timer 等类都在这里
#include "std_msgs/msg/string.hpp" // ROS2 内置的标准 String 消息类型，消息内容放在 data 字段里

using namespace std::chrono_literals; // 允许直接写 500ms，等价于 std::chrono::milliseconds(500)

// 自定义节点类：继承 rclcpp::Node（一个类就是一个 ROS2 节点）
class MinimalPublisher : public rclcpp::Node
{
public:
    // 构造函数：创建节点对象时自动执行，用来初始化发布方和定时器
    // : Node("minimal_publisher") 调用父类构造函数，给节点起名叫 minimal_publisher
    // count_(0) 把计数器成员变量初始化为 0
    MinimalPublisher()
        : Node("minimal_publisher"), count_(0)
    {
        // 初始化发布方对象：
        //   <std_msgs::msg::String> 表示发布的消息类型
        //   "topic" 是话题名称（发布方和订阅方必须完全一致才能通信）
        //   10 是消息队列长度（QoS 队列深度：消息来不及处理时最多缓存 10 条）
        publisher_ = this->create_publisher<std_msgs::msg::String>("topic", 10);

        // 初始化定时器对象：每 500ms 触发一次 timer_callback 函数
        // timer_callback是非静态成员函数，成员函数指针不携带对象
        // 用std::bind绑定当前对象this，生成无参可调用对象作为定时器回调
        timer_ = this->create_wall_timer(500ms, std::bind(&MinimalPublisher::timer_callback, this));
    }

private:
    // 定时器的回调函数：时间到了 ROS2 就会自动调用它，在这里组织并发布消息
    void timer_callback()
    {
        auto message = std_msgs::msg::String();                     // 使用msg功能包下面的原生string组织消息
        message.data = "Hello, world! " + std::to_string(count_++); // 构造data字段内容：hello world+计数器的值（count_ 用完后自增 1）

        RCLCPP_INFO(this->get_logger(), "发布的消息：'%s'", message.data.c_str()); // 在终端打印日志；c_str() 把 std::string 转成 C 风格字符串以配合 %s
        publisher_->publish(message);                                              // 发布方发布任务（真正把消息发到话题上）
    }
    rclcpp::TimerBase::SharedPtr timer_;                            // 定时器对象（SharedPtr 是 ROS2 常用的智能指针，自动管理内存）
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_; // 发布方对象
    size_t count_;                                                  // 计数器，每发一条消息加 1
};

// 程序入口
int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv); // 1. 初始化 ROS2 通信环境（必须最先调用）

    rclcpp::spin(std::make_shared<MinimalPublisher>()); // 2. 创建节点并进入事件循环：主线程在这里不断处理定时器等回调，按 Ctrl+C 才会退出

    rclcpp::shutdown(); // 3. 关闭 ROS2 通信，释放资源
    return 0;
}
