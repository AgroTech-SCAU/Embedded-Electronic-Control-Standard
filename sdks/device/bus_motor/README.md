# bus_motor

`bus_motor` 是 AgroTech 嵌入式标准中的**总线电机统一能力接口**

它不试图统一所有厂家协议，而是统一业务层真正依赖的控制语义：逻辑电机 ID、控制 Profile、命令、反馈、基础生命周期操作和 Group

```text
业务层
  │  logical motor id + common semantics
  ▼
bus_motor
  │  registry / profile / command / feedback
  ▼
厂家驱动（当前：dm_motor）
  │  protocol + instance + device-specific config
  ▼
PortOps
  │
  ▼
CAN / UART / 其他平台能力
```

核心目标：**更换电机厂家时，把变化限制在设备适配与 assemble，业务层尽量保持不变**

---

## 1. 先判断你是哪类使用者

| 角色 | 建议阅读 |
|---|---|
| 业务开发者 | 本 README 的 2、5、6 节 |
| 工程集成人员 | 本 README 全文 + `validation/bus_motor_dm_stm32/` |
| 新厂家驱动开发者 | 本 README + [`DESIGN.md`](DESIGN.md) |
| 修改 Profile / Command / Group 的维护者 | 直接阅读 [`DESIGN.md`](DESIGN.md) |

普通业务代码应优先只包含：

```c
#include "bus_motor/bus_motor.h"
```

只有 assemble、厂家配置、诊断或厂家私有能力需要包含：

```c
#include "bus_motor/dm_motor.h"
```

---

## 2. Quick Start

### 2.1 定义业务逻辑电机 ID

`BusMotorId` 表示**业务角色**，不是 CAN ID

```c
typedef enum {
    MOTOR_ARM_J1 = 0,
    MOTOR_ARM_J2,
} RobotMotorId;
```

推荐关系：

```text
MOTOR_ARM_J1
    ↓ assemble
DmMotorConfig
    ↓
CAN ID / Master ID / Model / Firmware
```

以后换厂家时仍可保留 `MOTOR_ARM_J1`

### 2.2 提供底层 PortOps

厂家驱动通过 `BusMotorPortOps` 使用平台总线能力；DM 当前至少需要 `send`；模式切换等待 ACK 时还需要时间与延时能力

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

`bus_motor` 本身不应直接依赖 STM32 HAL；上面的平台函数属于成员工程 `platform/` 或独立 chip SDK

### 2.3 配置并绑定真实 DM 电机

```c
static DmMotorInstance s_dm_instances[1];

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

初始化顺序：

```c
if(bus_motor.init() != MOTOR_STATUS_OK) {
    return false;
}

if(dm_motor_init(&s_motor_ops,
                 s_dm_instances,
                 sizeof(s_dm_instances) / sizeof(s_dm_instances[0])) != MOTOR_STATUS_OK) {
    return false;
}

