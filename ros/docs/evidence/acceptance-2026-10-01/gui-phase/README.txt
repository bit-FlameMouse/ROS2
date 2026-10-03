GUI 阶段截图留证（2026-10-02 凌晨，ROS_DOMAIN_ID=31，Wayland + VMware SVGA 软件渲染 llvmpipe）

A-03-rviz-map-desktop.png   整屏抓取（gnome-screenshot）：RViz 显示已保存地图——八角围墙、
                            9 根柱阵列、WP0/1/2 航点标记（文本标签）、机器人箭头，
                            右下 Navigation/Localization 面板均 active。与 Gazebo 世界布局一致。
A-02-gazebo-world-scanrays.png  窗口直读（import -window）：Gazebo 世界（八角+绿角柱+9 白柱），
                            机器人位于场内（画面下方中央），蓝色激光扇面清晰可辨；状态栏
                            RTF 0.99 / FPS 62.5 / Sim Time 在走。
A-02-gazebo-arena-wide.png  Gazebo 远景机位（同一世界，另一视角，含机器人激光扇面）。

状态说明：A-02 的"Gazebo 机器人可视"已具基本留证（机器人+激光扇面可辨），
"机器人特写级"摆位留证未完成（相机控制受 Wayland 焦点与 gz 工具链限制，
详见 测试轮次报告-2026-10-02.md §四）。
截图均在本次验收会话内抓取，不含任何个人信息。
