# 公共 SDK 接入

`sdks/` 只保存跨项目复用模块

```text
sdks/
├── infra
├── domain
└── device
```

设备目录入口见 [device/README.md](device/README.md)
模块成熟度见 [module-status](../docs/module-status.md)
架构边界见 [architecture](../docs/architecture.md)

## 接入步骤

1. 成员项目以 submodule 固定 Standard Tag 或 commit
2. 只把实际需要的 SDK 源文件加入构建
3. platform 实现模块需要的底层能力
4. service/assemble 创建 PortOps 配置与实例存储
5. 检查初始化返回值后再由 service 接管运行和安全策略

## 规则

- 不复制 SDK 到项目维护第二份实现
- public header 不包含项目 HAL FSP CubeMX 头文件
- 项目 CAN ID 电机角色 方向 零位和机械补偿不进入 Standard
- 厂家专有功能使用厂家扩展接口
- 详细 API 以对应 public header 为准
