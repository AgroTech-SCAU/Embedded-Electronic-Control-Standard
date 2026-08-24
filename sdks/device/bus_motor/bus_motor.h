#ifndef _bus_motor_h_
#define _bus_motor_h_

#include <stdbool.h>
#include <stdint.h>

// ! ========================= 接 口 变 量 / Typedef 声 明 ========================= ! //

/**
 * @brief 总线电机最大逻辑实例数量
 */
#define BUS_MOTOR_MAX_COUNT 32u

/**
 * @brief 总线电机最大逻辑组数量
 */
#define BUS_MOTOR_GROUP_MAX_COUNT 8u

/**
 * @brief 单个逻辑组最大电机数量
 */
#define BUS_MOTOR_GROUP_MEMBER_MAX 8u

/**
 * @brief 电机通用状态码表
 */
#define MOTOR_STATUS_TABLE \
    X(OK, 0)               \
    X(ERROR, 1)            \
    X(INVALID_PARAM, 2)    \
    X(PORT_ERROR, 3)       \
    X(TIMEOUT, 4)          \
    X(ID_MISMATCH, 5)      \
    X(NO_INSTANCE, 6)      \
    X(NOT_INITIALIZE, 7)   \
    X(UNSUPPORTED, 8)      \
    X(NO_FEEDBACK, 9)      \
    X(NOT_FOUND, 10)       \
    X(ALREADY_BOUND, 11)   \
    X(NO_RESOURCE, 12)     \
    X(PROFILE_MISMATCH, 13)

#define X(name, value) MOTOR_STATUS_##name = value,
/**
 * @brief 电机通用状态码
 */
typedef enum {
    MOTOR_STATUS_TABLE
} BusMotorStatus;
#undef X

/**
 * @brief 业务逻辑电机 ID
 *
 * bus_motor 只规定 ID 类型，具体枚举由业务层自行定义
 */
typedef uint16_t BusMotorId;

/**
 * @brief 业务逻辑电机组 ID
 *
 * bus_motor 只规定 ID 类型，具体枚举由业务层自行定义
 */
typedef uint16_t BusMotorGroupId;

/**
 * @brief 电机控制 Profile
 */
typedef enum {
    BUS_MOTOR_PROFILE_NONE = 0u,
    BUS_MOTOR_PROFILE_POSITION = 1u << 0,
    BUS_MOTOR_PROFILE_VELOCITY = 1u << 1,
    BUS_MOTOR_PROFILE_TORQUE = 1u << 2,
    BUS_MOTOR_PROFILE_IMPEDANCE = 1u << 3,
    BUS_MOTOR_PROFILE_CURRENT_Q = 1u << 4,
    BUS_MOTOR_PROFILE_VOLTAGE_Q = 1u << 5,
    BUS_MOTOR_PROFILE_CURRENT_DQ = 1u << 6,
    BUS_MOTOR_PROFILE_VOLTAGE_DQ = 1u << 7,
    BUS_MOTOR_PROFILE_ACCELERATION = 1u << 8,
} BusMotorProfile;

/**
 * @brief 电机控制 Profile 位图
 */
typedef uint32_t BusMotorProfileMask;

/**
 * @brief 阻抗控制指令
 */
typedef struct {
    float position; /**< 目标位置，单位 rad */
    float velocity; /**< 目标速度，单位 rad/s */
    float kp;       /**< 位置比例系数 */
    float kd;       /**< 位置微分系数 */
    float torque;   /**< 前馈扭矩，单位 N*m */
} BusMotorImpedanceCommand;

/**
 * @brief DQ 控制指令
 */
typedef struct {
    float d; /**< D 轴目标值 */
    float q; /**< Q 轴目标值 */
} BusMotorDqCommand;

/**
 * @brief 电机扩展控制命令类型
 */
