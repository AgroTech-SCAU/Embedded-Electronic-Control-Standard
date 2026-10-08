# IMU

统一 IMU 门面和 BMI088 实现

## 文件

```text
imu.*            统一接口
bmi088.*         BMI088 驱动
imu_attitude.*   可选姿态融合
```

## 基本使用

选择实例后初始化

```c
Bmi088Config config;
bmi088_make_config(&config, &ops, acc_int_pin, gyro_int_pin);

imu_set_instance(&bmi088_instance);
imu_init(&config);
```

周期更新并读取

```c
imu_update();
ImuSample sample;
imu_get_sample(&sample);
```

也可直接读取 acc gyro angle 和校正后的 gyro

完整接口以 `imu.h` `bmi088.h` `imu_attitude.h` 为准

## BMI088

BMI088 通过 `Bmi088PortOps` 注入 SPI CS 时间和可选异步完成能力

提供普通实例和 blocking 实例

```c
bmi088_instance
bmi088_blocking_instance
```

EXTI SPI 完成和 SPI 错误由项目 platform 转发到对应回调

## 边界

- device 不固定 SPI 句柄和 GPIO
- 姿态融合可关闭
- 传感器安装方向和整机坐标变换由项目配置
- 真机使用前确认量程 中断 采样率和轴方向
