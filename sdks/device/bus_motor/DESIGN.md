# bus_motor Design & Maintainer Guide

> 本文面向 `bus_motor` SDK 维护者、厂家驱动开发者和需要理解内部语义的集成人员
> 普通业务使用与首次接入请优先阅读 [`README.md`](README.md)
> 本文件由原完整 `bus_motor` 文档迁移而来，用于保留架构、厂家适配、能力扩展与 Group 设计细节

---


`bus_motor` 是总线电机的统一能力接口

它不统一厂家协议，也不要求所有电机拥有完全相同的功能，而是统一业务层真正依赖的控制语义

核心目标是将电机替换影响限制在 `device/bus_motor/` 与 `service/assemble/`，业务层只依赖逻辑电机 ID 和 `bus_motor`

```text
接口规定
    ↓
厂家驱动实现
    ↓
assemble 绑定具体实例
    ↓
业务层通过 bus_motor 调用
```

---

## 1. Quick Start

### 1.1. 定义业务逻辑电机 ID

`bus_motor` 只规定 ID 类型

```c
typedef uint16_t BusMotorId;
```

具体逻辑 ID 由业务层定义

例如

```c
typedef enum {
    MOTOR_ARM_J1 = 0,
    MOTOR_ARM_J2,
    MOTOR_ARM_J3,
    MOTOR_ARM_J4,
} RobotMotorId;
```

这里的 `MOTOR_ARM_J1` 表示机器人系统中的 J1 关节

它不是 CAN ID

正确关系是

```text
MOTOR_ARM_J1
    ↓ assemble 绑定
DM4340
CAN ID 0x01
Master ID 0x11
```

以后换其他厂家电机时仍然可以保持 `MOTOR_ARM_J1` 不变

### 1.2. 准备达妙底层端口

达妙驱动通过 `BusMotorPortOps` 使用底层 CAN

至少需要实现 `send`

当前达妙模式切换还需要等待 ACK，因此推荐同时提供 `now_ms` 与 `delay_ms`

```c
static bool motor_can_send(uint32_t id, const uint8_t* data, uint8_t len) {
    return can_send(&hfdcan1, id, data, len) == STM32_HAL_CAN_OK;
}

static BusMotorPortOps s_motor_ops = {
    .send = motor_can_send,
    .read = 0,
    .now_ms = HAL_GetTick,
    .delay_ms = delay_ms,
    .flush_rx = 0,
};
```

当前含义

- `send`：发送 CAN 帧
- `now_ms`：记录反馈接收时间
- `delay_ms`：模式切换等待 ACK 时使用
- `read`：当前流程不要求
- `flush_rx`：当前流程不要求

如果没有提供 `delay_ms`，需要等待模式切换 ACK 时会返回 `MOTOR_STATUS_PORT_ERROR`

### 1.3. 配置真实 DM 电机

例如 J1 使用 DM4340 V4

```c
static const DmMotorConfig s_j1_config = {
    .can_id = 0x01u,
    .master_id = 0x11u,
    .model = DM_MOTOR_MODEL_DM4340,
    .firmware = {
        .major = DM_MOTOR_FIRMWARE_V4,
    },
    .default_mode = DM_MOTOR_MODE_POS_VEL,
};
```

这里允许出现具体厂家信息，因为这些内容属于真实硬件装配

业务层不应该知道这些字段

### 1.4. 提供实例内存并完成初始化绑定

需要由 assemble 按当前工程真实使用数量提供静态实例内存

例如只有一台 DM 电机时

```c
static DmMotorInstance s_dm_motors[1];
```

如果实际有 6 台 DM，就直接声明

```c
static DmMotorInstance s_dm_motors[6];
```

推荐初始化顺序

```c
if(bus_motor.init() != MOTOR_STATUS_OK) {
    return false;
}

if(dm_motor_init(&s_motor_ops, s_dm_motors,
        sizeof(s_dm_motors) / sizeof(s_dm_motors[0])) != MOTOR_STATUS_OK) {
    return false;
}

if(dm_motor_bind(MOTOR_ARM_J1, &s_j1_config) != MOTOR_STATUS_OK) {
    return false;
}
```

三步分别完成

```text
assemble
    ↓
提供 DmMotorInstance[] 静态实例内存

bus_motor.init()
    ↓
清空 bus_motor 逻辑电机 Registry 与 Group Registry

dm_motor_init()
    ↓
接管 assemble 提供的实例数组
    ↓
记录 instances / capacity / count
    ↓
清空该实例数组

dm_motor_bind()
    ↓
从实例数组中取得下一个空闲 instance
    ↓
写入型号 / 固件 / CAN ID / 运行状态
    ↓
选择 V3 / V4 协议
    ↓
计算当前实例支持的公共 Profile
    ↓
注册到 bus_motor
    ↓
MOTOR_ARM_J1 ↔ DM instance
```

### 1.5. 接入 CAN RX

达妙反馈和模式切换 ACK 都需要通过 RX callback 进入驱动

推荐顺序

```c
static void motor_can_rx(FDCAN_HandleTypeDef* hcan, const FDCAN_RxHeaderTypeDef* header,
    const uint8_t data[8], void* user) {
    (void)hcan;
    (void)user;

    if(header == 0) {
        return;
    }

    if(dm_motor_parse_parameter_frame(header->Identifier, data)) {
        return;
    }

    dm_motor_parse_feedback_frame(header->Identifier, data, 0);
}
```

参数帧必须先解析

```text
RX frame
    ↓
dm_motor_parse_parameter_frame()
    ├── true  → 参数帧或模式切换 ACK，结束
    └── false
            ↓
dm_motor_parse_feedback_frame()
```

然后注册并启动 CAN

```c
if(can_register_rx_callback(&hfdcan1, motor_can_rx, 0) != STM32_HAL_CAN_OK) {
    return false;
}

if(can_filter_init(&hfdcan1) != STM32_HAL_CAN_OK) {
    return false;
}

if(can_start(&hfdcan1) != STM32_HAL_CAN_OK) {
    return false;
}
```

在调用 `bus_motor.profile.activate()` 前必须保证 RX callback 已经能够持续工作

### 1.6. 检查业务必须能力

假设机械臂 J1 必须同时支持位置和阻抗控制

```c
BusMotorProfileMask required =
    BUS_MOTOR_PROFILE_POSITION |
    BUS_MOTOR_PROFILE_IMPEDANCE;

if(bus_motor.profile.require(MOTOR_ARM_J1, required) != MOTOR_STATUS_OK) {
    return false;
}
```

`require()` 推荐只在初始化阶段执行

它解决的是

```text
这个业务能不能在当前硬件上成立
```

如果未来换成不支持阻抗控制的电机，系统会在初始化阶段直接返回 `MOTOR_STATUS_UNSUPPORTED`

### 1.7. 完成启动准备

达妙当前推荐由 assemble 完成厂家初始化过程

```c
if(dm_motor_clear_error(MOTOR_ARM_J1) != MOTOR_STATUS_OK) {
    return false;
}

if(bus_motor.profile.activate(MOTOR_ARM_J1, BUS_MOTOR_PROFILE_POSITION) != MOTOR_STATUS_OK) {
    return false;
}

if(bus_motor.basic.enable(MOTOR_ARM_J1) != MOTOR_STATUS_OK) {
    return false;
}
```

这里要注意

```text
DmMotorConfig.default_mode
```

只表示 `DmMotorInstance` 绑定时认定的初始硬件模式

它不等价于

