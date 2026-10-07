#ifndef _dji_motor_h_
#define _dji_motor_h_

#include "bus_motor.h"

#define DJI_MOTOR_CMD_LEN 8u
#define DJI_MOTOR_MAX_ID 8u

/** @brief 当前支持的电机与电调组合 */
typedef enum {
    DJI_MOTOR_MODEL_M2006 = 0u, /**< M2006 / C610 */
    DJI_MOTOR_MODEL_M3508,      /**< M3508 / C620 */
} DjiMotorModel;

/** @brief 单台电机配置，所有速度和位置均指减速箱输出轴 */
typedef struct {
    uint8_t can_id;                    /**< 物理 ID，范围 1-8 */
    DjiMotorModel model;
    uint32_t feedback_timeout_ms;      /**< 必须非零且小于 2^31 ms */
    float current_limit_a;             /**< 输出电流限幅，单位 A */
    float velocity_limit_rad_s;        /**< 速度目标限幅，单位 rad/s */
    float control_period_s;            /**< 固定控制调用周期，单位 s */
    float kp;                         /**< 速度 PID，A / (rad/s) */
    float ki;                         /**< 速度 PID，A / rad */
    float kd;                         /**< 速度 PID，A / (rad/s^2) */
    float feedback_current_a_per_lsb;  /**< 0 表示仅原始反馈，有确认依据后才能配置换算 */
} DjiMotorConfig;

/** @brief assemble 提供存储，业务不得直接修改成员 */
typedef struct {
    bool registered;
    bool enabled;
    bool has_feedback;
    BusMotorId motor_id;
    DjiMotorConfig config;
    BusMotorProfile profile;
    BusMotorFeedback feedback;
    uint32_t last_rx_ms;
    uint16_t last_angle;
    double total_ticks;
    float current_a;
    float integral;
    float last_error;
    bool pid_started;
} DjiMotorInstance;

/** @brief 初始化单 CAN 总线驱动，必须提供 send 和 now_ms */
BusMotorStatus dji_motor_init(const BusMotorPortOps* ops, DjiMotorInstance* instances, uint16_t capacity);
/** @brief 显式绑定逻辑 ID 与物理 ID，不在 SDK 固定机器人编号 */
BusMotorStatus dji_motor_bind(BusMotorId motor_id, const DjiMotorConfig* config);
/** @brief 接收完整 8 字节标准 CAN 数据帧，调用者负责校验 DLC 和帧类型 */
BusMotorStatus dji_motor_parse_feedback_frame(uint32_t frame_id, const uint8_t data[8], BusMotorFeedback* feedback);
/** @brief 每个控制周期调用，反馈超时则清零并锁存软件失能，恢复后须显式 enable */
BusMotorStatus dji_motor_update(void);

#endif
