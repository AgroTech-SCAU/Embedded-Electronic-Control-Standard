# portops_motor_device

最小 PortOps 设备设计示例

```text
platform/mock port
       ↓
PortOps
       ↓
motor_device
       ↓
chassis_service_example
```

文件作用

- `motor_device.*` 设备接口和实现
- `mock_motor_port.c` 模拟底层端口
- `chassis_service_example.c` service 组装示例

该示例只用于说明依赖方向
真实电机协议请使用 `sdks/device/bus_motor`