```text
bus_motor 已经建立当前公共 Profile
```

因此即使 `default_mode == DM_MOTOR_MODE_POS_VEL`，仍然推荐显式调用

```c
bus_motor.profile.activate(MOTOR_ARM_J1, BUS_MOTOR_PROFILE_POSITION);
```

这样 `bus_motor.profile.current()` 才与实际硬件控制状态建立明确对应

### 1.8. 业务层开始控制

位置控制

```c
bus_motor.pos(MOTOR_ARM_J1, target_position);
```

读取位置

```c
float position;

if(bus_motor.feedback.position(MOTOR_ARM_J1, &position) == MOTOR_STATUS_OK) {
    // 使用 position
}
```

读取完整反馈

```c
BusMotorFeedback feedback;

if(bus_motor.feedback.all(MOTOR_ARM_J1, &feedback) == MOTOR_STATUS_OK) {
    // 使用 feedback
}
```

业务代码只需要 include

```c
#include "bus_motor/bus_motor.h"
```

普通业务不需要 include `dm_motor.h`

### 1.9. 运行时切换到阻抗控制

运行时状态变化时可以切换 Profile

例如自由空间使用位置控制

```c
static void arm_enter_free(void) {
    bus_motor.profile.activate(MOTOR_ARM_J1, BUS_MOTOR_PROFILE_POSITION);
}

static void arm_run_free(float target) {
    bus_motor.pos(MOTOR_ARM_J1, target);
}
```

检测到接触后切换为阻抗

```c
static bool arm_enter_contact(void) {
    return bus_motor.profile.activate(MOTOR_ARM_J1,
        BUS_MOTOR_PROFILE_IMPEDANCE) == MOTOR_STATUS_OK;
}
```

周期阻抗控制

```c
static void arm_run_contact(float q, float dq, float kp, float kd, float tau) {
    BusMotorImpedanceCommand command = {
        .position = q,
        .velocity = dq,
        .kp = kp,
        .kd = kd,
        .torque = tau,
    };

    bus_motor.imp(MOTOR_ARM_J1, &command);
}
```

离开接触后恢复位置控制

```c
static bool arm_leave_contact(void) {
    return bus_motor.profile.activate(MOTOR_ARM_J1,
        BUS_MOTOR_PROFILE_POSITION) == MOTOR_STATUS_OK;
}
```

整个业务状态机不需要知道

```text
DM_MOTOR_MODE_POS_VEL
DM_MOTOR_MODE_MIT
RID10
V3 / V4
```

### 1.10. Quick Start 完整 assemble 示例

下面将前面的步骤合并

```c
typedef enum {
    MOTOR_ARM_J1 = 0,
} RobotMotorId;

static bool motor_can_send(uint32_t id, const uint8_t* data, uint8_t len) {
    return can_send(&hfdcan1, id, data, len) == STM32_HAL_CAN_OK;
}

static BusMotorPortOps s_motor_ops = {
    .send = motor_can_send,
    .read = 0,
    .now_ms = HAL_GetTick,
    .delay_ms = delay_ms,
    .flush_rx = 0,
};

static DmMotorInstance s_dm_motors[1];

static const DmMotorConfig s_j1_config = {
    .can_id = 0x01u,
    .master_id = 0x11u,
    .model = DM_MOTOR_MODEL_DM4340,
    .firmware = {
        .major = DM_MOTOR_FIRMWARE_V4,
    },
    .default_mode = DM_MOTOR_MODE_POS_VEL,
};

static void motor_can_rx(FDCAN_HandleTypeDef* hcan, const FDCAN_RxHeaderTypeDef* header,
    const uint8_t data[8], void* user) {
    (void)hcan;
    (void)user;

    if(header == 0) {
        return;
    }

    if(dm_motor_parse_parameter_frame(header->Identifier, data)) {
        return;
    }

    dm_motor_parse_feedback_frame(header->Identifier, data, 0);
}

bool assemble_motor(void) {
    BusMotorProfileMask required =
        BUS_MOTOR_PROFILE_POSITION |
        BUS_MOTOR_PROFILE_IMPEDANCE;

    if(bus_motor.init() != MOTOR_STATUS_OK) {
        return false;
    }

    if(dm_motor_init(&s_motor_ops, s_dm_motors,
            sizeof(s_dm_motors) / sizeof(s_dm_motors[0])) != MOTOR_STATUS_OK) {
        return false;
    }

    if(dm_motor_bind(MOTOR_ARM_J1, &s_j1_config) != MOTOR_STATUS_OK) {
        return false;
    }

    if(can_register_rx_callback(&hfdcan1, motor_can_rx, 0) != STM32_HAL_CAN_OK) {
        return false;
    }

    if(can_filter_init(&hfdcan1) != STM32_HAL_CAN_OK) {
        return false;
    }

    if(can_start(&hfdcan1) != STM32_HAL_CAN_OK) {
        return false;
    }

    if(bus_motor.profile.require(MOTOR_ARM_J1, required) != MOTOR_STATUS_OK) {
        return false;
    }

    if(dm_motor_clear_error(MOTOR_ARM_J1) != MOTOR_STATUS_OK) {
        return false;
    }

    if(bus_motor.profile.activate(MOTOR_ARM_J1,
            BUS_MOTOR_PROFILE_POSITION) != MOTOR_STATUS_OK) {
        return false;
    }

    if(bus_motor.basic.enable(MOTOR_ARM_J1) != MOTOR_STATUS_OK) {
        return false;
    }

    return true;
}
```

在 assemble 完成绑定与初始化后，业务层只需要使用

```c
bus_motor.pos(...);
bus_motor.vel(...);
bus_motor.tor(...);
bus_motor.imp(...);
bus_motor.cmd(...);

bus_motor.basic.xxx(...);
bus_motor.profile.xxx(...);
bus_motor.feedback.xxx(...);
bus_motor.group.xxx(...);
```

---

## 2. bus_motor 基本使用模型

Quick Start 展示了怎么使用，下面解释接口为什么这样设计

### 2.1. 文件结构

```text
bus_motor/
├── bus_motor.h
├── bus_motor.c
├── dm_motor.h
├── dm_motor.c
├── README.md
└── dm_motor/
    ├── dm_motor_core.h
    ├── dm_motor_core.c
    ├── dm_motor_protocol.h
    ├── dm_motor_protocol_v3.c
    └── dm_motor_protocol_v4.c
```

职责

- `bus_motor.h/.c`：统一能力接口、逻辑电机 Registry、Profile、Command、Feedback、Group 和统一分发
- `dm_motor.h/.c`：达妙配置接口、实例绑定、厂家特殊功能和 `bus_motor` 适配
- `dm_motor/dm_motor_core.*`：达妙型号参数、Instance、Registry 与基础 Codec
- `dm_motor/dm_motor_protocol.h`：达妙内部协议接口
- `dm_motor/dm_motor_protocol_v3.c`：V3 协议实现
- `dm_motor/dm_motor_protocol_v4.c`：V4 协议实现

依赖关系

```text
service
    │
    │ BusMotorId
    ▼
bus_motor
    │
    │ Registry
    ▼
BusMotorDriver
    │
    │ instance
    ▼
dm_motor / other_motor
    │
    ▼
厂家协议
```

业务层不接触 `BusMotorDriver`、厂家 instance、CAN ID、Master ID、厂家硬件模式和协议帧

### 2.2. 逻辑电机 ID

`BusMotorId` 是业务逻辑 ID 的底层类型

