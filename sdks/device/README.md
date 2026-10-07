# 设备 SDK

设备层负责协议 命令 反馈与恢复，通过 PortOps 或等价依赖注入使用项目平台能力

| 模块 | 接入入口 |
|---|---|
| bus_motor | [DM DJI](bus_motor/README.md) |
| fs_ia10b | [显式实例 iBUS](fs_ia10b/README.md) |
| imu | [IMU 与 BMI088](imu/README.md) |
| rgb_led | [WS2812](rgb_led/README.md) |
| bus_servo | [舵机接口](bus_servo/README.md) |

成熟度以 [模块状态](../../docs/module-status.md) 为准，不按目录存在与否判定稳定性

设备公共头文件不包含 HAL FSP CubeMX 项目头，不固定 UART CAN GPIO 句柄，不包含安装方向 旧编号或比赛业务

项目 assemble 负责存储 配置和端口生命周期，service 负责错误处理 超时 Stop 和人工恢复，异步缓冲区与并发约束以对应模块契约为准
