# 模块状态

Stable 表示当前已验证配置内可固定引用，不保证任意板卡 设备组合或并发方式都适用

负责人确认步骤 1～3 已在真机验证通过，原始日志与完整硬件配置未随包提供，来源见 [硬件证据](../validation/hardware-evidence.md)

| 模块 | 状态 | 当前证据与范围 |
|---|---|---|
| bus_motor registry Group DM | Stable | 正式 SDK 主机协议回归通过，DM 真机结果由负责人确认 |
| DJI M2006/C610 | Stable | 四路组帧 反馈 Stop 超时主机回归通过，真机结果由负责人确认 |
| FS-iA10B | Stable | 有效帧 重同步 双实例 恢复主机回归通过，真机结果由负责人确认 |
| steer_wheel_kine | Stable | 固定坐标 轮序 IK FK 限速主机回归通过，步骤 3 真机使用由负责人确认 |
| IMU BMI088 | Stable | 源码与头文件编译通过，基础门面回归通过，步骤 3 真机结果由负责人确认 |
| RGB WS2812 | Stable | GRB 编码 异步 busy 主机回归通过，步骤 3 真机结果由负责人确认 |
| delay log | Stable | 时钟回绕与基础队列主机回归通过，步骤 3 真机结果由负责人确认 |
| DJI M3508/C620 | Candidate | 主机缩放回归通过，未收到该具体组合的真机确认 |
| PID matrix protocol_parser HFSM | Candidate | 保留正式实现，本轮没有专项完整功能回归 |
| serial_arm 系列运动学 | Experimental | 非 Longinus 当前依赖，本轮未验证模型与功能 |
| bus_servo | Experimental | 需专项协议与真机验收，本轮不宣称稳定 |
| 已退出正式 SDK 的旧项目入口与旧驱动 | Legacy | 仅在 Git 历史保留，不提供并行可选实现 |

## 状态含义

| 状态 | 使用规则 |
|---|---|
| Stable | 项目可按已验证配置固定引用，变更按发布规则审查 |
| Candidate | 有实现或局部证据，缺少完整适用范围的验收 |
| Experimental | 实验性或未经本轮确认，使用前自行专项验证 |
| Legacy | 已退出正式发布面，只用于历史参考 |

Longinus 固定负责人确认的 Tag 或 commit，任何新增平台 设备协议或机械配置都需要项目侧验证