```c
typedef uint16_t BusMotorId;
```

具体枚举由业务层定义

```c
typedef enum {
    MOTOR_ARM_J1 = 0,
    MOTOR_ARM_J2,
    MOTOR_STEER_FL,
    MOTOR_STEER_FR,
} RobotMotorId;
```

逻辑 ID 的核心作用是保持业务身份稳定

```text
MOTOR_ARM_J1
    ↓
assemble
    ↓
DM4340
```

未来可以替换为

```text
MOTOR_ARM_J1
    ↓
assemble
    ↓
Other Motor
```

业务层仍然使用相同的 `MOTOR_ARM_J1`

### 2.3. Profile

Profile 表示

> 当前运行时采用哪一种厂家无关控制语义

当前公共 Profile 包括

```text
POSITION
VELOCITY
TORQUE
IMPEDANCE
CURRENT_Q
VOLTAGE_Q
CURRENT_DQ
VOLTAGE_DQ
ACCELERATION
```

Profile 不是厂家模式

例如当前达妙映射

```text
BUS_MOTOR_PROFILE_POSITION
        ↓
DM_MOTOR_MODE_POS_VEL

BUS_MOTOR_PROFILE_VELOCITY
        ↓
DM_MOTOR_MODE_VEL

BUS_MOTOR_PROFILE_TORQUE
        ↓
DM_MOTOR_MODE_MIT

BUS_MOTOR_PROFILE_IMPEDANCE
        ↓
DM_MOTOR_MODE_MIT
```

可以出现多个公共 Profile 映射到同一个厂家模式

因为公共 Profile 表达的是业务控制语义，不是硬件 mode 编号

### 2.4. Profile 的四个接口

#### 2.4.1. supports()

检查某个能力是否存在

```c
if(bus_motor.profile.supports(MOTOR_ARM_J1, BUS_MOTOR_PROFILE_IMPEDANCE)) {
    // 可以使用阻抗控制路径
}
```

适用于可选功能

例如有阻抗能力就使用阻抗，没有时业务允许选择另一条路径

#### 2.4.2. require()

检查业务硬约束

```c
BusMotorProfileMask required =
    BUS_MOTOR_PROFILE_POSITION |
    BUS_MOTOR_PROFILE_IMPEDANCE;

if(bus_motor.profile.require(MOTOR_ARM_J1, required) != MOTOR_STATUS_OK) {
    return false;
}
```

推荐在 service 初始化阶段调用一次

实时循环不需要重复调用

#### 2.4.3. activate()

运行时切换当前控制语义

```c
bus_motor.profile.activate(MOTOR_ARM_J1, BUS_MOTOR_PROFILE_IMPEDANCE);
```

厂家驱动负责完成真正的模式迁移

可能包括

```text
disable
    ↓
发送模式切换
    ↓
等待 ACK
    ↓
重新 enable
```

只有厂家 `activate()` 返回 `MOTOR_STATUS_OK` 后，`bus_motor` 才更新公共 Profile

#### 2.4.4. current()

查询当前公共 Profile

```c
BusMotorProfile profile = bus_motor.profile.current(MOTOR_ARM_J1);
```

如果逻辑 ID 不存在或当前已经进入厂家私有模式，返回

```c
BUS_MOTOR_PROFILE_NONE
```

### 2.5. Command

Command 表示

> 在当前 Profile 下发送什么控制目标

正常运行关系

```text
activate Profile
    ↓
Profile READY
    ↓
周期发送 Command
```

当前 Command 包括

```text
POSITION
VELOCITY
TORQUE
IMPEDANCE
CURRENT_Q
VOLTAGE_Q
CURRENT_DQ
VOLTAGE_DQ
ACCELERATION
```

高频通用命令提供短接口

```c
bus_motor.pos(MOTOR_ARM_J1, position);
bus_motor.vel(MOTOR_ARM_J1, velocity);
bus_motor.tor(MOTOR_ARM_J1, torque);
bus_motor.imp(MOTOR_ARM_J1, &command);
```

这些只是 `bus_motor.cmd()` 的快捷入口

例如

```c
bus_motor.pos(MOTOR_ARM_J1, target);
```

等价于

```c
bus_motor.cmd(MOTOR_ARM_J1, BUS_CMD_POSITION(target));
```

低频和扩展控制统一使用

```c
bus_motor.cmd(...)
```

例如

```c
bus_motor.cmd(MOTOR_ARM_J1, BUS_CMD_CURRENT_Q(target_iq));
bus_motor.cmd(MOTOR_ARM_J1, BUS_CMD_VOLTAGE_Q(target_vq));
bus_motor.cmd(MOTOR_ARM_J1, BUS_CMD_CURRENT_DQ(target_id, target_iq));
bus_motor.cmd(MOTOR_ARM_J1, BUS_CMD_VOLTAGE_DQ(target_vd, target_vq));
bus_motor.cmd(MOTOR_ARM_J1, BUS_CMD_ACCELERATION(target_acc));
```

### 2.6. Profile 与 Command 匹配

Profile 不会被控制命令隐式切换

例如当前位置 Profile 下

```c
bus_motor.pos(MOTOR_ARM_J1, target);
```

是合法的

如果当前 Profile 是 IMPEDANCE，却调用

```c
bus_motor.pos(MOTOR_ARM_J1, target);
```

返回

```text
MOTOR_STATUS_PROFILE_MISMATCH
```

不会自动切换到 POSITION

这样可以保证

```text
Profile 切换
```

始终是业务状态迁移的一部分，而不是被每个实时控制命令偷偷触发

### 2.7. Basic

基础接口

```text
enable
disable
stop
brake
```

调用

```c
bus_motor.basic.enable(MOTOR_ARM_J1);
bus_motor.basic.disable(MOTOR_ARM_J1);
bus_motor.basic.stop(MOTOR_ARM_J1);
bus_motor.basic.brake(MOTOR_ARM_J1);
```

公共层只规定业务语义

不同厂家具体实现可以不同

如果厂家无法满足语义，应返回 `MOTOR_STATUS_UNSUPPORTED`

不能为了接口统一而静默替换成另一个行为

例如没有 brake 能力时不能默认

```text
brake → stop
```

### 2.8. Feedback

完整反馈

```c
BusMotorFeedback feedback;
bus_motor.feedback.all(MOTOR_ARM_J1, &feedback);
```

字段读取

```c
float position;
float velocity;
float torque;
BusMotorTemperature temperature;

bus_motor.feedback.position(MOTOR_ARM_J1, &position);
bus_motor.feedback.velocity(MOTOR_ARM_J1, &velocity);
bus_motor.feedback.torque(MOTOR_ARM_J1, &torque);
bus_motor.feedback.temperature(MOTOR_ARM_J1, &temperature);
```

`BusMotorFeedback.valid` 标记厂家实际提供哪些字段

例如

```c
if((feedback.valid & BUS_MOTOR_FEEDBACK_TORQUE) != 0u) {
    // torque 有效
}
```

如果厂家没有某个反馈字段，对应读取接口返回 `MOTOR_STATUS_UNSUPPORTED`

不会用默认 `0` 冒充真实反馈

---

## 3. 达妙驱动详细使用

### 3.1. 当前支持范围

当前 DM 已映射公共 Profile

```text
POSITION   → POS_VEL
VELOCITY   → VEL
TORQUE     → MIT
IMPEDANCE  → MIT
```

当前没有向公共层声明

