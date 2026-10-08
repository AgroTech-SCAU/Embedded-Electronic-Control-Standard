# bus_servo

统一串行总线舵机门面

当前包含 FEETECH SCS 和中菱相关实现
该模块当前为 Experimental，项目使用前需要专项验证

## 基本接入

选择具体实例并传入厂家配置

```c
bus_servo_set_instance(&ft_scs_servo_common_instance);
bus_servo_init(&config);
```

公共接口包括

```c
bus_servo_set_speed(...);
bus_servo_set_pos_spd(...);
bus_servo_set_pos_spd_tor(...);
bus_servo_update_feedback(...);
```

反馈位置 速度 力矩通过统一门面读取

厂家特有能力继续使用对应厂家头文件

## PortOps

平台能力通过 `BusServoPortOps` 注入

UART 编号 半双工方向 GPIO 和具体 HAL 句柄不得进入业务层

## 边界

- 通用门面只保留多厂家共有能力
- 协议寄存器 扫描 EEPROM 等厂家能力留在厂家扩展接口
- 真实机械臂的关节限位 零位和安全策略由项目 service 负责
