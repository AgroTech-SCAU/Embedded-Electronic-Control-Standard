# bus_motor DM STM32 Validation

DM 新版 `bus_motor` 的最小 STM32 板级集成工程

该目录用于证明正式 SDK 能被真实 MCU 项目接入，不是教学 example

## 依赖

正式 SDK 通过 `sdk-sources.mk` 直接引用

主要包含

```text
sdks/device/bus_motor
sdks/device/rgb_led
sdks/infra/delay
sdks/infra/log
```

validation 内不应维护这些模块的复制副本

## 工程职责

- `Core` 和 `robot.ioc` 提供 STM32 基础工程
- `src/platform` 适配 FDCAN UART SPI DWT 等平台能力
- `src/service/assemble` 完成 PortOps 和真实电机绑定
- `src/service/motor_test.*` 负责最小验证状态机

## 验证顺序

1. 确认供电 CAN 接线 ID 波特率和急停手段
2. 上电后保持零输出
3. 验证初始化 使能 模式切换和反馈
4. 低风险条件验证命令输出
5. 验证 timeout stop disable 和错误恢复

真实执行结果由负责人在发布前确认
仓库不把未执行的硬件步骤写成 PASS