```text
CURRENT_Q
VOLTAGE_Q
CURRENT_DQ
VOLTAGE_DQ
ACCELERATION
```

因此这些命令对 DM 实例返回 `MOTOR_STATUS_UNSUPPORTED`

当前保留的达妙私有接口

```c
dm_motor_clear_error()
dm_motor_save_zero()
dm_motor_switch_mode()
dm_motor_set_pos_force()
dm_motor_parse_feedback_frame()
dm_motor_parse_parameter_frame()
```

这些不是普通业务控制入口

主要用于

```text
assemble
诊断
标定
厂家专用 Adapter
```

### 3.2. 多实例绑定

同一个系统可以绑定多个达妙实例

```c
static const DmMotorConfig s_motor_config[] = {
    {
        .can_id = 0x01u,
        .master_id = 0x11u,
        .model = DM_MOTOR_MODEL_DM4340,
        .firmware = { .major = DM_MOTOR_FIRMWARE_V4 },
        .default_mode = DM_MOTOR_MODE_POS_VEL,
    },
    {
        .can_id = 0x02u,
        .master_id = 0x12u,
        .model = DM_MOTOR_MODEL_DM6006,
        .firmware = { .major = DM_MOTOR_FIRMWARE_V4 },
        .default_mode = DM_MOTOR_MODE_POS_VEL,
    },
};

if(dm_motor_bind(MOTOR_ARM_J1, &s_motor_config[0]) != MOTOR_STATUS_OK) {
    return false;
}

if(dm_motor_bind(MOTOR_ARM_J2, &s_motor_config[1]) != MOTOR_STATUS_OK) {
    return false;
}
```

每个 `DmMotorInstance` 独立保存

```text
model
firmware
can_id
master_id
limits
mode
feedback
ACK state
```

不同逻辑电机可以绑定不同型号和不同固件版本

### 3.3. V3 / V4

固件主版本由 assemble 明确声明

```c
.firmware = {
    .major = DM_MOTOR_FIRMWARE_V3,
}
```

或

```c
.firmware = {
    .major = DM_MOTOR_FIRMWARE_V4,
}
```

内部根据 `firmware.major` 选择

```text
V3 → dm_motor_protocol_v3
V4 → dm_motor_protocol_v4
```

业务层不需要知道 V3 / V4

同一系统可以同时存在不同固件版本的 DM 电机

### 3.4. POSITION

激活

```c
bus_motor.profile.activate(MOTOR_ARM_J1, BUS_MOTOR_PROFILE_POSITION);
```

周期发送

```c
bus_motor.pos(MOTOR_ARM_J1, target_position);
```

内部路径

```text
bus_motor.pos
    ↓
BUS_MOTOR_CMD_POSITION
    ↓
DM driver command
    ↓
检查当前 DM mode == POS_VEL
    ↓
更新 position
    ↓
protocol V3 / V4 build_control
    ↓
发送 CAN frame
```

### 3.5. VELOCITY

激活

```c
bus_motor.profile.activate(MOTOR_ARM_J1, BUS_MOTOR_PROFILE_VELOCITY);
```

周期发送

```c
bus_motor.vel(MOTOR_ARM_J1, target_velocity);
```

DM 内部使用 VEL 模式

### 3.6. TORQUE

激活

```c
bus_motor.profile.activate(MOTOR_ARM_J1, BUS_MOTOR_PROFILE_TORQUE);
```

周期发送

```c
bus_motor.tor(MOTOR_ARM_J1, target_torque);
```

DM 内部使用 MIT 模式，并构造

```text
position = 0
velocity = 0
kp = 0
kd = 0
torque = target_torque
```

目标扭矩超出当前型号 `tau_max` 时返回 `MOTOR_STATUS_INVALID_PARAM`

### 3.7. IMPEDANCE

激活

```c
bus_motor.profile.activate(MOTOR_ARM_J1, BUS_MOTOR_PROFILE_IMPEDANCE);
```

控制

```c
BusMotorImpedanceCommand command = {
    .position = target_position,
    .velocity = target_velocity,
    .kp = kp,
    .kd = kd,
    .torque = torque_ff,
};

bus_motor.imp(MOTOR_ARM_J1, &command);
```

DM 内部使用 MIT 模式

驱动检查

```text
position ∈ [-q_max, q_max]
velocity ∈ [-dq_max, dq_max]
kp ∈ [0, DM_MOTOR_MIT_KP_MAX]
kd ∈ [0, DM_MOTOR_MIT_KD_MAX]
torque ∈ [-tau_max, tau_max]
```

不同型号使用自己的 `q_max / dq_max / tau_max`

### 3.8. stop 与 brake

#### 3.8.1. stop

```c
bus_motor.basic.stop(MOTOR_ARM_J1);
```

当前 DM 后端会将

```text
velocity = 0
torque = 0
```

然后按照当前硬件模式重新发送控制帧

它不是统一意义上的硬件急停

#### 3.8.2. brake

```c
bus_motor.basic.brake(MOTOR_ARM_J1);
```

当前 DM 后端在可以支持的模式下通过保持位置实现控制制动

如果已经收到反馈，会将目标位置设置为当前反馈位置

MIT 模式下还会设置默认 `kp / kd`

当前在

```text
VEL
POS_FORCE
```

模式下返回 `MOTOR_STATUS_UNSUPPORTED`

### 3.9. POS_FORCE 私有模式

当前 POS_FORCE 没有映射成公共 Profile

原因是当前公共层还没有确定稳定的厂家无关语义与参数模型

因此继续使用达妙私有接口

先切换

```c
if(dm_motor_switch_mode(MOTOR_ARM_J1, DM_MOTOR_MODE_POS_FORCE) != MOTOR_STATUS_OK) {
    return false;
}
```

然后发送

```c
if(dm_motor_set_pos_force(MOTOR_ARM_J1, position, velocity, current) != MOTOR_STATUS_OK) {
    return false;
}
```

当前接口范围

```text
velocity 0 ~ 10000
current  0 ~ 10000
```

进入私有模式成功后，驱动会调用

```c
bus_motor_driver_reset_profile(MOTOR_ARM_J1);
```

因此

```c
bus_motor.profile.current(MOTOR_ARM_J1)
```

返回

```c
BUS_MOTOR_PROFILE_NONE
```

这样可以避免硬件已经处于 POS_FORCE，而公共层还认为处于 POSITION 或 IMPEDANCE

从 POS_FORCE 返回公共控制时重新激活 Profile

```c
bus_motor.profile.activate(MOTOR_ARM_J1, BUS_MOTOR_PROFILE_POSITION);
```

### 3.10. 清错与零点

清错

```c
dm_motor_clear_error(MOTOR_ARM_J1);
```

保存当前位置为零点

```c
dm_motor_save_zero(MOTOR_ARM_J1);
```

这两项目前属于厂家配置或维护能力

推荐放在

```text
assemble
设备诊断
标定流程
```

普通控制 service 不应直接依赖

### 3.11. Feedback

反馈首先由 RX callback 写入 DM Instance

业务读取仍然统一使用

```c
bus_motor.feedback.all(...)
bus_motor.feedback.position(...)
bus_motor.feedback.velocity(...)
bus_motor.feedback.torque(...)
bus_motor.feedback.temperature(...)
```

不要让普通业务直接读取 `DmMotorInstance`

---

## 4. 新厂家驱动接入

本章给出一个从零接入 `example_motor` 的完整流程

目标是最终让 assemble 只需要