typedef enum {
    BUS_MOTOR_CMD_POSITION = 0u,
    BUS_MOTOR_CMD_VELOCITY,
    BUS_MOTOR_CMD_TORQUE,
    BUS_MOTOR_CMD_IMPEDANCE,
    BUS_MOTOR_CMD_CURRENT_Q,
    BUS_MOTOR_CMD_VOLTAGE_Q,
    BUS_MOTOR_CMD_CURRENT_DQ,
    BUS_MOTOR_CMD_VOLTAGE_DQ,
    BUS_MOTOR_CMD_ACCELERATION,
} BusMotorCommandType;

/**
 * @brief 电机统一控制命令
 */
typedef struct {
    BusMotorCommandType type; /**< 命令类型 */
    union {
        float scalar;                 /**< 单标量目标值 */
        BusMotorDqCommand dq;         /**< DQ 目标值 */
        BusMotorImpedanceCommand imp; /**< 阻抗控制目标值 */
    } data;
} BusMotorCommand;

/**
 * @brief 构造位置控制命令
 */
#define BUS_CMD_POSITION(value) ((BusMotorCommand){ BUS_MOTOR_CMD_POSITION, { .scalar = (value) } })

/**
 * @brief 构造速度控制命令
 */
#define BUS_CMD_VELOCITY(value) ((BusMotorCommand){ BUS_MOTOR_CMD_VELOCITY, { .scalar = (value) } })

/**
 * @brief 构造扭矩控制命令
 */
#define BUS_CMD_TORQUE(value) ((BusMotorCommand){ BUS_MOTOR_CMD_TORQUE, { .scalar = (value) } })

/**
 * @brief 构造阻抗控制命令
 */
#define BUS_CMD_IMPEDANCE(position, velocity, kp, kd, torque) \
    ((BusMotorCommand){ BUS_MOTOR_CMD_IMPEDANCE, { .imp = { (position), (velocity), (kp), (kd), (torque) } } })

/**
 * @brief 构造 Q 轴电流控制命令
 */
#define BUS_CMD_CURRENT_Q(value) ((BusMotorCommand){ BUS_MOTOR_CMD_CURRENT_Q, { .scalar = (value) } })

/**
 * @brief 构造 Q 轴电压控制命令
 */
#define BUS_CMD_VOLTAGE_Q(value) ((BusMotorCommand){ BUS_MOTOR_CMD_VOLTAGE_Q, { .scalar = (value) } })

/**
 * @brief 构造 DQ 电流控制命令
 */
#define BUS_CMD_CURRENT_DQ(d, q) ((BusMotorCommand){ BUS_MOTOR_CMD_CURRENT_DQ, { .dq = { (d), (q) } } })

/**
 * @brief 构造 DQ 电压控制命令
 */
#define BUS_CMD_VOLTAGE_DQ(d, q) ((BusMotorCommand){ BUS_MOTOR_CMD_VOLTAGE_DQ, { .dq = { (d), (q) } } })

/**
 * @brief 构造加速度控制命令
 */
#define BUS_CMD_ACCELERATION(value) ((BusMotorCommand){ BUS_MOTOR_CMD_ACCELERATION, { .scalar = (value) } })

/**
 * @brief 电机反馈有效字段
 */
typedef enum {
    BUS_MOTOR_FEEDBACK_POSITION = 1u << 0,
    BUS_MOTOR_FEEDBACK_VELOCITY = 1u << 1,
    BUS_MOTOR_FEEDBACK_TORQUE = 1u << 2,
    BUS_MOTOR_FEEDBACK_TEMPERATURE = 1u << 3,
} BusMotorFeedbackValid;

/**
 * @brief 电机温度反馈
 */
typedef struct {
    float motor; /**< 电机温度，单位 degC */
    float mos;   /**< MOS 温度，单位 degC */
} BusMotorTemperature;

/**
 * @brief 电机统一反馈
 */
typedef struct {
    BusMotorId id;                   /**< 业务逻辑电机 ID */
    uint32_t valid;                  /**< 有效字段位图 */
    uint8_t error_code;              /**< 最近一次反馈错误码 */
    float position;                  /**< 当前位置，单位 rad */
    float velocity;                  /**< 当前速度，单位 rad/s */
    float torque;                    /**< 当前扭矩，单位 N*m */
    BusMotorTemperature temperature; /**< 温度反馈 */
} BusMotorFeedback;