if(dm_motor_bind(MOTOR_ARM_J1, &s_j1_config) != MOTOR_STATUS_OK) {
    return false;
}
```

职责边界：

```text
assemble        提供实例内存和真实硬件配置
bus_motor.init  清空公共 registry
dm_motor_init   接管厂家实例池
dm_motor_bind   建立 logical id ↔ DM instance 映射
```

### 2.4 接入 RX

DM 反馈和模式切换 ACK 必须持续进入厂家驱动

```c
static void motor_can_rx(FDCAN_HandleTypeDef* hcan,
                         const FDCAN_RxHeaderTypeDef* header,
                         const uint8_t data[8],
                         void* user) {
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

参数帧必须先解析：

```text
RX frame
  ↓
dm_motor_parse_parameter_frame()
  ├─ true  → 参数帧 / ACK，结束
  └─ false → dm_motor_parse_feedback_frame()
```

在调用 `bus_motor.profile.activate()` 前，应保证 RX callback 已经正常工作

### 2.5 检查业务要求并启动

例如业务要求位置与阻抗能力：

```c
BusMotorProfileMask required =
    BUS_MOTOR_PROFILE_POSITION |
    BUS_MOTOR_PROFILE_IMPEDANCE;

if(bus_motor.profile.require(MOTOR_ARM_J1, required) != MOTOR_STATUS_OK) {
    return false;
}
```

然后完成厂家初始化、公共 Profile 激活和使能：

```c
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
```

`DmMotorConfig.default_mode` 只描述厂家实例绑定时认定的硬件初始模式，不等价于公共层已经建立当前 Profile，因此仍建议显式 `activate()`

---

## 3. 常用 API

### 3.1 高频控制

```c
bus_motor.pos(MOTOR_ARM_J1, target_position);
bus_motor.vel(MOTOR_ARM_J1, target_velocity);
bus_motor.tor(MOTOR_ARM_J1, target_torque);
bus_motor.imp(MOTOR_ARM_J1, &command);
```

少见但具有公共语义的命令统一通过：

```c
bus_motor.cmd(MOTOR_ARM_J1, command);
```

### 3.2 基础生命周期

```c
bus_motor.basic.enable(MOTOR_ARM_J1);
bus_motor.basic.disable(MOTOR_ARM_J1);
bus_motor.basic.stop(MOTOR_ARM_J1);
bus_motor.basic.brake(MOTOR_ARM_J1);
```

注意：`stop()` 是统一控制语义，不代表所有厂家都具有相同的硬件急停实现；真实系统仍必须设计独立的急停与安全链路

### 3.3 Profile

```c
bus_motor.profile.supports(MOTOR_ARM_J1, profile);
bus_motor.profile.require(MOTOR_ARM_J1, required_mask);
bus_motor.profile.activate(MOTOR_ARM_J1, profile);
bus_motor.profile.current(MOTOR_ARM_J1, &profile);
```

推荐用法：

- `supports()`：查询能力
- `require()`：初始化阶段验证“这套硬件能不能满足业务”
- `activate()`：进入某个公共控制状态
- `current()`：查询公共层认定的当前状态

不要把厂家 Mode 直接等同于公共 Profile

### 3.4 Feedback

```c
float position;
BusMotorFeedback feedback;

bus_motor.feedback.position(MOTOR_ARM_J1, &position);
bus_motor.feedback.all(MOTOR_ARM_J1, &feedback);
```

无有效反馈时接口应返回明确状态，而不是用默认 `0` 冒充真实反馈

---

## 4. Profile 与 Command

公共层把“运行时控制状态”和“控制目标”分开：

```text
Profile = 当前电机处于什么公共控制状态
Command = 这一次要提交什么控制目标
```

例如：

```text
POSITION  + position command
VELOCITY  + velocity command
TORQUE    + torque command
IMPEDANCE + impedance command
```

如果当前 Profile 与 Command 不匹配，应返回明确错误，而不是静默切换或偷偷降级

新增厂家功能时也不要看到一个厂家 Mode 就增加一个公共 Profile；详细判断规则见 [`DESIGN.md`](DESIGN.md)

---

## 5. DM 当前支持范围

当前 DM 驱动是 `bus_motor` 的第一个厂家实现，支持 V3 / V4 协议按实例选择

公共 Profile 映射以当前代码为准，主要包括：

```text
POSITION   → DM position/velocity 类模式
VELOCITY   → DM velocity 模式
TORQUE     → DM torque 类控制
IMPEDANCE  → DM MIT/impedance 类控制
```

业务层不需要知道：

```text
V3 / V4
RID10
DM_MOTOR_MODE_*
具体帧布局
```

这些信息应停留在 `dm_motor` 与厂家协议实现中

厂家专用能力（例如没有稳定厂家无关语义的模式、清错、零点、诊断）可以留在 `dm_motor.h`，不要为了“接口看起来统一”强行塞进公共层

---

## 6. Group

Group ID 同样由业务层定义：

```c
typedef enum {
    MOTOR_GROUP_ARM = 0,
} RobotMotorGroupId;
```

公共接口提供 Group 的基础操作、Profile 与 Command 语义

```c
bus_motor.group.enable(MOTOR_GROUP_ARM);
bus_motor.group.disable(MOTOR_GROUP_ARM);
bus_motor.group.activate(MOTOR_GROUP_ARM, BUS_MOTOR_PROFILE_POSITION);
```

Group policy 用于表达业务要求，例如普通逐台发送、同步语义或原子语义

**当前接口存在不代表厂家后端已经实现原生同步/原子发送；** 当前 DM Group 的真实实现状态与一拖四接入设计见 [`DESIGN.md`](DESIGN.md)

---

## 7. 工程接入建议

成员工程建议保持：

```text
MyProject/
├── external/
│   └── Embedded-Electronic-Control-Standard/
├── src/
│   ├── app/
│   ├── service/
│   │   └── assemble/
│   ├── device/
│   ├── domain/
│   ├── infra/
│   └── platform/
└── ...
```

构建时直接加入本仓库正式 SDK 源文件与 include path，不建议复制一份 SDK 到项目目录后继续修改

本仓库内已有一个真实硬件验证资产：

```text
validation/bus_motor_dm_stm32/
```

它用于验证 `STM32 + FDCAN + DM + bus_motor` 的完整集成链路，不属于教学 example；具体硬件假设、构建引用和测试流程见该目录 README

---

## 8. 新厂家接入

如果只是使用已有厂家驱动，不需要阅读这一节

新增厂家通常需要：

1. 定义厂家 `Config`
2. 定义厂家 `Instance`
3. 由 assemble 提供实例池
4. 实现公共 `basic / activate / command / feedback`
5. 声明 `BusMotorDriver`
6. 计算实例真实支持的 Profile
7. `bind(logical_id, config)` 注册到 `bus_motor`

不要让业务层直接读取厂家 Instance

完整模板、扩展新 Profile/Command 的判断规则、Group backend 设计与厂家特殊能力处理见：

- [`DESIGN.md`](DESIGN.md)

---

## 9. 当前边界

`bus_motor` 当前目标不是：

- 热插拔设备框架
- 自动探测所有电机型号
- 统一所有厂家私有配置
- 用软件接口替代硬件急停
- 对不支持的能力进行静默 fallback

当前 Registry 更适合**启动阶段静态装配、运行阶段稳定使用**

如果硬件不能满足业务要求，应返回 `MOTOR_STATUS_UNSUPPORTED` 或其他明确状态，让上层决定是否允许退化

---

## 10. 安全要求

涉及真实电机测试时至少满足：

- 有可确认的 disable / stop / fault 路径
- 有反馈链路与超时策略
- 有位置、速度、扭矩/电流等边界
- 首次测试使用低速、低目标、小负载
- 明确硬件急停链路
- 记录测试对应的 commit
- 不在 ISR 中执行阻塞模式切换或复杂业务逻辑

验证工程只是软件集成参考，不代表接上真实机械负载后自动满足整机安全要求

---

## 11. 文件结构

```text
bus_motor/
├── README.md                # 普通用户与集成人员入口
├── DESIGN.md                # 维护者、厂家适配与设计细节
├── bus_motor.h
├── bus_motor.c
├── dm_motor.h
├── dm_motor.c
└── dm_motor/
    ├── dm_motor_core.h
    ├── dm_motor_core.c
    ├── dm_motor_protocol.h
    ├── dm_motor_protocol_v3.c
    └── dm_motor_protocol_v4.c
```

依赖原则：

```text
service/assemble
      │
      ▼
 bus_motor API
      │
      ▼
 BusMotorDriver
      │
      ▼
 dm_motor / other vendor
      │
      ▼
    PortOps
```

**普通业务依赖公共语义；真实硬件差异留在 assemble 与厂家驱动**
