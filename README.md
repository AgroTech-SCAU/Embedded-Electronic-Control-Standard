<div align="center">

# Embedded-Electronic-Control-Standard

</div>

> AgroTech 协会嵌入式电控开发标准：用于统一电控项目的目录分层、开发顺序、SDK 复用方式、Git/GitHub 协作流程、芯片平台适配方式和安全调试规范

> [!IMPORTANT]
> 参与开发前阅读 [协作指南](.github/CONTRIBUTING.md)
> Issue → Branch → Commit → Push → Pull Request → 项目负责人 Merge
> 正式 SDK 仅包含 infra、domain、device，app、service、service/assemble、platform 由成员项目维护

---

## 1. 当前状态

```text
Status: Draft v0.2
用途: AgroTech 协会内部标准建设、项目迁移试用与文档口径统一
稳定性: 文档原则基本确定；SDK 代码按模块逐步同步，public API 在 v1.0 前仍允许调整
```

---

## 2. 快速导航

| 文件/目录 | 作用 |
|---|---|
| `README.md` | 总览、快速开始、submodule 使用方式、仓库结构说明 |
| [docs/步骤.md](docs/步骤.md) | 当前实施计划与步骤完成状态 |
| [plan.md](plan.md) | 历史建设路线与后续参考 |
| [docs/architecture.md](docs/architecture.md) | 架构分层、公共 SDK 边界、PortOps、assemble 和安全约束 |
| [.github/CONTRIBUTING.md](.github/CONTRIBUTING.md) | 唯一协作规范入口 |
| [.github/rulesets/README.md](.github/rulesets/README.md) | main 保护规则导入与权限说明 |
| `sdks/infra/` | 通用基础设施 SDK，例如 delay、matrix、PID、parser、HFSM、log |
| `sdks/domain/` | 领域/算法 SDK，例如机械臂运动学、舵轮运动学 |
| `sdks/device/` | 常用真实设备 SDK，例如 bus_motor、bus_servo、imu、rgb_led 等；具体代码以当前 `sdks/` 目录实际同步状态为准 |
| `examples/` | 最小教学示例：用于说明标准、接口和模块组织方式 |
| `validation/` | 真实平台/硬件集成验证资产，例如 STM32 + DM 电机验证工程 |

---

## 3. 核心分层标准

成员项目采用 `app / service / device / domain / infra / platform` 六层架构

Standard 只维护公共 SDK 三层 `infra / domain / device`，成员项目自己维护 `app / service / service/assemble / platform`

项目 `service/assemble` 将 platform 的 CAN/UART/Tick 等能力以 PortOps 或等价依赖注入方式绑定到公共 SDK，不把 HAL 句柄、安装补偿或比赛业务写入 SDK

完整依赖规则和设计方法见 [架构说明](docs/architecture.md)

---

## 4. 仓库使用方式

### 4.1 标准仓库自身

**本仓库自身保存**

```text
Embedded-Electronic-Control-Standard/
├── README.md
├── plan.md
├── docs/
│   ├── architecture.md
│   └── 步骤.md
├── .github/              # Issue Forms、PR、CONTRIBUTING、Ruleset
├── .githooks/            # 本地防误操作提醒
├── setup-scripts/
├── sdks/
│   ├── infra/
│   ├── domain/
│   └── device/
├── examples/              # 最小教学示例
└── validation/            # 真实平台/硬件集成验证
```

### 4.2 成员项目如何引用本标准

**成员自己的项目建议把本仓库作为 submodule 放在 `external/` 下**

```text
My-Embedded-Project/
├── README.md
├── external/
│   └── Embedded-Electronic-Control-Standard/   # submodule，本仓库
├── src/
│   ├── app/
│   ├── service/
│   ├── device/
│   ├── domain/
│   ├── infra/
│   └── platform/
└── docs/
```

**添加本标准仓库，并固定到指定 tag 或 commit**

