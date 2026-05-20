# infra SDK 基本说明

> `sdks/infra/` 提供与业务和具体芯片无关的基础设施模块

---

## 1. 模块定位

infra 模块用于沉淀通用能力，例如控制器、数学工具、协议解析器、状态机和时间工具；它们应尽量能在 PC/mock 环境下单独编译和测试

---

## 2. 当前模块

| 模块 | 文件 | 说明 |
|---|---|---|
| delay | `delay.h` / `delay.c` | 通用延时或时间辅助接口 |
| matrix | `matrix.h` / `matrix.c` | 矩阵运算工具 |
| PID | `pid.h` / `pid.c` | 多实例 PID 控制器 |
| protocol parser | `protocol_parser.h` / `protocol_parser.c` | 通信协议解析辅助 |
| HFSM | `hfsm/` | 层级状态机基础框架 |
| log | `log.h` / `log.c` | 可替换输出端口的轻量日志接口 |

---

## 3. 设计约束

- public header 不包含 HAL/FSP/CubeMX/CMSIS 头文件
- 不保存平台全局句柄，例如 `hfdcan1`、`huart1`
- 多实例工具优先使用 `Handle* self` 或上下文结构
- 二值配置、状态和查询接口统一使用 `bool` / `true` / `false`，例如 `ring_buf_create(..., bool overwrite)`、`is_full()` 和 `is_empty()`
- 不依赖 `platform/`、`device/`、`domain/`、`service/`、`app/`
- 如果需要时间、锁、输出流等外部能力，应通过配置/PortOps 接收上层注入的能力，而不是直接依赖 `platform/`
- 错误码不直接泄漏平台错误类型
- 能 PC/mock 测试的模块应保留最小测试入口

---

## 4. 推荐接口形态

```c
typedef struct {
    uint32_t (*now_ms)(void);
} InfraPortOps;

typedef struct {
    const InfraPortOps* ops;
    uint32_t timeout_ms;
} InfraConfig;

InfraStatus infra_init(Infra* self, const InfraConfig* config);
InfraStatus infra_update(Infra* self);
```

纯算法工具或基础设施工具应优先保持简单函数或多实例对象；只有确实需要外部能力时，再通过注入接口接入

---

## 5. 后续补充方向

- CRC
- limiter
- low pass filter
- moving average
- error code table helper
- unit test scaffold
