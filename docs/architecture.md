# 架构与 SDK 边界

Standard 维护可跨项目复用的公共 MCU SDK，成员项目维护硬件装配 机械安装参数与任务业务

## 六层项目架构

| 层 | 职责 | 依赖 |
|---|---|---|
| app | 任务流程 调度 用户输入 | service |
| service | 组合设备与算法 完成业务能力 | device domain infra |
| service/assemble | 初始化顺序 PortOps 实例存储和配置 | platform device domain infra |
| device | 设备协议 命令 反馈和恢复 | infra 标准库 注入端口 |
| domain | 运动学 几何 坐标与通用数学 | infra 标准库 |
| infra | 时间 日志 控制器 解析和基础设施 | 标准库 注入能力 |
| platform | HAL RTOS CAN UART SPI GPIO 时基 | 板级环境 |

assemble 是 service 的装配子目录，不是新增公共 SDK 层

正式 SDK 仅包含 infra domain device，examples 与 validation 可包含最小 app service platform，不属于公共 SDK

## 依赖注入

平台句柄与芯片头文件留在 platform，assemble 用适配函数绑定到 SDK PortOps，device 不反向包含项目 platform 或 service

不要求所有模块使用同一配置类型，配置可由具体厂家定义，成熟接口的等价函数注入可以保留

实例缓存和 PortOps 必须覆盖驱动生命周期，配置是否被复制以模块契约为准，异步 write 返回后仍持有的缓存必须保持有效直到完成通知

主循环 控制函数与 ISR 按模块约定串行化，FS-iA10B 使用注入临界区，电机 RX ISR 先排队，由主循环解析与控制，不假定所有 SDK 都具备多任务并发能力

## 数据与项目边界

- position 使用 rad，velocity 使用 rad/s，长度使用 m
- torque 仅表示 N·m，原始电流和可换算电流使用独立字段与有效标记
- domain 舵轮采用 +x 前 +y 左 +z 上 +wz 逆时针，轮序 FL FR RR RL
- CAN 分配 逻辑角色 电机方向 舵角偏置和机械补偿留在项目 assemble 或 service
- 厂家专用控制模式留在厂家扩展接口，不扩大所有厂家公共 API

## 初始化与运行

先完成平台外设与时基初始化，再装配 infra device domain，最后启动 service 与 app

每个步骤检查返回值，初始化失败不进入正常运动循环，反馈不足 超时或发送错误由 service 锁存并进入停止路径，反馈恢复后人工确认再使能

SDK stop 和 disable 的真实含义以厂家模块说明为准，系统级急停和供电切断由项目实现

## 复用与沉淀

公共算法不接触设备，公共设备不包含机器人业务，至少形成实际复用需求后再提炼公共能力

平台适配由各项目维护，当前不建立公共 Chip SDK，不能把 HAL 工程或完整机器人业务迁入 sdks

示例用于理解接口，validation 用于证明集成结果，验证源文件必须直接编译正式 SDK，禁止保留复制副本

模块状态以 [module-status](module-status.md) 为准，贡献流程以 [CONTRIBUTING](../.github/CONTRIBUTING.md) 为准
