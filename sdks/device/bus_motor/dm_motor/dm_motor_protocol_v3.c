#include "dm_motor_protocol.h"

#include <string.h>

// ! ========================= 变 量 声 明 ========================= ! //

#define DM_MOTOR_V3_FRAME_ID_POS_VEL 0x100u
#define DM_MOTOR_V3_FRAME_ID_VEL 0x200u
#define DM_MOTOR_V3_FRAME_ID_POS_FORCE 0x300u
#define DM_MOTOR_V3_REG_FRAME_ID 0x7FFu
#define DM_MOTOR_V3_REG_CMD_READ 0x33u
#define DM_MOTOR_V3_REG_CMD_WRITE 0x55u
#define DM_MOTOR_V3_REG_MODE_ID 10u

// ! ========================= 私 有 函 数 声 明 ========================= ! //

/**
 * @brief dm_motor_v3_supports_mode 内部辅助函数
 * @param mode 控制模式
 * @return true 表示支持，false 表示不支持
 */
static bool dm_motor_v3_supports_mode(DmMotorMode mode);

/**
 * @brief dm_motor_v3_build_control 内部辅助函数
 * @param context 电机实例
 * @param frame 输出协议帧
 * @return 电机状态码
 */
static BusMotorStatus dm_motor_v3_build_control(const DmMotorInstance* context, DmMotorProtocolFrame* frame);

/**
 * @brief dm_motor_v3_build_pos_force 内部辅助函数
 * @param context 电机实例
 * @param position 目标位置
 * @param velocity 速度控制值，范围 0-10000
 * @param current 电流控制值，范围 0-10000
 * @param frame 输出协议帧
 * @return 电机状态码
 */
static BusMotorStatus dm_motor_v3_build_pos_force(const DmMotorInstance* context, float position, uint16_t velocity,
                                                  uint16_t current, DmMotorProtocolFrame* frame);

/**
 * @brief dm_motor_v3_build_special 内部辅助函数
 * @param context 电机实例
 * @param command 特殊控制命令
 * @param frame 输出协议帧
 * @return 电机状态码
 */
static BusMotorStatus dm_motor_v3_build_special(const DmMotorInstance* context, DmMotorSpecialCommand command,
                                                DmMotorProtocolFrame* frame);

/**
 * @brief dm_motor_v3_build_switch_mode 内部辅助函数
 * @param context 电机实例
 * @param mode 控制模式
 * @param frame 输出协议帧
 * @return 电机状态码
 */
static BusMotorStatus dm_motor_v3_build_switch_mode(const DmMotorInstance* context, DmMotorMode mode,
                                                    DmMotorProtocolFrame* frame);

/**
 * @brief dm_motor_v3_decode_feedback 内部辅助函数
 * @param context 电机实例
 * @param data 反馈数据
 * @param feedback 输出通用反馈
 * @return 电机状态码
 */
static BusMotorStatus dm_motor_v3_decode_feedback(const DmMotorInstance* context, const uint8_t data[DM_MOTOR_CMD_LEN],
                                                  BusMotorFeedback* feedback);

/**
 * @brief dm_motor_v3_is_parameter 内部辅助函数
 * @param data 参数应答数据
 * @return true 表示参数应答，false 表示普通状态反馈
 */
static bool dm_motor_v3_is_parameter(const uint8_t data[DM_MOTOR_CMD_LEN]);

/**
 * @brief dm_motor_v3_parse_parameter 内部辅助函数
 * @param context 电机实例
 * @param data 参数应答数据
 * @return true 表示参数应答，false 表示普通状态反馈
 */
static bool dm_motor_v3_parse_parameter(DmMotorInstance* context, const uint8_t data[DM_MOTOR_CMD_LEN]);

// ! ========================= 接 口 函 数 实 现 ========================= ! //

/**
 * @brief 达妙电机 V3 协议实例
 */
const DmMotorProtocol dm_motor_protocol_v3 = {
    .supports_mode = dm_motor_v3_supports_mode,
    .build_control = dm_motor_v3_build_control,
    .build_pos_force = dm_motor_v3_build_pos_force,
    .build_special = dm_motor_v3_build_special,
    .build_switch_mode = dm_motor_v3_build_switch_mode,
    .decode_feedback = dm_motor_v3_decode_feedback,
    .is_parameter = dm_motor_v3_is_parameter,
    .parse_parameter = dm_motor_v3_parse_parameter,
};

// ! ========================= 私 有 函 数 实 现 ========================= ! //

/**
 * @brief 查询达妙电机 V3 协议控制模式是否支持
 */
static bool dm_motor_v3_supports_mode(DmMotorMode mode) {
    switch(mode) {
        case DM_MOTOR_MODE_MIT:
        case DM_MOTOR_MODE_POS_VEL:
        case DM_MOTOR_MODE_VEL:
        case DM_MOTOR_MODE_POS_FORCE:
            return true;
        default:
            return false;
    }
}

/**
 * @brief 生成达妙电机 V3 协议控制指令
 */
