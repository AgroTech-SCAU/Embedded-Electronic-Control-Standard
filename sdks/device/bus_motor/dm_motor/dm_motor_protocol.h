#ifndef _dm_motor_protocol_h_
#define _dm_motor_protocol_h_

#include "dm_motor_core.h" // IWYU pragma: keep

// ! ========================= 接 口 变 量 / Typedef 声 明 ========================= ! //

/**
 * @brief 达妙电机特殊控制命令
 */
typedef enum {
    DM_MOTOR_SPECIAL_CLEAR_ERROR = 0xFBu, /**< 清除错误 */
    DM_MOTOR_SPECIAL_ENABLE = 0xFCu,      /**< 使能 */
    DM_MOTOR_SPECIAL_DISABLE = 0xFDu,     /**< 失能 */
    DM_MOTOR_SPECIAL_SAVE_ZERO = 0xFEu,   /**< 保存零点 */
} DmMotorSpecialCommand;

/**
 * @brief 达妙电机待发送协议帧
 */
typedef struct {
    uint32_t id;                    /**< CAN 帧 ID */
    uint8_t data[DM_MOTOR_CMD_LEN]; /**< 帧数据 */
    uint8_t len;                    /**< 帧数据长度 */
} DmMotorProtocolFrame;

/**
 * @brief 达妙电机固件协议接口
 */
typedef struct {
    /**
     * @brief 查询控制模式是否支持
     * @param mode 达妙电机控制模式
     * @return true 表示支持，false 表示不支持
     */
    bool (*supports_mode)(DmMotorMode mode);

    /**
     * @brief 生成当前控制模式指令
     * @param context 达妙电机实例
     * @param frame 输出协议帧
     * @return 电机状态码
     */
    BusMotorStatus (*build_control)(const DmMotorInstance* context, DmMotorProtocolFrame* frame);

    /**
     * @brief 生成 POS_FORCE 指令
     * @param context 达妙电机实例
     * @param position 目标位置，单位 rad
     * @param velocity 速度控制值
     * @param current 电流控制值
     * @param frame 输出协议帧
     * @return 电机状态码
     */
    BusMotorStatus (*build_pos_force)(const DmMotorInstance* context, float position, uint16_t velocity,
                                      uint16_t current, DmMotorProtocolFrame* frame);

    /**
     * @brief 生成特殊控制指令
     * @param context 达妙电机实例
     * @param command 特殊控制命令
     * @param frame 输出协议帧
     * @return 电机状态码
     */
    BusMotorStatus (*build_special)(const DmMotorInstance* context, DmMotorSpecialCommand command,
                                    DmMotorProtocolFrame* frame);

    /**
     * @brief 生成模式切换指令
     * @param context 达妙电机实例
     * @param mode 目标控制模式
     * @param frame 输出协议帧
     * @return 电机状态码
     */
    BusMotorStatus (*build_switch_mode)(const DmMotorInstance* context, DmMotorMode mode,
                                        DmMotorProtocolFrame* frame);

    /**
     * @brief 解析状态反馈
     * @param context 达妙电机实例
     * @param data 反馈帧数据
     * @param feedback 输出反馈
     * @return 电机状态码
     */
    BusMotorStatus (*decode_feedback)(const DmMotorInstance* context, const uint8_t data[DM_MOTOR_CMD_LEN],
                                      BusMotorFeedback* feedback);

    /**
     * @brief 判断参数应答
     * @param data 接收帧数据
     * @return true 表示参数应答，false 表示不是参数应答
     */
    bool (*is_parameter)(const uint8_t data[DM_MOTOR_CMD_LEN]);

    /**
     * @brief 解析参数应答
     * @param context 达妙电机实例
     * @param data 参数应答数据
     * @return true 表示解析成功，false 表示解析失败
     */
    bool (*parse_parameter)(DmMotorInstance* context, const uint8_t data[DM_MOTOR_CMD_LEN]);
} DmMotorProtocol;

/**
 * @brief 达妙电机 V3 协议实例
 */
extern const DmMotorProtocol dm_motor_protocol_v3;

/**
 * @brief 达妙电机 V4 协议实例
 */
extern const DmMotorProtocol dm_motor_protocol_v4;

#endif