```c
example_motor_init(&s_motor_ops, s_example_motors,
    sizeof(s_example_motors) / sizeof(s_example_motors[0]));
example_motor_bind(MOTOR_ARM_J1, &s_j1_config);
```

业务层仍然只使用 `bus_motor`

### 4.1. 建议文件结构

协议简单时

```text
bus_motor/
├── example_motor.h
└── example_motor.c
```

只有当型号表、协议版本、Codec、寄存器等内容已经明显独立时，再增加

```text
example_motor/
```

内部目录

不要为了每个小概念单独创建文件

### 4.2. 定义厂家 Config

Config 只描述真实硬件

```c
typedef struct {
    uint16_t can_id;
    ExampleMotorModel model;
    ExampleMotorFirmware firmware;
} ExampleMotorConfig;
```

业务逻辑 ID 不属于厂家 Config

它由 `bind()` 单独传入

### 4.3. 定义厂家 Instance

新厂家同样应该提供一个类型化实例存储

```c
typedef struct {
    bool used;
    BusMotorId motor_id;
    uint16_t can_id;
    ExampleMotorModel model;
    ExampleMotorFirmware firmware;
    ExampleMotorMode mode;
    BusMotorFeedback feedback;
    bool has_feedback;
} ExampleMotorInstance;
```

`ExampleMotorInstance` 的作用和 `DmMotorInstance` 一样

```text
让 assemble 可以按真实硬件数量分配静态实例内存
```

它不是业务对象

assemble 只负责声明数组，不应直接访问实例成员

### 4.4. 由 assemble 提供厂家实例池

不要在厂家驱动内部写

```c
static ExampleMotorInstance s_motors[EXAMPLE_MOTOR_MAX_COUNT];
```

而是在 assemble 中按照当前硬件真实数量声明

```c
static ExampleMotorInstance s_example_motors[2];
```

厂家驱动内部只保存这块内存的位置和容量

```c
static ExampleMotorInstance* s_instances = 0;
static uint16_t s_capacity = 0u;
static uint16_t s_count = 0u;
```

初始化接口保持和 DM 相同的模式

```c
BusMotorStatus example_motor_init(const BusMotorPortOps* ops,
    ExampleMotorInstance* instances, uint16_t capacity) {
    if(ops == 0 || ops->send == 0 || instances == 0 || capacity == 0u) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    memset(instances, 0, sizeof(instances[0]) * capacity);

    s_ops = ops;
    s_instances = instances;
    s_capacity = capacity;
    s_count = 0u;

    return MOTOR_STATUS_OK;
}
```

创建实例时只从 assemble 提供的数组中顺序取得一个 slot

```c
static BusMotorStatus example_motor_create(BusMotorId motor_id,
    const ExampleMotorConfig* config, uint16_t* instance_out) {
    ExampleMotorInstance* motor;
    uint16_t instance;

    if(config == 0 || instance_out == 0) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    if(s_count >= s_capacity) {
        return MOTOR_STATUS_NO_RESOURCE;
    }

    instance = s_count;
    motor = &s_instances[instance];

    memset(motor, 0, sizeof(*motor));
    motor->used = true;
    motor->motor_id = motor_id;
    motor->can_id = config->can_id;
    motor->model = config->model;
    motor->firmware = config->firmware;

    s_count++;
    *instance_out = instance;
    return MOTOR_STATUS_OK;
}
```

因此厂家增加多少，不会自动增加当前固件的实例 RAM

```text
仓库里有 DM / HT / DJI / ...
        ↓
当前 assemble 实际声明什么池
        ↓
当前固件才为哪些实例分配 RAM
```

### 4.5. 实现 basic()

所有 Basic 操作统一进入一个回调

```c
static BusMotorStatus example_motor_driver_basic(uint16_t instance, BusMotorBasicAction action) {
    ExampleMotorInstance* motor = example_motor_get(instance);

    if(motor == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }

    switch(action) {
        case BUS_MOTOR_BASIC_ENABLE:
            return example_motor_enable(motor);

        case BUS_MOTOR_BASIC_DISABLE:
            return example_motor_disable(motor);

        case BUS_MOTOR_BASIC_STOP:
            return example_motor_stop(motor);

        case BUS_MOTOR_BASIC_BRAKE:
            return example_motor_brake(motor);

        default:
            return MOTOR_STATUS_INVALID_PARAM;
    }
}
```

如果厂家没有 brake

```c
case BUS_MOTOR_BASIC_BRAKE:
    return MOTOR_STATUS_UNSUPPORTED;
```

不要偷偷替换成 stop

### 4.6. 实现 activate()

`activate()` 负责

```text
公共 Profile
    ↓
厂家硬件 Mode
```

例如

```c
static BusMotorStatus example_motor_driver_activate(uint16_t instance, BusMotorProfile profile) {
    ExampleMotorInstance* motor = example_motor_get(instance);

    if(motor == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }

    switch(profile) {
        case BUS_MOTOR_PROFILE_POSITION:
            return example_motor_switch_mode(motor, EXAMPLE_MODE_POSITION);

        case BUS_MOTOR_PROFILE_VELOCITY:
            return example_motor_switch_mode(motor, EXAMPLE_MODE_VELOCITY);

        case BUS_MOTOR_PROFILE_TORQUE:
            return example_motor_switch_mode(motor, EXAMPLE_MODE_TORQUE);

        case BUS_MOTOR_PROFILE_CURRENT_Q:
            return example_motor_switch_mode(motor, EXAMPLE_MODE_CURRENT_Q);

        default:
            return MOTOR_STATUS_UNSUPPORTED;
    }
}
```

如果厂家切换模式需要

```text
disable
写寄存器
等待 ACK
重新 enable
```

全部由厂家驱动完成

业务仍然只调用

```c
bus_motor.profile.activate(...)
```

### 4.7. 实现 command()

所有控制命令统一进入一个回调

```c
static BusMotorStatus example_motor_driver_command(uint16_t instance, BusMotorCommand command) {
    ExampleMotorInstance* motor = example_motor_get(instance);

    if(motor == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }

    switch(command.type) {
        case BUS_MOTOR_CMD_POSITION:
            return example_motor_set_position(motor, command.data.scalar);

        case BUS_MOTOR_CMD_VELOCITY:
            return example_motor_set_velocity(motor, command.data.scalar);

        case BUS_MOTOR_CMD_TORQUE:
            return example_motor_set_torque(motor, command.data.scalar);

        case BUS_MOTOR_CMD_CURRENT_Q:
            return example_motor_set_current_q(motor, command.data.scalar);

        default:
            return MOTOR_STATUS_UNSUPPORTED;
    }
}
```

厂家只实现自己真实支持的 Command

不支持的统一返回 `MOTOR_STATUS_UNSUPPORTED`

### 4.8. 实现 feedback()

厂家 RX callback 负责解析协议并更新 Instance

例如

```c
motor->feedback.position = parsed_position;
motor->feedback.velocity = parsed_velocity;
motor->feedback.valid =
    BUS_MOTOR_FEEDBACK_POSITION |
    BUS_MOTOR_FEEDBACK_VELOCITY;

motor->has_feedback = true;
```

Driver 回调只负责返回缓存

```c
static BusMotorStatus example_motor_driver_feedback(uint16_t instance, BusMotorFeedback* feedback) {
    ExampleMotorInstance* motor = example_motor_get(instance);

    if(motor == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }

    if(feedback == 0) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    if(motor->has_feedback == false) {
        return MOTOR_STATUS_NO_FEEDBACK;
    }

    *feedback = motor->feedback;
    return MOTOR_STATUS_OK;
}
```

