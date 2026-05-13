# servo SDK 接口文档

> `src/device/servo/` 提供舵机设备层统一入口，以及 FEETECH STS3215 串行舵机的 SCS 协议实例实现

---

## 1. 模块定位

`servo.*` 是舵机统一接口入口，供 service/app 上层调用

`sts3215_servo.*` 是 FEETECH STS3215 实例，负责 SCS 协议组帧、校验、应答解析、寄存器读写与常用控制命令

推荐使用方式：

```text
service init
-> 组装 ServoPortOps
-> servo_set_instance(&sts3215_servo_instance)
-> servo_init(&config)
-> app/service 统一调用 servo.xxx 或 servo_xxx
```

---

## 2. 文件结构

```text
src/device/servo/
|-- servo.h                 # 通用舵机接口、状态码、反馈结构、PortOps、入口单例
|-- servo.c                 # 入口单例转发实现
|-- sts3215_servo.h         # STS3215 寄存器/常量与实例声明
|-- sts3215_servo.c         # STS3215 SCS 协议实现
|-- Servo.md                # 原始项目说明资料
|-- 舵机SCS通信协议.md       # FEETECH SCS 协议参考资料
`-- README.md
```

---

## 3. PortOps

```c
typedef struct {
    bool (*write)(const uint8_t* data, uint16_t len);
    int (*read)(uint8_t* data, uint16_t len);
    uint32_t (*now_ms)(void);
    void (*delay_ms)(uint32_t ms);
} ServoPortOps;
```

约定：

- `write()` 成功返回 `true`，失败返回 `false`
- `read()` 返回实际读到的字节数，可以小于请求长度；SDK 内部会按 `timeout_ms` 循环补齐
- `now_ms()` 用于超时判断，必须提供
- `delay_ms()` 可选，用于总线收发切换后的短延时

---

## 4. 设计约束

- device 层不直接 include `main.h`、`usart.h`、`stm32xxx_hal.h` 等平台头文件
- 串口收发、时间、延时由 service/platform 通过 `ServoPortOps` 注入
- 上层优先依赖 `servo.h`，不要直接调用协议内部函数
- STS3215 的具体寄存器地址放在 `sts3215_servo.h`，业务层不需要记裸地址
- 表达是否的接口和字段使用 `bool`，协议寄存器原始 0/1 值仍使用 `uint8_t`

---

## 5. 初始化示例

```c
#include "servo.h"
#include "sts3215_servo.h"
#include "platform_uart.h"
#include "platform_time.h"

static const ServoPortOps servo_ops = {
    .write = platform_servo_uart_write,
    .read = platform_servo_uart_read,
    .now_ms = platform_time_now_ms,
    .delay_ms = platform_time_delay_ms,
};

void arm_servo_init(void)
{
    ServoConfig config = {
        .ops = &servo_ops,
        .timeout_ms = 20u,
        .retry_count = 0u,
        .endian = SERVO_ENDIAN_LITTLE,
    };

    servo_set_instance(&sts3215_servo_instance);
    servo_init(&config);
}
```

STS3215 属于磁编码串行舵机，双字节寄存器默认按小端处理

---

## 6. 常用调用

```c
uint8_t found_id;
ServoFeedback feedback;

servo.ping(1u, &found_id);
servo.enable_torque(1u);
servo.set_position(1u, 2048, 1000u, 50u);
servo.read_feedback(1u, &feedback);
```

`ServoFeedback.moving` 是 `bool`，表示舵机当前是否处于运动状态

---

## 7. 安全提醒

- 上真机前先确认独立供电、共地、TX/RX 接线和波特率
- 第一次调试先 `ping`，再低速、小范围运动
- 机械臂或夹爪联调前，应在 service/app 层加入角度、速度、负载、温度和超时保护
- 不建议在中断中调用阻塞式读写接口
