# 架构边界

## 项目分层

```text
app
 ↓
service
 ├─ assemble
 ↓
device / domain / infra
 ↑
platform 通过 PortOps 注入底层能力
```

| 层 | 职责 |
|---|---|
| app | 任务流程 状态和用户输入 |
| service | 组合设备 算法与安全策略 |
| service/assemble | 初始化 PortOps 实例和真实硬件绑定 |
| device | 设备协议 命令 反馈与恢复 |
| domain | 运动学 几何与纯算法 |
| infra | PID 日志 状态机 解析器等基础设施 |
| platform | HAL RTOS CAN UART SPI GPIO 时基等板级能力 |

Standard 正式 SDK 只包含 `infra / domain / device`

## 依赖规则

- app 只通过 service 使用系统能力
- device 和需要外部能力的 infra 通过 PortOps 或等价注入使用 platform
- domain 不接触真实设备和平台句柄
- public header 不直接包含项目 HAL FSP CubeMX 头文件
- 项目参数和厂家绑定放在 service/assemble

## 数据约定

- position 使用 rad
- velocity 使用 rad/s
- length 使用 m
- torque 只表示 N·m
- 原始电流和换算电流使用独立字段
- 舵轮坐标固定为 `+x 前 +y 左 +z 上 +wz 逆时针`
- 舵轮顺序固定为 `FL FR RR RL`

## 边界

Standard 不维护完整机器人业务和 Chip SDK

CAN 分配 电机方向 舵角偏置 机械补偿 比赛逻辑和平台初始化都属于成员项目

厂家特有能力保留在对应厂家扩展接口，不为了单一设备扩大所有公共 API
