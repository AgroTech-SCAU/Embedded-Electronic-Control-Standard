# device SDK 基本说明

> `sdks/device/` 提供常用真实设备的通用接口和协议实现；设备 SDK 通过 PortOps 与成员项目的 `platform/` 或 chip SDK 对接

---

## 1. 模块定位

device 模块负责真实设备的命令编码、反馈解析、反馈缓存、超时判断、停止和故障处理；它不负责 app 业务模式，也不应直接依赖某个 MCU 的 HAL/FSP/CubeMX 头文件

---

## 2. 当前模块

| 模块 | 目录 | 说明 |
|---|---|---|
| bus_motor | `bus_motor/` | 电机统一接口、入口单例、PortOps、达妙电机实例 |
| rgb_led | `rgb_led/` | RGB 灯统一接口，当前包含 WS2812 实例 |

---

## 3. PortOps 模板计划

device SDK 统一采用以下方向建设 PortOps 模板：

```text
sdks/device/
├── bus_motor/          # 已有：CAN 电机 PortOps 与实例接口
├── rgb_led/        # 已有：RGB 灯统一接口与 WS2812 实例
├── bus_servo/          # 计划：PWM/UART/CAN 舵机 PortOps
├── encoder/        # 计划：GPIO/Timer/SPI 编码器 PortOps
├── imu/            # 计划：SPI/I2C/UART IMU PortOps
├── can_sensor/     # 计划：CAN 传感器通用接入模板
└── uart_module/    # 计划：UART 模块收发和 parser 接入模板
```

---

## 4. 推荐 PortOps 形态

```c
typedef struct {
    bool (*send)(uint32_t id, const uint8_t* data, uint8_t len);
    bool (*read)(uint32_t* id, uint8_t* data, uint8_t* len);
    uint32_t (*now_ms)(void);
    void (*delay_ms)(uint32_t ms);
} DevicePortOps;

typedef struct {
    const DevicePortOps* ops;
    uint32_t feedback_timeout_ms;
} DeviceConfig;
```

优先使用 `device_init(config with PortOps)`，不优先把 `register_port()` 和 `init()` 分成两个必须手动按顺序调用的接口

---

## 5. 新增 device 模块检查表

- [ ] public header 不包含芯片头文件
- [ ] PortOps 能覆盖发送、接收、时间和必要延时
- [ ] init 时检查 PortOps 完整性
- [ ] 有反馈结构和反馈缓存
- [ ] 有超时判断
- [ ] 有 stop、disable、deinit 或 fault 处理
- [ ] 有 mock port 示例
- [ ] 有真实硬件低速测试记录模板

---

## 6. 示例参考

- `bus_motor/README.md`：当前电机 SDK 说明
- `rgb_led/README.md`：RGB 灯统一接口和 WS2812 实例说明
- `examples/module_design/portops_bus_motor_device/`：从需求到 service 绑定的 PortOps 贯穿式示例
