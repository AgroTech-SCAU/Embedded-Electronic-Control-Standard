#ifndef _dm_motor_h_
#define _dm_motor_h_

#include "bus_motor.h"

// ! ========================= 接 口 变 量 / Typedef 声 明 ========================= ! //

/**
 * @brief 达妙电机命令帧长度，单位 byte
 */
#define DM_MOTOR_CMD_LEN 8u

/**
 * @brief 达妙电机 MIT 模式默认位置环比例系数
 */
#define DM_MOTOR_DEFAULT_KP 4.0f

/**
 * @brief 达妙电机 MIT 模式默认位置环微分系数
 */
#define DM_MOTOR_DEFAULT_KD 0.04f

/**
 * @brief 达妙电机 MIT 模式默认前馈扭矩
 */
#define DM_MOTOR_DEFAULT_TORQUE 0.0f

/**
 * @brief 达妙电机 MIT 模式 Kp 编码上限
 */
#define DM_MOTOR_MIT_KP_MAX 500.0f

/**
 * @brief 达妙电机 MIT 模式 Kd 编码上限
 */
#define DM_MOTOR_MIT_KD_MAX 5.0f

/**
 * @brief POS_FORCE 模式速度控制值上限
 */
#define DM_MOTOR_POS_FORCE_VEL_MAX 10000u

/**
 * @brief POS_FORCE 模式电流控制值上限
 */
#define DM_MOTOR_POS_FORCE_CURRENT_MAX 10000u

/**
 * @brief 达妙电机型号
 */
typedef enum {
    DM_MOTOR_MODEL_DM3507 = 0u, /**< DM3507 */
    DM_MOTOR_MODEL_DM4310,      /**< DM4310 */
    DM_MOTOR_MODEL_DM4310_48V,  /**< DM4310_48V */
    DM_MOTOR_MODEL_DM4340,      /**< DM4340 */
    DM_MOTOR_MODEL_DM4340_48V,  /**< DM4340_48V */
    DM_MOTOR_MODEL_DM6006,      /**< DM6006 */
    DM_MOTOR_MODEL_DM6248,      /**< DM6248 */
    DM_MOTOR_MODEL_DM8006,      /**< DM8006 */
    DM_MOTOR_MODEL_DM8009,      /**< DM8009 */
    DM_MOTOR_MODEL_DM10010L,    /**< DM10010L */
    DM_MOTOR_MODEL_DM10010,     /**< DM10010 */
    DM_MOTOR_MODEL_DMH3510,     /**< DMH3510 */
    DM_MOTOR_MODEL_DMH6215,     /**< DMH6215 */
    DM_MOTOR_MODEL_DMS3519,     /**< DMS3519 */
    DM_MOTOR_MODEL_DMG6220,     /**< DMG6220 */
} DmMotorModel;

/**
 * @brief 达妙电机固件大版本
 */
typedef enum {
    DM_MOTOR_FIRMWARE_UNKNOWN = 0u, /**< 未知固件版本 */
    DM_MOTOR_FIRMWARE_V3 = 3u,      /**< V3 固件 */
    DM_MOTOR_FIRMWARE_V4 = 4u,      /**< V4 固件 */
} DmMotorFirmwareMajor;

/**
 * @brief 达妙电机固件版本信息
 */
typedef struct {
    DmMotorFirmwareMajor major; /**< 固件大版本 */
    uint32_t sw_ver;            /**< sw_ver 寄存器原始值 */
    uint32_t sub_ver;           /**< sub_ver 寄存器原始值 */
} DmMotorFirmwareInfo;

/**
 * @brief 达妙电机 MIT 编解码量程
 */
typedef struct {
    float q_max;   /**< 最大位置绝对值，单位 rad */
    float dq_max;  /**< 最大速度绝对值，单位 rad/s */
    float tau_max; /**< 最大扭矩绝对值，单位 N*m */
} DmMotorLimitParam;

/**
 * @brief 达妙电机控制模式代码
 */
typedef enum {
    DM_MOTOR_MODE_MIT = 1u,       /**< MIT 模式 */
    DM_MOTOR_MODE_POS_VEL = 2u,   /**< POS_VEL 模式 */
    DM_MOTOR_MODE_VEL = 3u,       /**< VEL 模式 */
    DM_MOTOR_MODE_POS_FORCE = 4u, /**< POS_FORCE 模式 */
} DmMotorMode;

/**
 * @brief 单台达妙电机配置
 */
typedef struct {
    uint16_t can_id;              /**< 电机 CAN ID */
    uint16_t master_id;           /**< 电机反馈使用的 Master ID */
    DmMotorModel model;           /**< 电机型号 */
    DmMotorFirmwareInfo firmware; /**< 固件版本 */
    DmMotorMode default_mode;     /**< 绑定时的硬件控制模式 */
} DmMotorConfig;

