# domain SDK

domain 只保存与具体硬件无关的模型和算法

当前主要模块

- `steer_wheel_kine` 四舵轮运动学
- `serial_arm` 历史机械臂运动学模块

## 约束

- 不包含 CAN UART GPIO HAL 等硬件依赖
- 不包含机器人业务状态和比赛逻辑
- 参数使用明确物理单位
- 项目安装方向 零位和经验补偿留在 service 或 assemble

## 舵轮约定

```text
+x 前
+y 左
+z 上
+wz 逆时针为正
轮序 FL FR RR RL
```

`steer_wheel_kine` 只负责通用 IK FK 和模型约束
项目特有轮组方向和补偿不进入该模块
