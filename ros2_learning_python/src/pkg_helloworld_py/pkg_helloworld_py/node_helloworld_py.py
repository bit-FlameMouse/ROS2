import rclpy
from rclpy.node import Node

class Mynode(Node):
    def __init__(self):
        super().__init__("node_helloworld_py")

    def send_helloworld(self):
        self.get_logger().info("msg: hello world!")


def main():
    rclpy.init()   
    node=Mynode()
    node.send_helloworld()
    rclpy.shutdown()


if __name__=="__main__":
    main() 