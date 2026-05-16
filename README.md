<div align="center">

# Embedded-Electronic-Control-Standard

</div>

> AgroTech 协会嵌入式电控开发标准：用于统一电控项目的目录分层、开发顺序、SDK 复用方式、Git/GitHub 协作流程、芯片平台适配方式和安全调试规范

---

## 1. 当前状态

```text
Status: Draft v0.1
用途: AgroTech 协会内部标准建设与项目迁移试用
稳定性: 文档结构基本确定，SDK API 仍允许调整
```

---

## 2. 快速导航

| 文件/目录 | 作用 |
|---|---|
| `README.md` | 总览、快速开始、submodule 使用方式、仓库结构说明 |
| `plan.md` | 标准建设计划、阶段目标、后续要补齐的内容 |
| `通用开发流文档.md` | 架构分层、开发顺序、模块边界、安全约束 |
| `团队协作开发文档.md` | Git/GitHub、分支、commit、PR、issue、submodule 协作规范 |
| `sdks/infra/` | 通用基础设施 SDK，例如 delay、matrix、PID、parser、HFSM、log |
| `sdks/domain/` | 领域/算法 SDK，例如机械臂运动学、舵轮运动学 |
| `sdks/device/` | 常用真实设备 SDK，例如 bus_motor、rgb_led，后续用于 bus_servo、encoder、IMU 等 |

---

## 3. 核心分层标准

**AgroTech 协会嵌入式电控工程统一采用以下从上到下的分层结构**：

```text
src/
├── app/        # 1. 应用层：任务入口、业务流程、状态机编排、整机逻辑
├── service/    # 2. 服务层：组合 device + domain + infra，形成系统能力
├── device/     # 3. 设备层：真实设备 SDK、设备协议、反馈缓存、设备抽象
├── domain/     # 4. 领域层：运动学、控制模型、机构解算、纯算法模型
├── infra/      # 5. 基础设施层：PID、滤波、矩阵、协议解析、HFSM、CRC、容器
└── platform/   # 6. 平台层：芯片/HAL/FSP/CubeMX 外设适配、CAN/UART/PWM/GPIO/Tick
```

**基本依赖规则**：

- 下层不得依赖上层
- app 只依赖 service 和少量 infra
- service 可依赖 device/domain/infra，并在自身 init 中完成 device/infra 与 platform 的 PortOps 对接
- device/infra 优先把 PortOps 合并进 init 配置，避免 register 和 init 分开调用导致漏绑定
- domain 可依赖 infra，但不应依赖 device/platform
- infra 应尽量无业务和芯片依赖，必要时通过 PortOps 或注册函数连接 platform
- platform 是唯一允许直接包含 HAL/FSP/CMSIS/CubeMX 头文件的层
- SDK 中所有二值语义统一使用 `bool` / `true` / `false`，并包含 `<stdbool.h>`；不要用 `uint8_t`、`int` 或 `0/1` 表示布尔状态

**另外**：
- service 负责 device/infra 与 platform 之间的对接，并完成系统能力的组合、缓存和安全策略
- 如需提升性能或确定设备无法复用，则允许 device 直接依赖 platform，但应限制在内部实现，不暴露给上层

---

## 4. 仓库使用方式

### 4.1 标准仓库自身

**本仓库自身保存**：

```text
Embedded-Electronic-Control-Standard/
├── README.md
├── plan.md
├── 通用开发流文档.md
├── 团队协作开发文档.md
└── sdks/
    ├── infra/
    ├── domain/
    └── device/
```

### 4.2 成员项目如何引用本标准

**成员自己的项目建议把本仓库作为 submodule 放在 `external/` 下**：

```text
My-Embedded-Project/
├── README.md
├── external/
│   ├── Embedded-Electronic-Control-Standard/   # submodule，本仓库
│   └── Embedded-Chip-STM32-HAL-SDK/        	# 可选，按平台添加
├── src/
│   ├── app/
│   ├── service/
│   ├── device/
│   ├── domain/
│   ├── infra/
│   └── platform/
└── docs/
```

**添加本标准仓库，并固定到指定 tag 或 commit**：

```bash
# 先添加 submodule
git submodule add https://github.com/Kaede-Rei/Embedded-Electronic-Control-Standard.git external/Embedded-Electronic-Control-Standard

# 再进入 submodule，切到项目需要固定的 tag
cd external/Embedded-Electronic-Control-Standard
git fetch --tags
git switch --detach refs/tags/<tag-name>     # 例如 refs/tags/v1.0.0

# 回到父项目，提交 submodule 指针
cd ../..
git add .gitmodules external/Embedded-Electronic-Control-Standard
git commit -m "chore(submodule): add embedded electronic control standard"
```

**克隆带有本标准 submodule 的项目**：

```bash
git clone --recurse-submodules <project-url>
```

**如果已经普通 clone，则通过以下指令重新添加本标准仓库**：

