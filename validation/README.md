# validation

`validation/` 保存**用于证明正式 SDK 能在真实集成环境中工作**的验证资产

它与 `examples/` 的区别：

| 目录 | 主要目的 | 是否允许绑定真实硬件 |
|---|---|---|
| `examples/` | 教用户怎么写、怎么组织 | 尽量避免 |
| `validation/` | 验证正式 SDK 的集成链路 | 允许，且通常需要 |
| `tests/` | 自动化验证软件逻辑 | 默认不依赖真实执行机构 |
| `sdks/` | 正式可复用实现 | 本体 |

规则：

1. validation 可以包含 CubeMX `.ioc`、板级 `platform/`、测试业务和硬件说明
2. validation **优先直接引用根目录 `sdks/` 的正式代码**，不要维护同名 SDK 的 shadow copy
3. 硬件绑定、目标值、测试风险和通过标准必须写在该验证目录 README
4. validation 通过只证明对应硬件组合和测试条件下的集成结果，不自动代表所有平台都已验证
5. 涉及执行机构时必须遵守仓库根 README 的安全要求

当前验证资产：

```text
validation/
└── bus_motor_dm_stm32/
```
