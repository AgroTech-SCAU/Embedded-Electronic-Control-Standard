#ifndef _servo_h_
#define _servo_h_

#include <stdbool.h>
#include <stdint.h>

// ! ========================= 接 口 类 型 ========================= ! //

/**
 * @brief 舵机入口单例, 上层可统一调用 servo.xxx 或 servo_xxx
 */
#define servo (*servo_instance)

/**
 * @brief 舵机通用状态码表
 * @param OK 操作成功
 * @param ERROR 通用错误
 * @param INVALID_PARAM 参数无效
 * @param PORT_ERROR 端口函数表未提供或底层端口失败
 * @param TIMEOUT 等待舵机应答超时
 * @param ID_MISMATCH 应答 ID 与期望 ID 不一致
 * @param CHECKSUM_ERROR 应答帧校验错误
 * @param NO_INSTANCE 未绑定具体舵机实例
 * @param NOT_INITIALIZE 具体舵机实例尚未初始化
 * @param BUFFER_TOO_SMALL 缓冲区容量不足
 * @param UNSUPPORTED 当前舵机不支持该通用能力
 * @param NO_FEEDBACK 指定 ID 尚无有效反馈缓存
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
    X(BUFFER_TOO_SMALL, 9) \
    X(UNSUPPORTED, 10) \
    X(NO_FEEDBACK, 11)

#define X(name, value) SERVO_STATUS_##name = value,
/**
 * @brief 舵机通用状态码
 */
typedef enum {
    SERVO_STATUS_TABLE
} ServoStatus;
#undef X

/**
 * @brief 具体舵机总线中 16 位寄存器的字节序
 */
typedef enum {
    /** 低字节在前, 高字节在后 */
    SERVO_ENDIAN_LITTLE = 0,
    /** 高字节在前, 低字节在后 */
    SERVO_ENDIAN_BIG = 1,
} ServoEndian;

/**
 * @brief 最近一次解析出的通用舵机反馈
 * @param id 舵机协议 ID
 * @param error_code 最近一次应答帧中的原始错误码
 * @param position 当前位置, 单位 rad
 * @param speed 当前速度, 单位 rad/s
 * @param torque 当前扭矩或负载估计, 单位 N*m
 */
typedef struct {
    uint8_t id;
    uint8_t error_code;
    float position;
    float speed;
    float torque;
} ServoFeedback;

/**
 * @brief 舵机底层端口函数表, 由 service 或 platform 注入
 */
typedef struct {
    /**
     * @brief 向舵机总线写出一段字节流
     * @param data 待发送缓冲区
     * @param len 待发送长度, 单位 byte
     * @return 为真表示底层写入成功, 为假表示底层写入失败
     */
    bool (*write)(const uint8_t* data, uint16_t len);
    /**
     * @brief 从舵机总线读取字节
     * @param data 接收缓冲区
     * @param len 期望读取长度, 单位 byte
     * @return 实际读取到的字节数, 0 表示暂无数据或读取失败
     */
    int (*read)(uint8_t* data, uint16_t len);
    /**
     * @brief 获取当前单调递增时间
     * @return 当前时间, 单位 ms
     */
    uint32_t(*now_ms)(void);
    /**
     * @brief 可选的阻塞延时函数
     * @param ms 延时时间, 单位 ms
     */
    void (*delay_ms)(uint32_t ms);
    /**
     * @brief 可选的接收缓冲区清空函数, 发送新指令前调用
     */
    void (*flush_rx)(void);
} ServoPortOps;

/**
 * @brief 舵机通用初始化配置
 * @param ops 底层端口函数表
 * @param timeout_ms 应答超时时间, 单位 ms, 传 0 时由驱动选择默认值
 * @param retry_count 预留重试次数, 由具体驱动按需使用
 * @param endian 具体舵机型号使用的 16 位寄存器字节序
 */
typedef struct {
    const ServoPortOps* ops;
    uint32_t timeout_ms;
    uint8_t retry_count;
    ServoEndian endian;
} ServoConfig;

/**
 * @brief 舵机通用接口表
 *
 * 这里仅放所有舵机都应具备的通用能力
 * 型号或协议专属能力应由驱动提供独立特色入口单例
 */
