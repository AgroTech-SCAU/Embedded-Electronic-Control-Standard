#ifndef _servo_h_
#define _servo_h_

#include <stdbool.h>
#include <stdint.h>

// ! ========================= 接 口 变 量 / Typedef 声 明 ========================= ! //

/**
 * @brief 舵机统一入口单例
 *
 * service 层通过 servo_set_instance() 绑定具体舵机实例后，上层可使用
 * `servo.xxx` 或 `servo_xxx()` 两种形式调用统一接口
 */
#define servo (*servo_instance)

/**
 * @brief 舵机通用状态码表
 *
 * 使用 X-Macro 同时维护枚举值和默认字符串转换
 */
#define SERVO_STATUS_TABLE \
    X(OK, 0) \
    X(ERROR, 1) \
    X(INVALID_PARAM, 2) \
    X(PORT_ERROR, 3) \
    X(TIMEOUT, 4) \
    X(ID_MISMATCH, 5) \
    X(CHECKSUM_ERROR, 6) \
    X(NO_INSTANCE, 7) \
    X(NOT_INITIALIZE, 8) \
    X(BUFFER_TOO_SMALL, 9)

#define X(name, value) SERVO_STATUS_##name = value,
/**
 * @brief 舵机 SDK 通用状态码
 */
typedef enum {
    SERVO_STATUS_TABLE
} ServoStatus;
#undef X

/**
 * @brief 双字节寄存器的字节序
 */
typedef enum {
    /** 低字节在前，高字节在后 */
    SERVO_ENDIAN_LITTLE = 0,
    /** 高字节在前，低字节在后 */
    SERVO_ENDIAN_BIG = 1,
} ServoEndian;

/**
 * @brief 舵机反馈信息
 *
 * 除 temperature_c 和 moving 外，其余字段保留为协议原始计数；
 * 上层可按具体舵机型号手册换算为角度、速度、电压、电流等物理单位
 */
typedef struct {
    /** 反馈所属舵机 ID，单位：协议 ID */
    uint8_t id;
    /** 舵机应答帧中的原始错误码，单位：协议错误码 bitmask */
    uint8_t error_code;
    /** 当前原始位置值，单位：协议原始位置计数 */
    int16_t position_raw;
    /** 当前原始速度值，单位：协议原始速度计数 */
    int16_t speed_raw;
    /** 当前原始负载值，单位：协议原始负载计数 */
    int16_t load_raw;
    /** 当前原始电压值，单位：协议原始电压计数 */
    uint8_t voltage_raw;
    /** 当前温度，单位：摄氏度 */
    uint8_t temperature_c;
    /** 舵机是否仍在运动，单位：布尔值 */
    bool moving;
    /** 当前原始电流值，单位：协议原始电流计数 */
    int16_t current_raw;
} ServoFeedback;

/**
 * @brief 舵机底层端口函数表，由 service/platform 绑定
 *
 * device 层不直接依赖 HAL 或芯片外设；串口/总线收发与时基都通过本结构注入
 */
typedef struct {
    /**
     * @brief 写出一段总线数据
     * @param data 待发送数据缓冲区，单位：byte 数组
     * @param len 待发送数据长度，单位：byte
     * @return true 表示底层发送成功，false 表示发送失败
     */
    bool (*write)(const uint8_t* data, uint16_t len);
    /**
     * @brief 读取一段总线数据
     * @param data 接收数据缓冲区，单位：byte 数组
     * @param len 期望读取长度，单位：byte
     * @return 实际读到的字节数，单位：byte；超时或失败返回 0
     */
    int (*read)(uint8_t* data, uint16_t len);
    /**
     * @brief 获取当前毫秒时基
     * @return 当前系统毫秒计数，单位：ms
     */
    uint32_t(*now_ms)(void);
    /**
     * @brief 阻塞延时指定毫秒数
     * @param ms 延时时间，单位：ms
     */
    void (*delay_ms)(uint32_t ms);
} ServoPortOps;

/**
 * @brief 舵机实例初始化配置
 */
