# FS-iA10B iBUS

FlySky FS-iA10B iBUS 接收驱动

特点

- 显式 `FsIa10b` 实例
- PortOps 平台注入
- 32 byte iBUS 帧解析
- 帧头重同步和校验
- UART 错误 abort / rearm
- 在线超时判断

## 接入

项目 platform 提供接收 时间和临界区能力

```c
FsIa10b receiver;
FsIa10bConfig config = {
    .ops = &ibus_ops,
};

fs_ia10b_init(&receiver, &config);
```

UART 完成和错误回调转发到实例

```c
fs_ia10b_on_rx_complete(&receiver);
fs_ia10b_on_rx_error(&receiver);
```

主循环持续维护

```c
fs_ia10b_maintain(&receiver);
```

读取数据和在线状态

```c
FsIa10bData data;
fs_ia10b_get_data(&receiver, &data);
fs_ia10b_is_online(&receiver, timeout_ms);
```

详细 PortOps 字段和状态码以 `fs_ia10b.h` 为准

## 边界

驱动不固定 UART 编号 不直接调用 HAL 不定义遥控通道业务含义

摇杆映射 开关语义 deadband 和失联后的底盘行为由项目 service 负责
