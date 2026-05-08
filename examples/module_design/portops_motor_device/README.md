# portops_motor_device 示例

> 本示例展示一个设备层电机模块如何从需求边界、public header、PortOps、mock port 到 service 绑定形成最小闭环

---

## 1. 示例目标

实现一个可复用的电机设备层接口：

- 上层通过 `motor_device.h` 调用
- 底层发送、接收和时间能力由 `MotorDevicePortOps` 注入
- mock port 可在 PC 或无硬件环境下验证接口流程
- service 负责绑定 PortOps，并向 app 暴露系统能力

---

## 2. 文件结构

```text
examples/module_design/portops_motor_device/
├── README.md
├── motor_device.h
├── motor_device.c
├── mock_motor_port.c
└── chassis_service_example.c
```

---

## 3. 贯穿流程

1. 定义需求和安全边界：电机速度命令、反馈超时、stop 默认输出
2. 判断所属层级：电机命令和反馈属于 `device/`
3. 定义 public header：状态码、反馈、PortOps、配置、生命周期函数
4. 定义底层连接方式：通过 `MotorDevicePortOps` 接入 send/read/now_ms
5. 实现 device：参数检查、PortOps 检查、命令发送、反馈读取、超时判断
6. 实现 mock port：提供最小发送、读取和时间函数
7. 在 service 中绑定：service init 组装 PortOps 并调用 device init
8. app 只调用 service，不直接知道 CAN、UART 或 mock port

---

## 4. 与标准章节的对应关系

| 示例内容 | 对应标准 |
|---|---|
| `motor_device.h` | public header、错误码、反馈结构、PortOps |
| `motor_device.c` | device 层实现、参数检查、超时、stop |
| `mock_motor_port.c` | PC/mock 测试入口 |
| `chassis_service_example.c` | service init 统一绑定 PortOps |

---

## 5. 使用方式

该示例主要用于阅读和复制接口形态；真实项目中应把 `mock_motor_port.c` 替换为项目 `platform/` 或 chip SDK 提供的 CAN/UART 端口实现