typedef struct {
    /** 底层端口函数表，必须有效 */
    const ServoPortOps* ops;
    /** 应答等待超时时间，单位：ms；传 0 时具体实例可使用默认值 */
    uint32_t timeout_ms;
    /** 预留重试次数，单位：次；具体实例可按需使用 */
    uint8_t retry_count;
    /** 双字节寄存器字节序，单位：ServoEndian 枚举值 */
    ServoEndian endian;
} ServoConfig;

/**
 * @brief 舵机统一接口表
 *
 * 不同型号舵机实现同一组接口，上层业务只依赖 ServoInterface/servo.h
 */
typedef struct {
    /**
     * @brief 初始化具体舵机实例
     * @param config 初始化配置
     * @return ServoStatus 状态码
     */
    ServoStatus(*init)(const ServoConfig* config);
    /**
     * @brief 状态码转字符串
     * @param status 状态码
     * @return const char* 状态码名称
     */
    const char* (*status_str)(ServoStatus status);
    /**
     * @brief 探测指定 ID 舵机是否在线
     * @param id 舵机 ID，单位：协议 ID
     * @param out_id 输出实际应答 ID，单位：协议 ID；可为 NULL
     * @return ServoStatus 状态码
     */
    ServoStatus(*ping)(uint8_t id, uint8_t* out_id);
    /**
     * @brief 打开扭矩输出
     * @param id 舵机 ID，单位：协议 ID
     * @return ServoStatus 状态码
     */
    ServoStatus(*enable_torque)(uint8_t id);
    /**
     * @brief 关闭扭矩输出
     * @param id 舵机 ID，单位：协议 ID
     * @return ServoStatus 状态码
     */
    ServoStatus(*disable_torque)(uint8_t id);
    /**
     * @brief 立即写入位置控制命令
     * @param id 舵机 ID，单位：协议 ID
     * @param position 目标位置，单位：协议原始位置计数
     * @param speed 目标速度，单位：协议原始速度计数
     * @param acc 加速度，单位：协议原始加速度计数
     * @return ServoStatus 状态码
     */
    ServoStatus(*set_position)(uint8_t id, int16_t position, uint16_t speed, uint8_t acc);
    /**
     * @brief 预写位置控制命令，等待 action 统一触发
     * @param id 舵机 ID，单位：协议 ID
     * @param position 目标位置，单位：协议原始位置计数
     * @param speed 目标速度，单位：协议原始速度计数
     * @param acc 加速度，单位：协议原始加速度计数
     * @return ServoStatus 状态码
     */
    ServoStatus(*reg_set_position)(uint8_t id, int16_t position, uint16_t speed, uint8_t acc);
    /**
     * @brief 触发已预写的 REG_WRITE 命令
     * @param id 舵机 ID 或广播 ID，单位：协议 ID
     * @return ServoStatus 状态码
     */
    ServoStatus(*action)(uint8_t id);
    /**
     * @brief 同步写多个舵机的位置控制命令
     * @param ids 舵机 ID 数组，单位：协议 ID 数组
     * @param count 舵机数量，单位：个
     * @param positions 目标位置数组，单位：协议原始位置计数
     * @param speeds 目标速度数组，单位：协议原始速度计数；可为 NULL
     * @param accs 加速度数组，单位：协议原始加速度计数；可为 NULL
     * @return ServoStatus 状态码
     */
    ServoStatus(*sync_set_position)(const uint8_t* ids, uint8_t count, const int16_t* positions, const uint16_t* speeds, const uint8_t* accs);
    /**
     * @brief 切换为轮式恒速模式
     * @param id 舵机 ID，单位：协议 ID
     * @return ServoStatus 状态码
     */
    ServoStatus(*set_wheel_mode)(uint8_t id);
    /**
     * @brief 轮式模式下设置转速
     * @param id 舵机 ID，单位：协议 ID
     * @param speed 目标速度，单位：协议原始速度计数
     * @param acc 加速度，单位：协议原始加速度计数
     * @return ServoStatus 状态码
     */
    ServoStatus(*set_speed)(uint8_t id, int16_t speed, uint8_t acc);
    /**
     * @brief 读取一组常用反馈
     * @param id 舵机 ID，单位：协议 ID
     * @param feedback 反馈输出指针，字段单位见 ServoFeedback
     * @return ServoStatus 状态码
     */
    ServoStatus(*read_feedback)(uint8_t id, ServoFeedback* feedback);
    /**
     * @brief 仅读取当前位置
     * @param id 舵机 ID，单位：协议 ID
     * @param position 当前位置输出指针，单位：协议原始位置计数
     * @return ServoStatus 状态码
     */
    ServoStatus(*read_position)(uint8_t id, int16_t* position);
    /**
     * @brief 写 1 字节寄存器
     * @param id 舵机 ID，单位：协议 ID
     * @param addr 寄存器地址，单位：协议地址偏移
     * @param value 写入值，单位：协议原始计数
     * @return ServoStatus 状态码
     */
    ServoStatus(*write_u8)(uint8_t id, uint8_t addr, uint8_t value);
    /**
     * @brief 写 2 字节寄存器
     * @param id 舵机 ID，单位：协议 ID
     * @param addr 寄存器地址，单位：协议地址偏移
     * @param value 写入值，单位：协议原始计数
     * @return ServoStatus 状态码
     */
    ServoStatus(*write_u16)(uint8_t id, uint8_t addr, uint16_t value);
    /**
     * @brief 读 1 字节寄存器
     * @param id 舵机 ID，单位：协议 ID
     * @param addr 寄存器地址，单位：协议地址偏移
     * @param value 读取值输出指针，单位：协议原始计数
     * @return ServoStatus 状态码
     */
    ServoStatus(*read_u8)(uint8_t id, uint8_t addr, uint8_t* value);
    /**
     * @brief 读 2 字节寄存器
     * @param id 舵机 ID，单位：协议 ID
     * @param addr 寄存器地址，单位：协议地址偏移
     * @param value 读取值输出指针，单位：协议原始计数
     * @return ServoStatus 状态码
     */
    ServoStatus(*read_u16)(uint8_t id, uint8_t addr, uint16_t* value);
} ServoInterface;