/**
 * @brief 单台达妙电机实例存储
 *
 * 该类型用于由 assemble 按实际硬件数量提供静态实例内存
 * assemble 只负责分配存储，不应直接读写实例成员
 */
typedef struct {
    bool registered;                     /**< 实例是否已注册 */
    BusMotorId motor_id;                 /**< 业务逻辑电机 ID */
    uint16_t can_id;                     /**< 电机 CAN ID */
    uint16_t master_id;                  /**< 电机反馈使用的 Master ID */
    DmMotorModel model;                  /**< 电机型号 */
    DmMotorFirmwareInfo firmware;        /**< 固件版本 */
    DmMotorLimitParam limits;            /**< 当前 MIT 编解码量程 */
    DmMotorMode mode;                    /**< 当前硬件控制模式 */
    bool mode_confirmed;                 /**< 当前硬件控制模式是否已由应答确认 */
    float position;                      /**< 目标位置 */
    float velocity;                      /**< 目标速度 */
    float torque;                        /**< 目标扭矩 */
    float kp;                            /**< MIT 模式 Kp */
    float kd;                            /**< MIT 模式 Kd */
    bool has_feedback;                   /**< 是否收到有效反馈 */
    BusMotorFeedback feedback;           /**< 最近一次反馈 */
    uint32_t last_rx_ms;                 /**< 最近一次反馈时间 */
    uint32_t rx_count;                   /**< 已接收反馈帧数量 */
    volatile uint32_t mode_ack_sequence; /**< 模式应答序号 */
    volatile uint32_t mode_ack_value;    /**< 最近一次模式应答值 */
} DmMotorInstance;

// ! ========================= 接 口 函 数 声 明 ========================= ! //

/**
 * @brief 初始化达妙电机驱动
 * @param ops 底层总线端口函数表
 * @param instances assemble 提供的达妙电机实例数组
 * @param capacity 实例数组容量
 * @return 电机状态码
 */
BusMotorStatus dm_motor_init(const BusMotorPortOps* ops, DmMotorInstance* instances, uint16_t capacity);

/**
 * @brief 将业务逻辑电机 ID 绑定为达妙电机实例
 * @param motor_id 业务逻辑电机 ID
 * @param config 达妙电机配置
 * @return 电机状态码
 */
BusMotorStatus dm_motor_bind(BusMotorId motor_id, const DmMotorConfig* config);

/**
 * @brief 解析达妙电机反馈帧
 * @param frame_id CAN 反馈帧 ID
 * @param data 8 字节反馈数据
 * @param feedback 输出统一电机反馈，可为 0
 * @return 电机状态码
 */
BusMotorStatus dm_motor_parse_feedback_frame(uint32_t frame_id, const uint8_t data[DM_MOTOR_CMD_LEN],
                                             BusMotorFeedback* feedback);

/**
 * @brief 解析达妙电机参数读写应答
 * @param frame_id CAN 应答帧 ID
 * @param data 8 字节应答数据
 * @return true 表示该帧属于参数应答，false 表示应继续按状态反馈解析
 */
bool dm_motor_parse_parameter_frame(uint32_t frame_id, const uint8_t data[DM_MOTOR_CMD_LEN]);

/**
 * @brief 清除指定达妙电机错误
 * @param motor_id 业务逻辑电机 ID
 * @return 电机状态码
 */
BusMotorStatus dm_motor_clear_error(BusMotorId motor_id);

/**
 * @brief 保存指定达妙电机当前位置为零点
 * @param motor_id 业务逻辑电机 ID
 * @return 电机状态码
 */
BusMotorStatus dm_motor_save_zero(BusMotorId motor_id);

/**
 * @brief 切换指定达妙电机硬件模式
 * @param motor_id 业务逻辑电机 ID
 * @param mode 达妙硬件控制模式
 * @return 电机状态码
 */
BusMotorStatus dm_motor_switch_mode(BusMotorId motor_id, DmMotorMode mode);

/**
 * @brief 发送达妙 POS_VEL 控制指令
 * @param motor_id 业务逻辑电机 ID
 * @param position 目标位置，单位 rad
 * @param velocity 目标速度，单位 rad/s
 * @return 电机状态码
 */
BusMotorStatus dm_motor_set_pos_vel(BusMotorId motor_id, float position, float velocity);

/**
 * @brief 发送达妙 POS_FORCE 控制指令
 * @param motor_id 业务逻辑电机 ID
 * @param position 目标位置，单位 rad
 * @param velocity 速度控制值，范围 0-10000
 * @param current 电流控制值，范围 0-10000
 * @return 电机状态码
 */
BusMotorStatus dm_motor_set_pos_force(BusMotorId motor_id, float position, uint16_t velocity, uint16_t current);

#endif