```bash
git submodule update --init --recursive
```

---

## 5. 日常更新方式

### 5.1 普通成员：更新到项目锁定版本

**普通成员一般不需要手动追标准仓库最新 `main`，只需要跟随当前项目锁定的 submodule 版本**：

```bash
git pull --recurse-submodules
git submodule update --init --recursive
```

**含义**：

```text
父项目决定当前应该使用哪个标准版本
普通成员同步父项目后，submodule 更新到父项目记录的 commit
```

### 5.2 项目维护者：更新项目使用的标准版本

**当项目需要升级本标准仓库到新版本时，由项目维护者执行**：

```bash
cd external/Embedded-Electronic-Control-Standard
git fetch --tags origin
git switch --detach refs/tags/<tag-name>     # 例如 refs/tags/v1.0.0

cd ../..
git status
git add external/Embedded-Electronic-Control-Standard
git commit -m "chore(submodule): update embedded electronic control standard"
git push
```

然后走项目 PR；合并后，其他成员再执行普通更新命令即可

### 5.3 不推荐普通成员直接执行的命令

```bash
git submodule update --remote --recursive
```

该命令会让 submodule 直接追远程分支最新提交，可能绕过项目维护者锁定和验证过的版本；只有维护者在明确要升级标准版本时才应该使用类似操作

---

## 6. 芯片平台 SDK 的使用方式

本仓库不直接内置 STM32、ESP32、Renesas、GD32 等具体芯片平台 SDK

**原因**：

1. 芯片平台适配强依赖 HAL/FSP/CubeMX/芯片工程生成代码
2. 不同平台的时钟、中断、DMA、Cache、串口、CAN 初始化差异很大
3. 全部塞进本标准仓库会使仓库膨胀且边界混乱

**推荐做法**：

```text
本仓库：保存通用标准和通用 SDK
chip SDK 仓库：保存某个芯片平台的 PortOps 适配、注意事项和最小示例
成员项目：同时 submodule 本仓库和对应 chip SDK
```

**例如 STM32 项目**：

```bash
git submodule add https://github.com/Kaede-Rei/Embedded-Chip-STM32-HAL-SDK.git external/Embedded-Chip-STM32-HAL-SDK

cd external/Embedded-Chip-STM32-HAL-SDK
git fetch --tags
git switch --detach refs/tags/<tag-name>     # 例如 refs/tags/v1.0.0

cd ../..
git add .gitmodules external/Embedded-Chip-STM32-HAL-SDK
git commit -m "chore(submodule): add stm32 hal chip sdk"
```

**目前协会已有的芯片 SDK**：

```text
暂无
```

---

## 7. 当前已包含的 SDK

### 7.1 infra

```text
sdks/infra/
├── delay.c / delay.h
├── matrix.c / matrix.h
├── pid.c / pid.h
├── protocol_parser.c / protocol_parser.h
├── log.c / log.h
└── hfsm/
```

> 定位：与真实硬件尽量解耦的基础设施模块

### 7.2 domain

```text
sdks/domain/
├── six_dof_arm_kine.c / six_dof_arm_kine.h
└── steer_wheel_kine.c / steer_wheel_kine.h
```

> 定位：领域/算法模块，主要负责运动学、机构几何、控制模型等硬件无关逻辑

### 7.3 device

```text
sdks/device/
├── bus_motor/
│   ├── bus_motor.c / bus_motor.h
│   └── dm_bus_motor.c / dm_bus_motor.h
└── rgb_led/
    ├── rgb_led.c / rgb_led.h
    └── ws2812_rgb_led.c / ws2812_rgb_led.h
```

设备 SDK 必须通过 PortOps/注册函数与平台层对接，不应在 public header 中直接暴露 `stm32xxx_hal.h`、`hal_data.h` 等芯片头文件

---

## 8. 安全提醒

**涉及电机、舵机、底盘、机械臂、夹爪等执行机构时，禁止在以下条件下直接上真实硬件**：

1. 没有 stop/brake/fault 逻辑
2. 没有反馈超时判断
3. 没有速度/角度/电流/位置限幅
4. 没有低速测试
5. 没有确认急停链路
6. 没有记录测试 commit
7. app 层直接拼底层控制帧
8. 中断中执行复杂控制和阻塞等待

**默认原则**：

```text
先安全，后功能
先低速，后高速
先单设备，后整机
先 mock/板级测试，后真实负载测试
```

---

## 9. 贡献方式

**推荐流程**：

```text
issue → branch → commit → PR → review → merge
```

**分支命名示例**：

```text
docs/readme-overview
sdk/infra-hfsm-cleanup
sdk/domain-steer-wheel-kine
refactor/device-portops-template
chip/stm32-fdcan-notes
```

**commit 示例**：

```text
docs(readme): add submodule usage guide
sdk(domain): add steer wheel kinematics interface
refactor(device): introduce portops template
```
