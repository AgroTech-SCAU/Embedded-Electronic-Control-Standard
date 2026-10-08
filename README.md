# Embedded-Electronic-Control-Standard

AgroTech 协会公共 MCU SDK

Standard 只维护可跨项目复用的 `infra / domain / device`
项目自行维护 `app / service / service/assemble / platform` 与机械安装配置

## 快速入口

| 入口 | 用途 |
|---|---|
| [设备 SDK](sdks/device/README.md) | 电机 遥控 IMU RGB 舵机 |
| [SDK 接入](sdks/README.md) | 项目如何引用和组装 |
| [架构边界](docs/architecture.md) | 分层 PortOps 单位与项目边界 |
| [模块状态](docs/module-status.md) | 判断模块成熟度 |
| [当前计划](docs/plan.md) | 当前剩余工作 |
| [协作指南](.github/CONTRIBUTING.md) | Issue Branch PR 与合并流程 |

## 引用方式

成员项目通过 submodule 固定负责人确认过的 Tag 或 commit

```bash
git submodule add https://github.com/AgroTech-SCAU/Embedded-Electronic-Control-Standard.git external/Embedded-Electronic-Control-Standard
cd external/Embedded-Electronic-Control-Standard
git fetch --tags
git switch --detach <tag-or-commit>
```

克隆成员项目后执行

```bash
git submodule update --init --recursive
```

## 使用规则

- 不复制 SDK 到成员项目维护第二份实现
- platform 提供 CAN UART SPI GPIO 时基和临界区等底层能力
- service/assemble 负责 PortOps 配置 实例存储和真实硬件绑定
- device 不直接依赖 HAL FSP CubeMX 项目头或具体外设句柄
- position 使用 rad velocity 使用 rad/s 长度使用 m torque 只表示 N·m
- CAN ID 电机方向 零位 机械补偿和比赛业务留在项目侧
- 涉及执行机构必须有 stop timeout fault 和人工恢复路径

## 验证与发布

`validation/` 只保存最小集成工程和复现入口，不复制正式 SDK

模块成熟度以 [module-status](docs/module-status.md) 为准

正式 Release 由项目负责人在代码 文档和必要真机结果确认后创建