/**
 * @brief 当前绑定的具体舵机实例
 */
extern const ServoInterface* servo_instance;

// ! ========================= 接 口 函 数 声 明 ========================= ! //

/**
 * @brief 绑定具体舵机实例
 * @param instance 具体舵机实例接口表，例如 sts3215_servo_instance
 * @return ServoStatus 状态码
 */
ServoStatus servo_set_instance(const ServoInterface* instance);

/**
 * @brief 初始化当前绑定的具体舵机实例
 * @param config 初始化配置
 * @return ServoStatus 状态码
 */
ServoStatus servo_init(const ServoConfig* config);

/**
 * @brief 状态码转字符串
 * @param status 状态码
 * @return const char* 状态码名称
 */
const char* servo_status_str(ServoStatus status);

/**
 * @brief 探测指定 ID 舵机是否在线
 * @param id 舵机 ID，单位：协议 ID
 * @param out_id 输出实际应答 ID，单位：协议 ID；可为 NULL
 * @return ServoStatus 状态码
 */
ServoStatus servo_ping(uint8_t id, uint8_t* out_id);

/**
 * @brief 打开扭矩输出
 * @param id 舵机 ID，单位：协议 ID
 * @return ServoStatus 状态码
 */
ServoStatus servo_enable_torque(uint8_t id);

/**
 * @brief 关闭扭矩输出
 * @param id 舵机 ID，单位：协议 ID
 * @return ServoStatus 状态码
 */
ServoStatus servo_disable_torque(uint8_t id);

/**
 * @brief 立即写入位置控制命令
 * @param id 舵机 ID，单位：协议 ID
 * @param position 目标位置，单位：协议原始位置计数
 * @param speed 目标速度，单位：协议原始速度计数
 * @param acc 加速度，单位：协议原始加速度计数
 * @return ServoStatus 状态码
 */
