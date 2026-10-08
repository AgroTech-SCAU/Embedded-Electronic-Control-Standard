# validation

`validation/` 保存最小板级集成工程和复现步骤

规则

- 直接编译正式 `sdks/`
- 不复制 SDK 源码维护第二份实现
- 只保留必要 `Core` `.ioc` `platform` `assemble` 和验证业务
- 只记录实际执行过的硬件结果

当前集成工程

- `bus_motor_dm_stm32` DM + STM32 验证工程

新增 validation 时沿用同样边界
