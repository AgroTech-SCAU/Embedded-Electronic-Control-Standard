# 公共 SDK 接入

按需编译正式源文件，通过项目 assemble 注入外部能力，禁止复制 SDK 到项目或 validation 维护第二份实现

| 目录 | 已包含模块 |
|---|---|
| infra | delay log PID matrix protocol_parser HFSM |
| domain | steer_wheel_kine 与 serial_arm 系列运动学 |
| device | bus_motor FS-iA10B IMU RGB bus_servo |

成熟度和验证范围见 [模块状态](../docs/module-status.md)，层级依赖见 [架构](../docs/architecture.md)

## 接入顺序

1. submodule 固定项目负责人确认的 Tag 或 commit
2. 按模块头文件选取源码和 include 根目录
3. 项目 platform 实现端口，assemble 提供配置与存储
4. 检查初始化返回值，完成单设备低输出验证
5. service 接管错误 超时 Stop 和人工恢复，app 调用 service

| 模块 | 接入注意 |
|---|---|
| bus_motor | 初始化公共 registry，再初始化厂家驱动并绑定逻辑 ID，公共接口不预设 CAN 分配 |
| FS-iA10B | 显式实例与 context PortOps，UART 回调路由到实例，主循环定期 maintain |
| IMU | 门面绑定具体驱动，BMI088 注入 SPI CS 时基和可选 DMA 完成路径 |
| RGB | 门面绑定 WS2812，提供颜色与发送缓存，异步传输需完成通知 |
| delay | 分别注入毫秒和微秒时钟，时基一致性由平台保证 |
| log | 注入 write，明确同步复制或异步持有缓存的语义 |
| steer_wheel_kine | 使用固定右手坐标和轮序，安装符号与偏置在项目侧处理 |

DM DJI FS-iA10B 的构建清单位于各自 validation 目录，include 根目录按清单加入，不使用删除的项目内 SDK 搜索路径

当前公共头文件只提供模块自己的状态码，不提供全仓库统一状态头文件