### 4.9. 声明 BusMotorDriver

```c
static const BusMotorDriver s_example_motor_driver = {
    .basic = example_motor_driver_basic,
    .activate = example_motor_driver_activate,
    .command = example_motor_driver_command,
    .feedback = example_motor_driver_feedback,
    .group_command = 0,
};
```

厂家驱动不需要再复制

```text
pos
vel
tor
imp
feedback.position
feedback.velocity
```

这些公共接口

`bus_motor` 会把公共调用统一转换为最小 Driver 回调

### 4.10. 计算当前实例支持的 Profile

如果不同型号能力不同，Profile mask 必须按实例计算

不要

```c
#define EXAMPLE_MOTOR_PROFILES ALL_PROFILES
```

例如

```c
static BusMotorProfileMask example_motor_get_profiles(const ExampleMotorInstance* motor) {
    switch(motor->model) {
        case EXAMPLE_MODEL_A:
            return BUS_MOTOR_PROFILE_POSITION |
                BUS_MOTOR_PROFILE_VELOCITY;

        case EXAMPLE_MODEL_B:
            return BUS_MOTOR_PROFILE_POSITION |
                BUS_MOTOR_PROFILE_TORQUE |
                BUS_MOTOR_PROFILE_CURRENT_Q;

        default:
            return 0u;
    }
}
```

如果固件版本也影响能力，需要同时考虑 firmware

### 4.11. 实现 bind()

assemble 不应该操作 Registry 或 Driver 指针

对 assemble 只暴露

```c
example_motor_bind(motor_id, config);
```

内部

```c
BusMotorStatus example_motor_bind(BusMotorId motor_id, const ExampleMotorConfig* config) {
    uint16_t instance;
    BusMotorProfileMask profiles;
    BusMotorStatus status;

    status = example_motor_create(motor_id, config, &instance);
    if(status != MOTOR_STATUS_OK) {
        return status;
    }

    profiles = example_motor_get_profiles(example_motor_get(instance));

    status = bus_motor_driver_register(motor_id, &s_example_motor_driver, instance, profiles);
    if(status != MOTOR_STATUS_OK) {
        example_motor_destroy(instance);
        return status;
    }

    return MOTOR_STATUS_OK;
}
```

### 4.12. assemble 使用新厂家

```c
static ExampleMotorInstance s_example_motors[1];

static const ExampleMotorConfig s_j1_config = {
    .can_id = 0x01u,
    .model = EXAMPLE_MOTOR_MODEL_X,
    .firmware = EXAMPLE_MOTOR_FIRMWARE_V2,
};

bool assemble_motor(void) {
    if(bus_motor.init() != MOTOR_STATUS_OK) {
        return false;
    }

    if(example_motor_init(&s_motor_ops, s_example_motors,
            sizeof(s_example_motors) / sizeof(s_example_motors[0])) != MOTOR_STATUS_OK) {
        return false;
    }

    if(example_motor_bind(MOTOR_ARM_J1, &s_j1_config) != MOTOR_STATUS_OK) {
        return false;
    }

    return true;
}
```

如果原来是 DM

```c
dm_motor_init(&s_motor_ops, s_dm_motors,
    sizeof(s_dm_motors) / sizeof(s_dm_motors[0]));
dm_motor_bind(MOTOR_ARM_J1, &s_dm_config);
```

换成新厂家只修改 assemble

```c
example_motor_init(&s_motor_ops, s_example_motors,
    sizeof(s_example_motors) / sizeof(s_example_motors[0]));
example_motor_bind(MOTOR_ARM_J1, &s_j1_config);
```

业务继续使用

```c
bus_motor.profile.activate(MOTOR_ARM_J1, BUS_MOTOR_PROFILE_POSITION);
bus_motor.pos(MOTOR_ARM_J1, target_position);
```

---

## 5. 新能力扩展

这一章解决

> 新厂家出现一个当前 `bus_motor` 没有的功能时，应该改哪一层

### 5.1. 先判断新功能属于哪一类

```text
新功能
  │
  ├─ 是独立的运行时控制语义
  │      └─ Profile + Command
  │
  ├─ 是当前 Profile 下的控制参数
  │      └─ Command payload 或通用参数 Command
  │
  ├─ 是高频且跨厂家普遍使用的控制
  │      └─ 才考虑增加 bus_motor.xxx() 快捷接口
  │
  ├─ 是通用反馈字段
  │      └─ BusMotorFeedback + valid bit
  │
  ├─ 是多电机组语义
  │      └─ Group
  │
  └─ 完全厂家私有
         └─ 厂家 API / assemble / 专用 Adapter
```

不要看到厂家增加一个 Mode 就增加一个公共 Profile

不要看到厂家增加一个函数就增加一个 `bus_motor` API

### 5.2. 已有 Profile 与 Command，新厂家只需实现

如果公共层已经存在

```text
BUS_MOTOR_PROFILE_POSITION
BUS_MOTOR_CMD_POSITION
```

新厂家只需要

```text
activate()
command()
profile mask
```

`bus_motor.h` 不需要修改

这是最理想的扩展情况

### 5.3. 新增 Q 轴电流

假设新厂家支持独立 Q 电流控制状态

如果公共层还没有，对应增加

```text
BUS_MOTOR_PROFILE_CURRENT_Q
BUS_MOTOR_CMD_CURRENT_Q
BUS_CMD_CURRENT_Q(value)
```

构造宏例如

```c
#define BUS_CMD_CURRENT_Q(value) \
    ((BusMotorCommand){ BUS_MOTOR_CMD_CURRENT_Q, { .scalar = (value) } })
```

厂家 `activate()`

```c
case BUS_MOTOR_PROFILE_CURRENT_Q:
    return example_motor_switch_mode(motor, EXAMPLE_MODE_CURRENT_Q);
```

厂家 `command()`

```c
case BUS_MOTOR_CMD_CURRENT_Q:
    return example_motor_set_current_q(motor, command.data.scalar);
```

业务使用

```c
bus_motor.profile.activate(MOTOR_ARM_J1, BUS_MOTOR_PROFILE_CURRENT_Q);
bus_motor.cmd(MOTOR_ARM_J1, BUS_CMD_CURRENT_Q(target_iq));
```

没有新增公共函数指针

### 5.4. 新增 DQ 电压

如果厂家允许同时设置 D/Q 电压

Command payload 使用

```c
BusMotorDqCommand
```

业务

```c
bus_motor.profile.activate(MOTOR_ARM_J1, BUS_MOTOR_PROFILE_VOLTAGE_DQ);
bus_motor.cmd(MOTOR_ARM_J1, BUS_CMD_VOLTAGE_DQ(vd, vq));
```

厂家

```c
case BUS_MOTOR_CMD_VOLTAGE_DQ:
    return example_motor_set_voltage_dq(motor, command.data.dq.d, command.data.dq.q);
```

仍然不新增新的公共 API 函数

### 5.5. 加速度的两种完全不同情况

#### 5.5.1. 真正的独立加速度控制

如果厂家确实存在独立 acceleration control state

它属于

```text
Profile + Command
```

业务

```c
bus_motor.profile.activate(MOTOR_ARM_J1, BUS_MOTOR_PROFILE_ACCELERATION);
bus_motor.cmd(MOTOR_ARM_J1, BUS_CMD_ACCELERATION(target_acc));
```