/**
 * @brief 电机组控制策略
 */
typedef enum {
    BUS_MOTOR_GROUP_POLICY_DEFAULT = 0u, /**< 允许通用逐台发送 */
    BUS_MOTOR_GROUP_POLICY_SYNCHRONIZED, /**< 要求驱动提供同步组控制 */
    BUS_MOTOR_GROUP_POLICY_ATOMIC,       /**< 要求驱动提供原子组控制 */
} BusMotorGroupPolicy;

/**
 * @brief 电机基础能力接口
 */
typedef struct {
    /**
     * @brief 使能电机
     * @param id 业务逻辑电机 ID
     * @return 电机状态码
     */
    BusMotorStatus (*enable)(BusMotorId id);

    /**
     * @brief 失能电机
     * @param id 业务逻辑电机 ID
     * @return 电机状态码
     */
    BusMotorStatus (*disable)(BusMotorId id);

    /**
     * @brief 停止电机
     * @param id 业务逻辑电机 ID
     * @return 电机状态码
     */
    BusMotorStatus (*stop)(BusMotorId id);

    /**
     * @brief 主动制动电机
     * @param id 业务逻辑电机 ID
     * @return 电机状态码
     */
    BusMotorStatus (*brake)(BusMotorId id);
} BusMotorBasicInterface;

/**
 * @brief 电机 Profile 接口
 */
typedef struct {
    /**
     * @brief 查询是否支持指定 Profile
     * @param id 业务逻辑电机 ID
     * @param profile 控制 Profile
     * @return true 表示支持，false 表示不支持
     */
    bool (*supports)(BusMotorId id, BusMotorProfile profile);

    /**
     * @brief 检查所需 Profile
     * @param id 业务逻辑电机 ID
     * @param profiles 所需 Profile 位图
     * @return 电机状态码
     */
    BusMotorStatus (*require)(BusMotorId id, BusMotorProfileMask profiles);

    /**
     * @brief 激活指定 Profile
     * @param id 业务逻辑电机 ID
     * @param profile 控制 Profile
     * @return 电机状态码
     */
    BusMotorStatus (*activate)(BusMotorId id, BusMotorProfile profile);

    /**
     * @brief 获取当前 Profile
     * @param id 业务逻辑电机 ID
     * @return 当前 Profile
     */
    BusMotorProfile (*current)(BusMotorId id);
} BusMotorProfileInterface;

/**
 * @brief 电机反馈接口
 */
typedef struct {
    /**
     * @brief 获取完整反馈
     * @param id 业务逻辑电机 ID
     * @param feedback 输出反馈
     * @return 电机状态码
     */
    BusMotorStatus (*all)(BusMotorId id, BusMotorFeedback* feedback);

    /**
     * @brief 获取位置反馈
     * @param id 业务逻辑电机 ID
     * @param position 输出位置，单位 rad
     * @return 电机状态码
     */
    BusMotorStatus (*position)(BusMotorId id, float* position);

    /**
     * @brief 获取速度反馈
     * @param id 业务逻辑电机 ID
     * @param velocity 输出速度，单位 rad/s
     * @return 电机状态码
     */
    BusMotorStatus (*velocity)(BusMotorId id, float* velocity);

    /**
     * @brief 获取扭矩反馈
     * @param id 业务逻辑电机 ID
     * @param torque 输出扭矩，单位 N*m
     * @return 电机状态码
     */
    BusMotorStatus (*torque)(BusMotorId id, float* torque);

    /**
     * @brief 获取温度反馈
     * @param id 业务逻辑电机 ID
     * @param temperature 输出温度反馈
     * @return 电机状态码
     */
    BusMotorStatus (*temperature)(BusMotorId id, BusMotorTemperature* temperature);
} BusMotorFeedbackInterface;

/**
 * @brief 电机组接口
 */