static BusMotorStatus dm_motor_v3_build_control(const DmMotorInstance* context, DmMotorProtocolFrame* frame) {
    if(context == 0 || frame == 0) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    memset(frame, 0, sizeof(*frame));
    switch(context->mode) {
        case DM_MOTOR_MODE_MIT:
            frame->id = context->can_id;
            frame->len = DM_MOTOR_CMD_LEN;
            dm_motor_codec_encode_mit(&context->limits, context->position, context->velocity,
                                      context->kp, context->kd, context->torque, frame->data);
            return MOTOR_STATUS_OK;
        case DM_MOTOR_MODE_POS_VEL:
            frame->id = (uint32_t)context->can_id + DM_MOTOR_V3_FRAME_ID_POS_VEL;
            frame->len = DM_MOTOR_CMD_LEN;
            dm_motor_codec_encode_pos_vel(context->position, context->velocity, frame->data);
            return MOTOR_STATUS_OK;
        case DM_MOTOR_MODE_VEL:
            frame->id = (uint32_t)context->can_id + DM_MOTOR_V3_FRAME_ID_VEL;
            frame->len = 4u;
            dm_motor_codec_encode_vel(context->velocity, frame->data);
            return MOTOR_STATUS_OK;
        default:
            return MOTOR_STATUS_UNSUPPORTED;
    }
}

/**
 * @brief 生成达妙电机 V3 协议 POS_FORCE 指令
 */
static BusMotorStatus dm_motor_v3_build_pos_force(const DmMotorInstance* context, float position, uint16_t velocity,
                                                  uint16_t current, DmMotorProtocolFrame* frame) {
    if(context == 0 || frame == 0 || context->mode != DM_MOTOR_MODE_POS_FORCE) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    memset(frame, 0, sizeof(*frame));
    frame->id = (uint32_t)context->can_id + DM_MOTOR_V3_FRAME_ID_POS_FORCE;
    frame->len = DM_MOTOR_CMD_LEN;
    dm_motor_codec_encode_pos_force(position, velocity, current, frame->data);
    return MOTOR_STATUS_OK;
}

/**
 * @brief 生成达妙电机 V3 协议特殊控制指令
 */
static BusMotorStatus dm_motor_v3_build_special(const DmMotorInstance* context, DmMotorSpecialCommand command,
                                                DmMotorProtocolFrame* frame) {
    if(context == 0 || frame == 0) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    memset(frame, 0xFF, sizeof(*frame));
    frame->id = context->can_id;
    frame->len = DM_MOTOR_CMD_LEN;
    frame->data[7] = (uint8_t)command;
    return MOTOR_STATUS_OK;
}

/**
 * @brief 生成达妙电机 V3 协议模式切换指令
 */
static BusMotorStatus dm_motor_v3_build_switch_mode(const DmMotorInstance* context, DmMotorMode mode,
                                                    DmMotorProtocolFrame* frame) {
    if(context == 0 || frame == 0 || dm_motor_v3_supports_mode(mode) == false) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    memset(frame, 0, sizeof(*frame));
    frame->id = DM_MOTOR_V3_REG_FRAME_ID;
    frame->len = DM_MOTOR_CMD_LEN;
    frame->data[0] = (uint8_t)(context->can_id & 0xFFu);
    frame->data[1] = (uint8_t)(context->can_id >> 8);
    frame->data[2] = DM_MOTOR_V3_REG_CMD_WRITE;
    frame->data[3] = DM_MOTOR_V3_REG_MODE_ID;
    frame->data[4] = (uint8_t)mode;
    return MOTOR_STATUS_OK;
}

/**
 * @brief 解析达妙电机 V3 协议状态反馈
 */
static BusMotorStatus dm_motor_v3_decode_feedback(const DmMotorInstance* context, const uint8_t data[DM_MOTOR_CMD_LEN],
                                                  BusMotorFeedback* feedback) {
    if(context == 0 || data == 0 || feedback == 0) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    feedback->id = context->motor_id;
    feedback->valid = BUS_MOTOR_FEEDBACK_POSITION | BUS_MOTOR_FEEDBACK_VELOCITY |
                      BUS_MOTOR_FEEDBACK_TORQUE | BUS_MOTOR_FEEDBACK_TEMPERATURE;
    feedback->error_code = (uint8_t)(data[0] >> 4);
    dm_motor_codec_decode_feedback(&context->limits, data, &feedback->position, &feedback->velocity, &feedback->torque);
    feedback->temperature.mos = (float)data[6];
    feedback->temperature.motor = (float)data[7];
    return MOTOR_STATUS_OK;
}

/**
 * @brief 判断达妙电机 V3 协议参数应答
 */
static bool dm_motor_v3_is_parameter(const uint8_t data[DM_MOTOR_CMD_LEN]) {
    if(data == 0) {
        return false;
    }

    return data[2] == DM_MOTOR_V3_REG_CMD_READ || data[2] == DM_MOTOR_V3_REG_CMD_WRITE;
}

/**
 * @brief 解析达妙电机 V3 协议参数应答
 */
static bool dm_motor_v3_parse_parameter(DmMotorInstance* context, const uint8_t data[DM_MOTOR_CMD_LEN]) {
    uint32_t value;

    if(context == 0 || data == 0) {
        return false;
    }
    if(dm_motor_v3_is_parameter(data) == false) {
        return false;
    }

    if(data[3] == DM_MOTOR_V3_REG_MODE_ID) {
        value = (uint32_t)data[4] |
                ((uint32_t)data[5] << 8) |
                ((uint32_t)data[6] << 16) |
                ((uint32_t)data[7] << 24);
        context->mode_ack_value = value;
        context->mode_ack_sequence++;
    }

    return true;
}