#### 5.5.2. 加速度只是速度模式的限制参数

如果所谓 acceleration 只是

```text
velocity command 的 acceleration limit
```

它不是独立 Profile

不要新增

```text
BUS_MOTOR_PROFILE_ACCELERATION
```

如果多个厂家都有相同运行时语义，可以考虑新增

```text
BUS_MOTOR_CMD_SET_ACCEL_LIMIT
```

如果只是某个厂家寄存器参数，保留厂家 API

```c
example_motor_set_acceleration_limit(...);
```

并限制在 assemble、配置流程或诊断中使用

判断标准是业务语义，不是厂家给功能起了什么名字

### 5.6. 什么情况下新增高频快捷 API

目前高频快捷接口

```text
pos
vel
tor
imp
```

新增快捷 API 应同时满足

1. 具有稳定的厂家无关控制语义
2. 多个厂家普遍支持
3. 高频实时循环大量调用
4. 使用 `cmd()` 会明显降低业务代码可读性

否则保持

```c
bus_motor.cmd(...)
```

即使某个项目高频使用 Q 电流，也不代表公共层立刻需要增加

```text
bus_motor.iq()
```

### 5.7. 新增 Feedback 字段

当前公共反馈包括

```text
position
velocity
torque
temperature
error_code
```

如果新厂家还提供

```text
bus voltage
phase current
Iq
Id
encoder raw
firmware state
```

先判断是否具有稳定的厂家无关业务语义

通用反馈可以进入 `BusMotorFeedback`

例如

```text
BUS_MOTOR_FEEDBACK_BUS_VOLTAGE
feedback.bus_voltage
```

并增加对应 valid bit

厂家诊断字段例如

```text
原始编码器字段
厂家内部寄存器状态
固件专用 bit
```

继续留在厂家 Instance 与厂家诊断 API

### 5.8. 新功能最小修改速查

| 新功能 | 推荐处理 |
|---|---|
| 位置 / 速度 / 扭矩 / 阻抗等高频通用控制 | Profile + 高频快捷 API |
| Q 电流 / Q 电压 / DQ / 加速度目标等少见控制 | Profile + `cmd()` |
| 当前 Profile 下的额外目标参数 | Command payload |
| 厂家寄存器配置 | 厂家 API |
| 修改 CAN ID / Master ID | 厂家 API |
| 清错 / 保存参数 / 零点 | 厂家 API，未来确认通用语义后再考虑抽象 |
| 通用反馈字段 | `BusMotorFeedback` |
| 厂家私有诊断字段 | 厂家 Instance / 厂家 API |
| 多电机批量控制 | Group |
| 达妙一拖四 | `BusMotorDriver.group_command` |
| 厂家私有模式但具有通用控制语义 | 映射为公共 Profile |
| 厂家私有模式且没有通用语义 | Service Port / 专用 Adapter |

---

## 6. Group 与厂家特殊情况

### 6.1. 定义 Group ID

Group ID 同样由业务层定义

```c
typedef enum {
    MOTOR_GROUP_ARM = 0,
    MOTOR_GROUP_STEER,
} RobotMotorGroupId;
```

`bus_motor` 只规定底层类型

```c
typedef uint16_t BusMotorGroupId;
```

### 6.2. assemble 绑定 Group

必须先完成所有单电机 bind

```c
static const BusMotorId s_arm_group[] = {
    MOTOR_ARM_J1,
    MOTOR_ARM_J2,
    MOTOR_ARM_J3,
    MOTOR_ARM_J4,
};

if(bus_motor.group.bind(MOTOR_GROUP_ARM, s_arm_group, 4u) != MOTOR_STATUS_OK) {
    return false;
}
```

Group 不能包含未注册的电机

同一个 Group 内不能重复同一个逻辑 ID

### 6.3. Group Basic

```c
bus_motor.group.enable(MOTOR_GROUP_ARM);
bus_motor.group.disable(MOTOR_GROUP_ARM);
bus_motor.group.stop(MOTOR_GROUP_ARM);
bus_motor.group.brake(MOTOR_GROUP_ARM);
```

当前这些操作按成员顺序逐台调用单电机接口

遇到第一个错误立即返回

### 6.4. Group Profile

```c
bus_motor.group.activate(MOTOR_GROUP_ARM, BUS_MOTOR_PROFILE_POSITION);
```

当前实现逐台切换成员 Profile

如果某个成员失败，已经成功切换的前序成员不会自动回滚

因此当前 `group.activate()` 不是事务式原子操作

如果未来需要严格事务切换，需要单独增强 Group 语义

### 6.5. Group Command

准备命令

```c
BusMotorCommand commands[4] = {
    BUS_CMD_POSITION(q1),
    BUS_CMD_POSITION(q2),
    BUS_CMD_POSITION(q3),
    BUS_CMD_POSITION(q4),
};
```

普通发送

```c
bus_motor.group.cmd(MOTOR_GROUP_ARM, commands, 4u,
    BUS_MOTOR_GROUP_POLICY_DEFAULT);
```

发送前统一层检查

```text
逻辑 ID 是否存在
目标 Command 对应 Profile 是否被支持
当前 Profile 是否与 Command 匹配
```

### 6.6. Group Policy

#### 6.6.1. DEFAULT

```c
BUS_MOTOR_GROUP_POLICY_DEFAULT
```

语义

```text
厂家有原生 group_command
    ↓
优先尝试厂家原生 Group

厂家没有或明确返回 UNSUPPORTED
    ↓
允许逐台 fallback
```

DEFAULT 不保证严格同步

#### 6.6.2. SYNCHRONIZED

```c
BUS_MOTOR_GROUP_POLICY_SYNCHRONIZED
```

表示业务要求厂家后端真正提供同步组控制

无法保证时返回

```text
MOTOR_STATUS_UNSUPPORTED
```

不能自动退化成逐台发送

#### 6.6.3. ATOMIC

```c
BUS_MOTOR_GROUP_POLICY_ATOMIC
```

表示业务要求原子组命令语义

厂家后端没有这种保证时必须返回 `MOTOR_STATUS_UNSUPPORTED`

### 6.7. 当前 DM Group 真实状态

`bus_motor.group` 已经预留 `group_command()`

但是当前 `s_dm_motor_driver` 尚未实现该回调

因此当前 DM 行为

```text
DEFAULT
    ↓
逐台 fallback

SYNCHRONIZED
    ↓
MOTOR_STATUS_UNSUPPORTED

ATOMIC
    ↓
MOTOR_STATUS_UNSUPPORTED
```

当前接口存在不代表达妙原生一拖四已经实现

### 6.8. 达妙一拖四应该怎么接

一拖四属于

```text
多电机通信 / Group 能力
```

不属于某一台 DM 电机的单电机公共能力

正确接入位置

```c
static BusMotorStatus dm_motor_driver_group_command(const uint16_t* instances,
    const BusMotorCommand* commands, uint8_t count, BusMotorGroupPolicy policy) {
    return MOTOR_STATUS_UNSUPPORTED;
}
```

实现后挂入

```c
static const BusMotorDriver s_dm_motor_driver = {
    .basic = dm_motor_driver_basic,
    .activate = dm_motor_driver_activate,
    .command = dm_motor_driver_command,
    .feedback = dm_motor_driver_feedback,
    .group_command = dm_motor_driver_group_command,
};
```

业务仍然调用

```c
bus_motor.group.cmd(...);
```

不新增

```text
dm_motor.one_to_four()
```