typedef struct {
    /**
     * @brief 绑定逻辑电机组
     * @param group_id 业务逻辑电机组 ID
     * @param motors 组内逻辑电机 ID 数组
     * @param count 组内电机数量
     * @return 电机状态码
     */
    BusMotorStatus (*bind)(BusMotorGroupId group_id, const BusMotorId* motors, uint8_t count);

    /**
     * @brief 使能电机组
     * @param group_id 业务逻辑电机组 ID
     * @return 电机状态码
     */
    BusMotorStatus (*enable)(BusMotorGroupId group_id);

    /**
     * @brief 失能电机组
     * @param group_id 业务逻辑电机组 ID
     * @return 电机状态码
     */
    BusMotorStatus (*disable)(BusMotorGroupId group_id);

    /**
     * @brief 停止电机组
     * @param group_id 业务逻辑电机组 ID
     * @return 电机状态码
     */
    BusMotorStatus (*stop)(BusMotorGroupId group_id);

    /**
     * @brief 制动电机组
     * @param group_id 业务逻辑电机组 ID
     * @return 电机状态码
     */
    BusMotorStatus (*brake)(BusMotorGroupId group_id);

    /**
     * @brief 激活电机组 Profile
     * @param group_id 业务逻辑电机组 ID
     * @param profile 控制 Profile
     * @return 电机状态码
     */
    BusMotorStatus (*activate)(BusMotorGroupId group_id, BusMotorProfile profile);

    /**
     * @brief 发送电机组控制命令
     * @param group_id 业务逻辑电机组 ID
     * @param commands 控制命令数组
     * @param count 控制命令数量
     * @param policy 电机组控制策略
     * @return 电机状态码
     */
    BusMotorStatus (*cmd)(BusMotorGroupId group_id, const BusMotorCommand* commands, uint8_t count,
                          BusMotorGroupPolicy policy);

    /**
     * @brief 获取电机组反馈
     * @param group_id 业务逻辑电机组 ID
     * @param feedback 输出反馈数组
     * @param count 反馈数组容量
     * @return 电机状态码
     */
    BusMotorStatus (*feedback)(BusMotorGroupId group_id, BusMotorFeedback* feedback, uint8_t count);
} BusMotorGroupInterface;

/**
 * @brief 电机底层端口函数表
 */
typedef struct {
    /**
     * @brief 发送一帧总线数据
     * @param id 总线帧 ID
     * @param data 帧数据
     * @param len 帧数据长度
     * @return true 表示成功，false 表示失败
     */
    bool (*send)(uint32_t id, const uint8_t* data, uint8_t len);

    /**
     * @brief 读取一帧总线数据
     * @param id 输出总线帧 ID
     * @param data 输出帧数据
     * @param len 输出帧数据长度
     * @return true 表示成功，false 表示失败
     */
    bool (*read)(uint32_t* id, uint8_t* data, uint8_t* len);

    /**
     * @brief 获取当前单调时间
     * @return 当前时间，单位 ms
     */
    uint32_t (*now_ms)(void);

    /**
     * @brief 可选阻塞延时
     * @param ms 延时时间，单位 ms
     */
    void (*delay_ms)(uint32_t ms);

    /**
     * @brief 可选清空接收缓冲
     */
    void (*flush_rx)(void);
} BusMotorPortOps;

/**
 * @brief 电机基础动作
 */
typedef enum {
    BUS_MOTOR_BASIC_ENABLE = 0u, /**< 使能 */
    BUS_MOTOR_BASIC_DISABLE,     /**< 失能 */
    BUS_MOTOR_BASIC_STOP,        /**< 停止 */
    BUS_MOTOR_BASIC_BRAKE,       /**< 主动制动 */
} BusMotorBasicAction;

/**
 * @brief 厂家驱动函数表
 */
