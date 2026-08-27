# bus_motor DM STM32 Validation

该目录是 `bus_motor` 的**真实硬件集成验证工程**，不是教学 example

目标是验证以下完整链路：

```text
CubeMX / STM32 HAL
        ↓
platform (FDCAN / UART / SPI / DWT)
        ↓
service/assemble
        ↓
正式 sdks/device/bus_motor
        ↓
DM 电机
        ↓
反馈 / 模式切换 / 故障路径
```

---

## 1. 当前验证对象

当前工程代码配置的主要对象：

- STM32 HAL / CubeMX 工程
- FDCAN1：DM 电机通信
- DM 电机：`DMG6220`
- 固件：V4
- 电机 CAN ID：`0x01`
- Master ID：`0x11`
- USER_KEY：PA15，低电平按下
- RGB：WS2812，用颜色表示当前测试状态
- UART：输出测试日志

真实配置以 `src/service/assemble/assemble_motor.c` 与 `robot.ioc` 为准

---

## 2. 目录职责

```text
bus_motor_dm_stm32/
├── README.md
├── robot.ioc
├── Core/
│   └── Src/main.c
└── src/
    ├── app/                 # entry_init / entry_loop
    ├── service/
    │   ├── assemble/        # 将 platform 与 SDK/真实硬件绑定
    │   └── motor_test.*     # 测试状态机
    ├── device/              # 当前仍保留其他验证依赖的设备副本
    ├── infra/               # 当前仍保留其他验证依赖的 infra 副本
    └── platform/            # STM32 HAL 适配
```

**`bus_motor` 本体不在本目录维护副本**

构建时必须直接引用仓库正式实现：

```text
../../sdks/device/bus_motor/
```

以仓库根目录为基准则是：

```text
sdks/device/bus_motor/
```

---

## 3. 构建时需要加入的 bus_motor 文件

至少加入：

```text
sdks/device/bus_motor/bus_motor.c
sdks/device/bus_motor/dm_motor.c
sdks/device/bus_motor/dm_motor/dm_motor_core.c
sdks/device/bus_motor/dm_motor/dm_motor_protocol_v3.c
sdks/device/bus_motor/dm_motor/dm_motor_protocol_v4.c
```

并加入 include path，使以下包含能够解析：

```c
#include "bus_motor/bus_motor.h"
#include "bus_motor/dm_motor.h"
```

推荐 include 根路径指向：

```text
sdks/device/
```

如果当前 IDE / EIDE / Makefile 仍引用原 `examples/bus_motor_test/src/device/bus_motor/`，迁移后必须改为根 `sdks/device/bus_motor/`

---

## 4. 启动顺序

`Core/Src/main.c` 完成 CubeMX 外设初始化后调用：

```c
entry_init();
```

主循环持续调用：

```c
entry_loop();
```

当前 `entry_init()` 的主要顺序：

```text
assemble_delay
    ↓
assemble_log
    ↓
assemble_rgb
    ↓
assemble_motor
    ↓
motor_test_init
```

任何关键初始化失败都不会进入正常测试循环

---

## 5. 电机测试状态机

默认启动后保持：

```text
DISABLED
```

按 USER_KEY 后按以下顺序循环：

```text
DISABLED
   ↓
POS_VEL
   ↓
VEL
   ↓
DISABLED
```

当前目标值：

```text
target position = 3.14 rad
position-mode velocity target/limit = 3.14 rad/s
velocity mode target = 3.14 rad/s
```

命令周期约 `10 ms`，反馈日志周期约 `100 ms`

RGB 状态：

```text
红色 → DISABLED
蓝色 → POS_VEL
绿色 → VEL
```

---

## 6. 故障处理

驱动反馈中识别到故障后，测试业务会：

```text
锁存 fault
   ↓
切回 DISABLED
   ↓
停止继续发送运动目标
   ↓
等待用户再次按键清错
```

清错成功后仍保持 `DISABLED`，必须再次按键才进入运动模式

如果 disable 请求本身失败，代码会把安全状态标记为未确认并锁存 fault，不继续自动运行

---

## 7. 首次上真实硬件前

至少确认：

1. 电机已经脱离危险机械负载，或机构具有足够安全空间
2. CAN ID / Master ID / 型号 / 固件版本与实际一致
3. FDCAN 波特率和收发器使能引脚正确
4. USER_KEY 可以可靠触发失能/切换
5. UART 日志可以看到反馈计数持续增加
6. 急停或断电路径独立可用
7. 目标位置和速度适合当前测试机构
8. 当前测试 commit 已记录

本工程是 HIL / integration validation，不替代整机安全验证

---

## 8. 通过标准建议

一次可追溯的验证至少记录：

- commit SHA
- 控制板 / MCU
- DM 型号和固件版本
- CAN 配置
- `DISABLED → POS_VEL → VEL → DISABLED` 是否全部成功
- RX 反馈是否连续
- position / velocity / torque / temperature / error 日志是否合理
- fault 后是否能够进入安全失能并人工清错
- 是否发生异常运动、超时或通信错误

验证结果应描述具体硬件与条件，不建议只写“测试通过”

---

## 9. 为什么不放在 examples

这个工程包含 `.ioc`、真实板级外设、CAN 收发器、真实 DM 电机、故障处理和运动测试流程

因此它回答的是：

> 当前正式 SDK 在这套真实硬件集成上能不能工作？

而不是：

> 新成员应该如何用最少代码理解某个接口？

前者属于 `validation/`，后者才属于 `examples/`