```bash
# 先添加 submodule
git submodule add https://github.com/AgroTech-SCAU/Embedded-Electronic-Control-Standard.git external/Embedded-Electronic-Control-Standard

# 再进入 submodule，切到项目需要固定的 tag
cd external/Embedded-Electronic-Control-Standard
git fetch --tags
git switch --detach refs/tags/<tag-name>     # 例如 refs/tags/v1.0.0

# 回到父项目，提交 submodule 指针
cd ../..
git add .gitmodules external/Embedded-Electronic-Control-Standard
git commit -m "chore(submodule): add embedded electronic control standard"
```

**克隆带有本标准 submodule 的项目**

```bash
git clone --recurse-submodules <project-url>
```

**如果已经普通 clone，则通过以下指令重新添加本标准仓库**

```bash
git submodule update --init --recursive
```

---

## 5. 日常更新方式

### 5.1 普通成员：更新到项目锁定版本

**普通成员一般不需要手动追标准仓库最新 `main`，只需要跟随当前项目锁定的 submodule 版本**

```bash
git pull --recurse-submodules
git submodule update --init --recursive
```

**含义**

```text
父项目决定当前应该使用哪个标准版本
普通成员同步父项目后，submodule 更新到父项目记录的 commit
```

### 5.2 项目维护者：更新项目使用的标准版本

**当项目需要升级本标准仓库到新版本时，由项目维护者执行**

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

## 6. 芯片平台适配

平台适配由成员项目自己的 `src/platform/` 维护，项目 `service/assemble/` 负责注入 PortOps

本仓库不提供项目级 app、service 或公共 platform，不要求额外创建 Chip SDK 仓库

`validation/` 中的板级适配和 CubeMX 配置只用于复现硬件验证

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
├── hfsm/
└── status.h              # 若暂未落地，应作为待补齐项追踪
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
├── bus_motor/            # 总线电机统一入口；具体实例按厂家独立配置
├── bus_servo/            # 总线舵机统一入口；具体实例按协议独立配置
├── imu/                  # IMU 统一入口；具体实例按芯片独立配置
└── rgb_led/              # RGB 灯统一入口
```

设备 SDK 必须通过 `init(config with PortOps)` 或等价注入方式与平台层对接，不应在 public header 中直接暴露 `stm32xxx_hal.h`、`hal_data.h` 等芯片头文件；统一接口不应强行固定具体厂家的 config 字段；配置差异明显时，统一入口使用 `const void* config`，具体驱动自行定义 `XxxConfig`

`sdks/infra/status.h` 是推荐沉淀方向，用于统一基础状态码、生命周期状态和字符串辅助；若当前代码尚未同步该文件，应在 plan/issue 中作为待补齐项追踪

---

## 8. 安全提醒

**涉及电机、舵机、底盘、机械臂、夹爪等执行机构时，禁止在以下条件下直接上真实硬件**

1. 没有 stop/brake/fault 逻辑
2. 没有反馈超时判断
3. 没有速度/角度/电流/位置限幅
4. 没有低速测试
5. 没有确认急停链路
6. 没有记录测试 commit
7. app 层直接拼底层控制帧
8. 中断中执行复杂控制和阻塞等待

**默认原则**

```text
先安全，后功能
先低速，后高速
先单设备，后整机
先 mock/板级测试，后真实负载测试
```

---

## 9. 贡献方式与仓库治理

完整协作流程统一见 [CONTRIBUTING](.github/CONTRIBUTING.md)

可选启用本地 Hook

```bash
bash setup-scripts/setup-git.sh
```

```powershell
.\setup-scripts\setup-git.ps1
```

仓库管理员按 [Ruleset 说明](.github/rulesets/README.md) 导入 `.github/rulesets/main-protection.json`

规则文件随仓库交付不会自动启用 GitHub 远端保护，本地 Hook 也不能替代远端 Ruleset
