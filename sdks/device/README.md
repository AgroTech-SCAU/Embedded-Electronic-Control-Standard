# device SDK

设备层保存可跨项目复用的设备协议与驱动

| 模块 | 说明 |
|---|---|
| [bus_motor](bus_motor/README.md) | 总线电机统一接口与 DM DJI 驱动 |
| [fs_ia10b](fs_ia10b/README.md) | FlySky iBUS 接收机 |
| [imu](imu/README.md) | IMU 与 BMI088 |
| [rgb_led](rgb_led/README.md) | RGB 与 WS2812 |
| [bus_servo](bus_servo/README.md) | 总线舵机 |

模块成熟度见 [module-status](../../docs/module-status.md)

## 边界

- device 通过 PortOps 或等价接口使用项目 platform
- CAN UART GPIO 句柄和真实硬件绑定由项目 `service/assemble` 负责
- public header 不直接依赖 HAL FSP CubeMX 项目头
- 安装方向 零位 机械补偿 比赛业务和安全策略留在项目侧