ServoStatus servo_set_position(uint8_t id, int16_t position, uint16_t speed, uint8_t acc);

/**
 * @brief 预写位置控制命令，等待 action 统一触发
 * @param id 舵机 ID，单位：协议 ID
 * @param position 目标位置，单位：协议原始位置计数
 * @param speed 目标速度，单位：协议原始速度计数
 * @param acc 加速度，单位：协议原始加速度计数
 * @return ServoStatus 状态码
 */
ServoStatus servo_reg_set_position(uint8_t id, int16_t position, uint16_t speed, uint8_t acc);

/**
 * @brief 触发已预写的 REG_WRITE 命令
 * @param id 舵机 ID 或广播 ID，单位：协议 ID
 * @return ServoStatus 状态码
 */
ServoStatus servo_action(uint8_t id);

/**
 * @brief 同步写多个舵机的位置控制命令
 * @param ids 舵机 ID 数组，单位：协议 ID 数组
 * @param count 舵机数量，单位：个
 * @param positions 目标位置数组，单位：协议原始位置计数
 * @param speeds 目标速度数组，单位：协议原始速度计数；可为 NULL
 * @param accs 加速度数组，单位：协议原始加速度计数；可为 NULL
 * @return ServoStatus 状态码
 */
ServoStatus servo_sync_set_position(const uint8_t* ids, uint8_t count, const int16_t* positions, const uint16_t* speeds, const uint8_t* accs);

/**
 * @brief 切换为轮式恒速模式
 * @param id 舵机 ID，单位：协议 ID
 * @return ServoStatus 状态码
 */
ServoStatus servo_set_wheel_mode(uint8_t id);

/**
 * @brief 轮式模式下设置转速
 * @param id 舵机 ID，单位：协议 ID
 * @param speed 目标速度，单位：协议原始速度计数
 * @param acc 加速度，单位：协议原始加速度计数
 * @return ServoStatus 状态码
 */
ServoStatus servo_set_speed(uint8_t id, int16_t speed, uint8_t acc);

/**
 * @brief 读取一组常用反馈
 * @param id 舵机 ID，单位：协议 ID
 * @param feedback 反馈输出指针，字段单位见 ServoFeedback
 * @return ServoStatus 状态码
 */
ServoStatus servo_read_feedback(uint8_t id, ServoFeedback* feedback);

/**
 * @brief 仅读取当前位置
 * @param id 舵机 ID，单位：协议 ID
 * @param position 当前位置输出指针，单位：协议原始位置计数
 * @return ServoStatus 状态码
 */
ServoStatus servo_read_position(uint8_t id, int16_t* position);

/**
 * @brief 写 1 字节寄存器
 * @param id 舵机 ID，单位：协议 ID
 * @param addr 寄存器地址，单位：协议地址偏移
 * @param value 写入值，单位：协议原始计数
 * @return ServoStatus 状态码
 */
ServoStatus servo_write_u8(uint8_t id, uint8_t addr, uint8_t value);

/**
 * @brief 写 2 字节寄存器
 * @param id 舵机 ID，单位：协议 ID
 * @param addr 寄存器地址，单位：协议地址偏移
 * @param value 写入值，单位：协议原始计数
 * @return ServoStatus 状态码
 */
ServoStatus servo_write_u16(uint8_t id, uint8_t addr, uint16_t value);

/**
 * @brief 读 1 字节寄存器
 * @param id 舵机 ID，单位：协议 ID
 * @param addr 寄存器地址，单位：协议地址偏移
 * @param value 读取值输出指针，单位：协议原始计数
 * @return ServoStatus 状态码
 */
ServoStatus servo_read_u8(uint8_t id, uint8_t addr, uint8_t* value);

/**
 * @brief 读 2 字节寄存器
 * @param id 舵机 ID，单位：协议 ID
 * @param addr 寄存器地址，单位：协议地址偏移
 * @param value 读取值输出指针，单位：协议原始计数
 * @return ServoStatus 状态码
 */
ServoStatus servo_read_u16(uint8_t id, uint8_t addr, uint16_t* value);

#endif
