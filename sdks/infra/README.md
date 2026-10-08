# infra SDK

infra 保存与具体硬件和机器人业务无关的基础设施

当前包括

- delay
- log
- PID
- matrix
- protocol_parser
- HFSM

## 约束

- 不直接依赖项目层和平台全局句柄
- 需要时间 输出 锁等外部能力时通过配置或 PortOps 注入
- public header 不包含 HAL FSP CubeMX 项目头
- 多实例模块优先使用显式实例或上下文

详细 API 以各模块头文件为准
