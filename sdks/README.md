# sdks/ 通用 SDK 说明

> `sdks/` 保存可以被成员项目复用的通用模块；项目通过 submodule 引入本仓库后，按需把对应模块加入自己的工程

---

## 1. 目录结构

```text
sdks/
├── infra/     # 基础设施 SDK：PID、矩阵、协议解析、HFSM、delay、status 等
├── domain/    # 领域算法 SDK：运动学、机构模型、坐标变换等
└── device/    # 设备 SDK：电机、舵机、编码器、IMU、通信设备等
```

---

## 2. 分层边界

| 目录 | 关注点 | 允许依赖 | 不应依赖 |
|---|---|---|---|
| `infra/` | 通用工具、状态机、解析器、控制器、统一状态码 | 标准 C、由上层注入的外部能力接口 | 任意项目层、HAL/FSP/CubeMX |
| `domain/` | 数学模型、运动学、坐标系、限幅 | `infra/`、标准 C 数学库 | 真实设备、CAN/UART/GPIO、HAL/FSP/CubeMX |
| `device/` | 真实设备命令、反馈、超时、安全停止 | `infra/`、PortOps | app 业务、domain、平台句柄、芯片头文件 |

具体芯片适配不放在本仓库内，应由独立 chip SDK 或成员项目的 `src/platform/` 提供

---

## 3. 推荐接入流程

1. 在成员项目中通过 submodule 引入本仓库
2. 从 `sdks/` 中选择需要的模块
3. 将模块加入项目构建系统
4. 在 service init 中组装 PortOps 或 adapter，并把 platform 能力注入 device
5. 调用 SDK 的 init 接口完成绑定
6. 先做 PC/mock 或单设备低速测试
7. 再接入 app 任务流

---

## 4. 编写新 SDK 的基本要求

- public header 不包含芯片平台头文件
- public API 明确错误码、配置结构、生命周期和单位
- 二值语义必须使用 `bool` / `true` / `false`，并在使用处包含 `<stdbool.h>`；不要用 `uint8_t`、`int` 或 `0/1` 表示布尔状态
- 涉及真实设备时必须提供 stop、timeout、fault 或等价安全路径
- device 需要底层能力时优先通过 `init(config with PortOps)` 接入
- infra 保持层独立；若需要时间、输出流等外部能力，应通过配置/PortOps 接收由 `service` 注入的能力
- 可独立测试的模块应提供最小 example 或测试入口
- 修改 public API 时同步更新 README、示例和 `plan.md`

---

## 5. 示例参考

- `sdks/device/bus_motor/`：电机统一接口、入口单例、PortOps 和达妙电机实例
- `sdks/infra/log.h` / `sdks/infra/log.c`：可替换输出端口的轻量日志接口
- `sdks/infra/status.h`：统一状态码、生命周期状态和 X-Macro 字符串辅助
- `sdks/device/rgb_led/`：RGB 灯统一接口，当前包含 WS2812/NeoPixel 实例
- `examples/module_design/portops_motor_device/`：从需求、接口、mock port 到 service 绑定的贯穿式示例