typedef struct {
    /**
     * @brief 初始化具体舵机驱动
     * @param config 初始化配置
     * @return 状态码
     */
    ServoStatus(*init)(const ServoConfig* config);
    /**
     * @brief 将状态码转换为常量字符串
     * @param status 状态码
     * @return 状态码名称字符串
     */
    const char* (*status_str)(ServoStatus status);
    /**
     * @brief 让舵机一直以指定速度旋转
     * @param id 舵机 ID
     * @param speed 目标速度, 单位 rad/s
     * @return 状态码
     */
    ServoStatus(*set_speed)(uint8_t id, float speed);
    /**
     * @brief 让舵机以指定速度旋转到指定位置
     * @param id 舵机 ID
     * @param position 目标位置, 单位 rad
     * @param velocity 目标速度, 单位 rad/s
     * @return 状态码
     */
    ServoStatus(*set_pos_spd)(uint8_t id, float position, float velocity);
    /**
     * @brief 让舵机以指定速度和扭矩旋转到指定位置并保持扭矩
     * @param id 舵机 ID
     * @param position 目标位置, 单位 rad
     * @param velocity 目标速度, 单位 rad/s
     * @param torque 保持扭矩或负载限制, 单位 N*m
     * @return 状态码
     */
    ServoStatus(*set_pos_spd_tor)(uint8_t id, float position, float velocity, float torque);
    /**
     * @brief 从本地反馈缓存获取最近解析的位置
     * @param id 舵机 ID
     * @return 最近解析的位置, 单位 rad
     */
    float (*get_position)(uint8_t id);
    /**
     * @brief 从本地反馈缓存获取最近解析的速度
     * @param id 舵机 ID
     * @return 最近解析的速度, 单位 rad/s
     */
    float (*get_speed)(uint8_t id);
    /**
     * @brief 从本地反馈缓存获取最近解析的扭矩
     * @param id 舵机 ID
     * @return 最近解析的扭矩或负载估计, 单位 N*m
     */
    float (*get_torque)(uint8_t id);
    /**
     * @brief 主动请求并解析反馈, 然后更新本地反馈缓存
     * @param id 舵机 ID
     * @param feedback 可选的反馈输出指针, 可为 NULL
     * @return 状态码
     */
    ServoStatus(*update_feedback)(uint8_t id, ServoFeedback* feedback);
} ServoInterface;

/**
 * @brief 当前绑定的具体舵机实例
 */
extern const ServoInterface* servo_instance;

// ! ========================= 接 口 函 数 ========================= ! //

/**
 * @brief 绑定具体舵机实例
 * @param instance 具体舵机接口表, 例如 ft_scs_servo_common_instance
 * @return 状态码
 */
ServoStatus servo_set_instance(const ServoInterface* instance);

/**
 * @brief 初始化当前绑定的舵机实例
 * @param config 初始化配置
 * @return 状态码
 */
ServoStatus servo_init(const ServoConfig* config);

/**
 * @brief 将状态码转换为常量字符串
 * @param status 状态码
 * @return 状态码名称字符串
 */
const char* servo_status_str(ServoStatus status);

/**
 * @brief 让舵机一直以指定速度旋转
 * @param id 舵机 ID
 * @param speed 目标速度, 单位 rad/s
 * @return 状态码
 */
ServoStatus servo_set_speed(uint8_t id, float speed);

/**
 * @brief 让舵机以指定速度旋转到指定位置
 * @param id 舵机 ID
 * @param position 目标位置, 单位 rad
 * @param velocity 目标速度, 单位 rad/s
 * @return 状态码
 */
ServoStatus servo_set_pos_spd(uint8_t id, float position, float velocity);

/**
 * @brief 让舵机以指定速度和扭矩旋转到指定位置并保持扭矩
 * @param id 舵机 ID
 * @param position 目标位置, 单位 rad
 * @param velocity 目标速度, 单位 rad/s
 * @param torque 保持扭矩或负载限制, 单位 N*m
 * @return 状态码
 */
ServoStatus servo_set_pos_spd_tor(uint8_t id, float position, float velocity, float torque);

/**
 * @brief 从本地反馈缓存获取最近解析的位置
 * @param id 舵机 ID
 * @return 最近解析的位置, 单位 rad
 */
float servo_get_position(uint8_t id);

/**
 * @brief 从本地反馈缓存获取最近解析的速度
 * @param id 舵机 ID
 * @return 最近解析的速度, 单位 rad/s
 */
float servo_get_speed(uint8_t id);

/**
 * @brief 从本地反馈缓存获取最近解析的扭矩
 * @param id 舵机 ID
 * @return 最近解析的扭矩或负载估计, 单位 N*m
 */
float servo_get_torque(uint8_t id);

/**
 * @brief 主动请求并解析反馈, 然后更新本地反馈缓存
 * @param id 舵机 ID
 * @param feedback 可选的反馈输出指针, 可为 NULL
 * @return 状态码
 */
ServoStatus servo_update_feedback(uint8_t id, ServoFeedback* feedback);

#endif
