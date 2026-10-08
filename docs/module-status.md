# 模块状态

状态用于说明当前维护优先级和适用范围，不代表任意板卡和任意设备组合都已验证

| 模块 | 状态 | 说明 |
|---|---|---|
| bus_motor / DM | Stable | 新版统一入口和 DM 驱动 |
| bus_motor / DJI M2006 C610 | Stable | 支持四路组帧 反馈 Stop 和超时 |
| FS-iA10B | Stable | 显式实例 PortOps 帧重同步和错误恢复 |
| steer_wheel_kine | Stable | 固定右手坐标和轮序 |
| IMU BMI088 | Stable | IMU 门面和 BMI088 实现 |
| RGB WS2812 | Stable | RGB 门面和 WS2812 实现 |
| delay / log | Stable | 基础时基与日志能力 |
| DJI M3508 C620 | Candidate | 有实现但当前不是 Longinus 主路径 |
| PID matrix protocol_parser HFSM | Candidate | 保留正式实现 按项目需要使用 |
| serial_arm | Experimental | 当前不作为机械臂主线方案 |
| bus_servo | Experimental | 使用前需要项目专项验证 |

## 状态含义

| 状态 | 含义 |
|---|---|
| Stable | 当前已确认使用路径 可由项目固定引用 |
| Candidate | 已有实现 但验证范围较窄 |
| Experimental | 实验性或当前未专项验收 |

真机结论只记录实际完成过的结果
新增平台 型号或机械配置仍需项目侧验证
