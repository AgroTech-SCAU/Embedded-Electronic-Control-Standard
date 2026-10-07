# FS-iA10B iBUS

显式实例 `FsIa10b` 加 `FsIa10bConfig` 和带 context 的 PortOps 支持多接收机

这是对旧单例 API 的不兼容替换，移除 register_rx 和全局 rx_callback，项目按 UART 路由完成与错误事件到对应实例

```c
static FsIa10b receiver;
FsIa10bConfig config = { &ops, uart_context, 100u, 200u };
fs_ia10b_init(&receiver, &config);
/* RX callback */
fs_ia10b_on_rx_complete(&receiver);
/* Error callback */
fs_ia10b_on_rx_error(&receiver);
/* Main loop */
fs_ia10b_maintain(&receiver);
FsIa10bData data;
fs_ia10b_get_data(&receiver, &data);
if(fs_ia10b_is_online(&receiver, 100u)) { /* consume data */ }
```

PortOps 和 context 必须在实例存活期间有效，实例必须使用清零存储并在回调关闭时初始化，不允许接收活动期间重新初始化

start_receive 非阻塞启动单字节接收且不内联调用完成事件，false 表示启动失败，abort_receive 同步取消旧接收缓冲区并清理平台 RX 错误，所有端口回调必须可在 critical 区域运行，避免阻塞等待中断

critical_enter 返回先前状态，critical_exit 恢复该状态，必须同时覆盖主循环与 RX ISR，RTOS 多任务环境由项目提供等价串行化，同一实例禁止重入

完成事件进行固定 32 字节有界解析并重新挂接接收，错误事件标记恢复，由 maintain 执行 abort 和 rearm，maintain 必须定期调用，建议间隔不超过 10 ms

帧头 0x20 0x40，校验和为 0xFFFF 减前 30 字节，14 通道均须落在 800 到 2200，损坏或丢字节后使用滑动候选重同步，异常帧不覆盖上一有效样本

frame_count 为有效帧数，error_count 为无效候选帧 UART 错误和 rearm 失败次数，计数按 uint32_t 回绕

valid 仅表示曾收到有效数据，业务必须使用 is_online 判断新鲜度，超时边界采用严格小于，恢复待处理时离线，时间差使用无符号回绕计算

零配置选择帧超时 100 ms 和恢复间隔 200 ms，无有效帧的冷启动与断联会按恢复间隔重启，明确 UART 错误和启动失败可立即由 maintain 重试