typedef struct {
    /**
     * @brief 执行基础动作
     * @param instance 厂家驱动内部实例编号
     * @param action 基础动作
     * @return 电机状态码
     */
    BusMotorStatus (*basic)(uint16_t instance, BusMotorBasicAction action);

    /**
     * @brief 激活控制 Profile
     * @param instance 厂家驱动内部实例编号
     * @param profile 控制 Profile
     * @return 电机状态码
     */
    BusMotorStatus (*activate)(uint16_t instance, BusMotorProfile profile);

    /**
     * @brief 执行控制命令
     * @param instance 厂家驱动内部实例编号
     * @param command 控制命令
     * @return 电机状态码
     */
    BusMotorStatus (*command)(uint16_t instance, BusMotorCommand command);

    /**
     * @brief 获取完整反馈
     * @param instance 厂家驱动内部实例编号
     * @param feedback 输出反馈
     * @return 电机状态码
     */
    BusMotorStatus (*feedback)(uint16_t instance, BusMotorFeedback* feedback);

    /**
     * @brief 执行厂家原生电机组控制
     * @param instances 厂家驱动内部实例编号数组
     * @param commands 控制命令数组
     * @param count 电机数量
     * @param policy 电机组控制策略
     * @return 电机状态码
     */
    BusMotorStatus (*group_command)(const uint16_t* instances, const BusMotorCommand* commands, uint8_t count,
                                    BusMotorGroupPolicy policy);
} BusMotorDriver;

/**
 * @brief 总线电机统一接口单例
 */
typedef struct {
    /**
     * @brief 初始化 bus_motor 注册表
     * @return 电机状态码
     */
    BusMotorStatus (*init)(void);

    /**
     * @brief 状态码转字符串
     * @param status 电机状态码
     * @return 状态字符串
     */
    const char* (*status_str)(BusMotorStatus status);

    BusMotorBasicInterface basic;       /**< 基础能力 */
    BusMotorProfileInterface profile;   /**< Profile 能力 */
    BusMotorFeedbackInterface feedback; /**< 反馈能力 */
    BusMotorGroupInterface group;       /**< 电机组能力 */

    /**
     * @brief 高频位置控制
     * @param id 业务逻辑电机 ID
     * @param position 目标位置，单位 rad
     * @return 电机状态码
     */
    BusMotorStatus (*pos)(BusMotorId id, float position);

    /**
     * @brief 高频速度控制
     * @param id 业务逻辑电机 ID
     * @param velocity 目标速度，单位 rad/s
     * @return 电机状态码
     */
    BusMotorStatus (*vel)(BusMotorId id, float velocity);

    /**
     * @brief 高频扭矩控制
     * @param id 业务逻辑电机 ID
     * @param torque 目标扭矩，单位 N*m
     * @return 电机状态码
     */
    BusMotorStatus (*tor)(BusMotorId id, float torque);

    /**
     * @brief 高频阻抗控制
     * @param id 业务逻辑电机 ID
     * @param command 阻抗控制指令
     * @return 电机状态码
     */
    BusMotorStatus (*imp)(BusMotorId id, const BusMotorImpedanceCommand* command);

    /**
     * @brief 通用扩展控制
     * @param id 业务逻辑电机 ID
     * @param command 控制命令
     * @return 电机状态码
     */
    BusMotorStatus (*cmd)(BusMotorId id, BusMotorCommand command);
} BusMotorInterface;

/**
 * @brief 总线电机统一接口单例
 */
extern const BusMotorInterface bus_motor;

// ! ========================= 接 口 函 数 声 明 ========================= ! //

/**
 * @brief 注册厂家电机实例
 * @param id 业务逻辑电机 ID
 * @param driver 厂家驱动函数表
 * @param instance 厂家驱动内部实例编号
 * @param profiles 实例支持的 Profile 位图
 * @return 电机状态码
 */
BusMotorStatus bus_motor_driver_register(BusMotorId id, const BusMotorDriver* driver, uint16_t instance,
                                         BusMotorProfileMask profiles);

/**
 * @brief 注销厂家电机实例
 * @param id 业务逻辑电机 ID
 * @return 电机状态码
 */
BusMotorStatus bus_motor_driver_unregister(BusMotorId id);

/**
 * @brief 清除公共 Profile 状态
 * @param id 业务逻辑电机 ID
 * @return 电机状态码
 */
BusMotorStatus bus_motor_driver_reset_profile(BusMotorId id);

#endif