这使一拖四只成为 DM 对公共 Group 语义的一种厂家优化

### 6.9. 厂家特殊功能的三种处理

#### 6.9.1. 有厂家无关控制语义

假设厂家 `MODE_7` 本质是阻抗控制

不要暴露

```text
MODE_7
```

应该映射到

```text
BUS_MOTOR_PROFILE_IMPEDANCE
```

#### 6.9.2. 厂家配置或诊断功能

例如

```text
改 CAN ID
改 Master ID
读写寄存器
保存 Flash
读固件版本
清除厂家错误
设置零点
```

保留厂家 API

```c
example_motor_xxx();
```

只在

```text
assemble
设备管理
诊断
标定
```

使用

#### 6.9.3. 业务运行时必须依赖的纯厂家特殊功能

这是最难处理的情况

假设某个任务状态必须进入厂家独有 `SPECIAL_MODE_X`

并且无法合理解释成 POSITION、TORQUE、IMPEDANCE、CURRENT_Q 等通用控制语义

不要直接让业务写

```c
example_motor_switch_mode(MOTOR_ARM_J1, SPECIAL_MODE_X);
```

否则业务重新与厂家耦合

推荐由对应 service 定义很小的任务能力接口

```c
typedef struct {
    bool (*enter_special_control)(void);
    bool (*exit_special_control)(void);
} TaskMotorPort;
```

assemble 提供 Adapter

```text
TaskMotorPort
    ↓
Example Motor Adapter
    ↓
SPECIAL_MODE_X
```

业务继续使用

```c
task_motor.enter_special_control();
```

未来换厂家只替换 Adapter

这种能力不能因为单个厂家存在就永久扩张 `bus_motor`

### 6.10. 运行时直接进入厂家私有模式

某些项目确实需要临时直接使用厂家模式

允许

```c
example_motor_switch_mode(MOTOR_ARM_J1, EXAMPLE_MODE_SPECIAL);
```

但是成功后厂家驱动必须同步执行

```c
bus_motor_driver_reset_profile(MOTOR_ARM_J1);
```

之后

```c
bus_motor.profile.current(MOTOR_ARM_J1)
```

应该返回 `BUS_MOTOR_PROFILE_NONE`

公共

```c
bus_motor.pos(...)
bus_motor.imp(...)
```

也应该因为 Profile 不匹配而拒绝执行

任务结束后重新进入公共语义

```c
bus_motor.profile.activate(MOTOR_ARM_J1, BUS_MOTOR_PROFILE_POSITION);
```

### 6.11. 硬件不能满足语义时不要静默降级

例如没有 brake 能力时

错误做法

```text
brake → stop
```

或者

```text
brake → disable
```

正确做法

```text
MOTOR_STATUS_UNSUPPORTED
```

如果业务允许退化，业务层显式写出策略

```c
if(bus_motor.basic.brake(id) == MOTOR_STATUS_UNSUPPORTED) {
    bus_motor.basic.stop(id);
}
```

这样硬件差异始终可见

### 6.12. 不把当前 Registry 当热插拔框架

当前设计重点是

```text
启动阶段 assemble 绑定
```

不推荐运行过程中频繁 unregister / register 来动态替换厂家实例

真正热插拔还需要额外处理

```text
生命周期
失联恢复
Profile 状态重建
Group 重绑定
```

这些不属于当前静态 Registry 的职责

---

## 7. 状态码、实现边界与设计原则

### 7.1. 常用状态码

```text
MOTOR_STATUS_OK
MOTOR_STATUS_ERROR
MOTOR_STATUS_INVALID_PARAM
MOTOR_STATUS_PORT_ERROR
MOTOR_STATUS_TIMEOUT
MOTOR_STATUS_ID_MISMATCH
MOTOR_STATUS_NO_INSTANCE
MOTOR_STATUS_NOT_INITIALIZE
MOTOR_STATUS_UNSUPPORTED
MOTOR_STATUS_NO_FEEDBACK
MOTOR_STATUS_NOT_FOUND
MOTOR_STATUS_ALREADY_BOUND
MOTOR_STATUS_NO_RESOURCE
MOTOR_STATUS_PROFILE_MISMATCH
```

推荐统一记录

```c
BusMotorStatus status =
    bus_motor.profile.activate(MOTOR_ARM_J1, BUS_MOTOR_PROFILE_IMPEDANCE);

if(status != MOTOR_STATUS_OK) {
    log_error("motor activate failed: %s", bus_motor.status_str(status));
}
```

常见含义

**`MOTOR_STATUS_UNSUPPORTED`**

硬件或后端没有能力实现请求语义

**`MOTOR_STATUS_PROFILE_MISMATCH`**

能力存在，但当前没有激活与 Command 对应的 Profile

**`MOTOR_STATUS_NO_FEEDBACK`**

实例存在，但尚未收到有效反馈

**`MOTOR_STATUS_NOT_FOUND`**

业务逻辑 ID 没有绑定厂家实例

### 7.2. 当前已经实现

```text
BusMotorId Registry
单一 bus_motor 对外接口
Basic enable / disable / stop / brake
Profile supports / require / activate / current
Position / Velocity / Torque / Impedance 高频入口
统一 cmd()
完整 Feedback 与字段读取
Group bind / basic / activate / cmd / feedback
BusMotorDriver 最小分发
DM 型号 Instance
DM V3 / V4 protocol 选择
DM POSITION / VELOCITY / TORQUE / IMPEDANCE 映射
DM 私有 POS_FORCE
DM 清错 / 零点 / 私有模式切换
```

### 7.3. 当前尚未实现或尚未完成验证

```text
达妙原生一拖四 group_command
DM POS_FORCE 的公共厂家无关 Profile / Command 语义
其他厂家真实驱动
CURRENT_Q / VOLTAGE_Q / CURRENT_DQ / VOLTAGE_DQ / ACCELERATION 的真实厂家后端
Group Profile 切换事务回滚
热插拔厂家实例
```

公共接口中预留能力不等价于已经有厂家后端支持

最终应以

```c
bus_motor.profile.supports(...)
```

以及具体 Driver 实现为准

### 7.4. 各层允许知道的信息

#### 7.4.1. 业务层

允许知道

```text
逻辑 Motor ID
逻辑 Group ID
公共 Profile
公共 Command
公共 Feedback
```

不应该知道

```text
厂家名
CAN ID
Master ID
固件协议
厂家 Mode
厂家 Instance
```

#### 7.4.2. assemble

允许知道

```text
厂家
型号
固件
CAN ID
Master ID
初始硬件配置
厂家初始化流程
```

assemble 是硬件 Composition Root

厂家信息出现在这里不是耦合失败

#### 7.4.3. 厂家驱动

允许知道全部厂家实现细节

```text
硬件 Mode
寄存器
编码范围
协议版本
CAN Frame
ACK
厂家特殊能力
```

### 7.5. 最终设计原则

统一的是业务能力，不是厂家模式

Profile 表示当前运行控制语义

Command 表示当前控制目标

高频通用能力使用短接口

低频和扩展能力统一使用 `cmd()`

厂家 Mode 永远由具体 Driver 解释

厂家私有配置保留厂家 API

厂家特殊组优化通过 `group_command()` 接入

无法抽象成通用电机语义的任务特殊能力通过 Service Port / Adapter 隔离

业务必须依赖的能力在初始化阶段通过 `require()` 验证

硬件不能满足公共语义时明确返回 `MOTOR_STATUS_UNSUPPORTED`，不做静默降级
