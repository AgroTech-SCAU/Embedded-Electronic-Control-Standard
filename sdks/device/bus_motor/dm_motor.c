#include "dm_motor.h"

#include "dm_motor/dm_motor_core.h"
#include "dm_motor/dm_motor_protocol.h"

#include <math.h>
#include <stdbool.h>

// ! ========================= 变 量 声 明 ========================= ! //

#define DM_MOTOR_MODE_ACK_TIMEOUT_MS 100u

static const BusMotorPortOps* s_ops = 0;
static bool s_is_initialized = false;
static DmMotorRegistry s_registry;

// ! ========================= 私 有 函 数 声 明 ========================= ! //

/**
 * @brief 获取达妙电机协议实例
 * @param context 电机实例
 * @return 协议实例指针
 */
static const DmMotorProtocol* dm_motor_get_protocol(const DmMotorInstance* context);

/**
 * @brief 获取达妙电机支持的公共 Profile
 * @param context 电机实例
 * @return Profile 位图
 */
static BusMotorProfileMask dm_motor_get_profiles(const DmMotorInstance* context);

/**
 * @brief 将公共 Profile 映射为达妙硬件模式
 * @param profile 公共控制 Profile
 * @param mode 输出达妙硬件模式
 * @return 电机状态码
 */
static BusMotorStatus dm_motor_profile_to_mode(BusMotorProfile profile, DmMotorMode* mode);

/**
 * @brief 发送达妙协议帧
 * @param frame 协议帧
 * @return 电机状态码
 */
static BusMotorStatus dm_motor_send_frame(const DmMotorProtocolFrame* frame);

/**
 * @brief 发送达妙协议帧并执行帧间延时
 * @param frame 协议帧
 * @return 电机状态码
 */
static BusMotorStatus dm_motor_send_frame_delayed(const DmMotorProtocolFrame* frame);

/**
 * @brief 发送达妙特殊控制命令
 * @param context 电机实例
 * @param command 特殊控制命令
 * @return 电机状态码
 */
static BusMotorStatus dm_motor_send_special(DmMotorInstance* context, DmMotorSpecialCommand command);

/**
 * @brief 切换达妙电机硬件模式
 * @param context 电机实例
 * @param mode 目标硬件模式
 * @return 电机状态码
 */
static BusMotorStatus dm_motor_switch_mode_context(DmMotorInstance* context, DmMotorMode mode);

/**
 * @brief 按当前硬件模式发送控制指令
 * @param context 电机实例
 * @return 电机状态码
 */
static BusMotorStatus dm_motor_apply_command(DmMotorInstance* context);

/**
 * @brief bus_motor 基础动作接口
 * @param instance 达妙内部实例编号
 * @param action 基础动作
 * @return 电机状态码
 */
static BusMotorStatus dm_motor_driver_basic(uint16_t instance, BusMotorBasicAction action);

/**
 * @brief bus_motor Profile 激活接口
 * @param instance 达妙内部实例编号
 * @param profile 公共控制 Profile
 * @return 电机状态码
 */
static BusMotorStatus dm_motor_driver_activate(uint16_t instance, BusMotorProfile profile);

/**
 * @brief bus_motor 控制命令接口
 * @param instance 达妙内部实例编号
 * @param command 公共控制命令
 * @return 电机状态码
 */
static BusMotorStatus dm_motor_driver_command(uint16_t instance, BusMotorCommand command);

/**
 * @brief bus_motor 反馈接口
 * @param instance 达妙内部实例编号
 * @param feedback 输出统一反馈
 * @return 电机状态码
 */
static BusMotorStatus dm_motor_driver_feedback(uint16_t instance, BusMotorFeedback* feedback);

// ! ========================= 接 口 函 数 实 现 ========================= ! //

/**
 * @brief 达妙电机 bus_motor 驱动函数表
 */
static const BusMotorDriver s_dm_motor_driver = {
    .basic = dm_motor_driver_basic,
    .activate = dm_motor_driver_activate,
    .command = dm_motor_driver_command,
    .feedback = dm_motor_driver_feedback,
    .group_command = 0,
};

/**
 * @brief 初始化达妙电机驱动
 */
BusMotorStatus dm_motor_init(const BusMotorPortOps* ops, DmMotorInstance* instances, uint16_t capacity) {
    BusMotorStatus status;

    if(ops == 0 || ops->send == 0 || instances == 0 || capacity == 0u) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    status = dm_motor_registry_init(&s_registry, instances, capacity);
    if(status != MOTOR_STATUS_OK) {
        return status;
    }

    s_ops = ops;
    s_is_initialized = true;
    return MOTOR_STATUS_OK;
}

/**
 * @brief 将业务逻辑电机 ID 绑定为达妙电机实例
 */
BusMotorStatus dm_motor_bind(BusMotorId motor_id, const DmMotorConfig* config) {
    const DmMotorProtocol* protocol;
    DmMotorInstance* context;
    BusMotorProfileMask profiles;
    BusMotorStatus status;
    uint16_t instance;

    if(s_is_initialized == false) {
        return MOTOR_STATUS_NOT_INITIALIZE;
    }

    status = dm_motor_registry_register(&s_registry, motor_id, config, &instance, &context);
    if(status != MOTOR_STATUS_OK) {
        return status;
    }

    protocol = dm_motor_get_protocol(context);
    if(protocol == 0 || protocol->supports_mode == 0 || protocol->supports_mode(context->mode) == false) {
        dm_motor_registry_unregister(&s_registry, instance);
        return MOTOR_STATUS_UNSUPPORTED;
    }

    profiles = dm_motor_get_profiles(context);
    status = bus_motor_driver_register(motor_id, &s_dm_motor_driver, instance, profiles);
    if(status != MOTOR_STATUS_OK) {
        dm_motor_registry_unregister(&s_registry, instance);
        return status;
    }

    return MOTOR_STATUS_OK;
}

/**
 * @brief 解析达妙电机反馈帧
 */
BusMotorStatus dm_motor_parse_feedback_frame(uint32_t frame_id, const uint8_t data[DM_MOTOR_CMD_LEN],
                                             BusMotorFeedback* feedback) {
    BusMotorFeedback parsed = { 0 };
    DmMotorInstance* context;
    const DmMotorProtocol* protocol;
    BusMotorStatus status;
    uint16_t payload_id;

    if(data == 0) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    payload_id = (uint16_t)(data[0] & 0x0Fu);
    if(payload_id == 0u) {
        payload_id = (uint16_t)(frame_id & 0x0Fu);
    }

    context = dm_motor_registry_find_by_feedback(&s_registry, (uint16_t)frame_id, payload_id);
    if(context == 0) {
        return MOTOR_STATUS_ID_MISMATCH;
    }

    protocol = dm_motor_get_protocol(context);
    if(protocol == 0 || protocol->decode_feedback == 0) {
        return MOTOR_STATUS_UNSUPPORTED;
    }

    status = protocol->decode_feedback(context, data, &parsed);
    if(status != MOTOR_STATUS_OK) {
        return status;
    }

    parsed.id = context->motor_id;
    context->feedback = parsed;
    context->has_feedback = true;
    context->rx_count++;
    if(s_ops != 0 && s_ops->now_ms != 0) {
        context->last_rx_ms = s_ops->now_ms();
    }

    if(feedback != 0) {
        *feedback = parsed;
    }

    return MOTOR_STATUS_OK;
}

/**
 * @brief 解析达妙电机参数读写应答
 */
bool dm_motor_parse_parameter_frame(uint32_t frame_id, const uint8_t data[DM_MOTOR_CMD_LEN]) {
    DmMotorInstance* master_context;
    DmMotorInstance* context;
    const DmMotorProtocol* protocol;
    uint16_t can_id;

    if(data == 0) {
        return false;
    }

    master_context = dm_motor_registry_find_by_master_id(&s_registry, (uint16_t)frame_id);
    if(master_context == 0) {
        return false;
    }

    protocol = dm_motor_get_protocol(master_context);
    if(protocol == 0 || protocol->is_parameter == 0 || protocol->is_parameter(data) == false) {
        return false;
    }

    can_id = (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
    context = dm_motor_registry_find_by_can_id(&s_registry, can_id);
    if(context == 0 || context->master_id != (uint16_t)frame_id) {
        return true;
    }

    protocol = dm_motor_get_protocol(context);
    if(protocol == 0 || protocol->parse_parameter == 0) {
        return true;
    }

    protocol->parse_parameter(context, data);
    return true;
}

/**
 * @brief 清除指定达妙电机错误
 */
BusMotorStatus dm_motor_clear_error(BusMotorId motor_id) {
    DmMotorInstance* context = dm_motor_registry_find_by_motor_id(&s_registry, motor_id);

    if(context == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }

    return dm_motor_send_special(context, DM_MOTOR_SPECIAL_CLEAR_ERROR);
}

/**
 * @brief 保存指定达妙电机当前位置为零点
 */
BusMotorStatus dm_motor_save_zero(BusMotorId motor_id) {
    DmMotorInstance* context = dm_motor_registry_find_by_motor_id(&s_registry, motor_id);

    if(context == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }

    return dm_motor_send_special(context, DM_MOTOR_SPECIAL_SAVE_ZERO);
}

/**
 * @brief 切换指定达妙电机硬件模式
 */
BusMotorStatus dm_motor_switch_mode(BusMotorId motor_id, DmMotorMode mode) {
    DmMotorInstance* context = dm_motor_registry_find_by_motor_id(&s_registry, motor_id);
    BusMotorStatus status;

    if(context == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }

    status = dm_motor_switch_mode_context(context, mode);
    if(status == MOTOR_STATUS_OK) {
        bus_motor_driver_reset_profile(motor_id);
    }

    return status;
}

/**
 * @brief 发送达妙 POS_VEL 控制指令
 */
BusMotorStatus dm_motor_set_pos_vel(BusMotorId motor_id, float position, float velocity) {
    DmMotorInstance* context = dm_motor_registry_find_by_motor_id(&s_registry, motor_id);

    if(context == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }
    if(context->mode != DM_MOTOR_MODE_POS_VEL || !context->mode_confirmed) {
        return MOTOR_STATUS_PROFILE_MISMATCH;
    }
    if(!isfinite(position) || !isfinite(velocity) ||
       position < -context->limits.q_max || position > context->limits.q_max ||
       velocity < -context->limits.dq_max || velocity > context->limits.dq_max) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    context->position = position;
    context->velocity = velocity;
    return dm_motor_apply_command(context);
}

/**
 * @brief 发送达妙 POS_FORCE 控制指令
 */
BusMotorStatus dm_motor_set_pos_force(BusMotorId motor_id, float position, uint16_t velocity, uint16_t current) {
    DmMotorProtocolFrame frame;
    DmMotorInstance* context = dm_motor_registry_find_by_motor_id(&s_registry, motor_id);
    const DmMotorProtocol* protocol;
    BusMotorStatus status;

    if(context == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }
    if(context->mode != DM_MOTOR_MODE_POS_FORCE || velocity > DM_MOTOR_POS_FORCE_VEL_MAX ||
       current > DM_MOTOR_POS_FORCE_CURRENT_MAX) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    protocol = dm_motor_get_protocol(context);
    if(protocol == 0 || protocol->build_pos_force == 0) {
        return MOTOR_STATUS_UNSUPPORTED;
    }

    status = protocol->build_pos_force(context, position, velocity, current, &frame);
    if(status != MOTOR_STATUS_OK) {
        return status;
    }

    context->position = position;
    return dm_motor_send_frame(&frame);
}

// ! ========================= 私 有 函 数 实 现 ========================= ! //

/**
 * @brief 获取达妙电机协议实例
 */
static const DmMotorProtocol* dm_motor_get_protocol(const DmMotorInstance* context) {
    if(context == 0) {
        return 0;
    }

    switch(context->firmware.major) {
        case DM_MOTOR_FIRMWARE_V3:
            return &dm_motor_protocol_v3;
        case DM_MOTOR_FIRMWARE_V4:
            return &dm_motor_protocol_v4;
        default:
            return 0;
    }
}

/**
 * @brief 获取达妙电机支持的公共 Profile
 */
static BusMotorProfileMask dm_motor_get_profiles(const DmMotorInstance* context) {
    const DmMotorProtocol* protocol = dm_motor_get_protocol(context);
    BusMotorProfileMask profiles = 0u;

    if(protocol == 0 || protocol->supports_mode == 0) {
        return 0u;
    }
    if(protocol->supports_mode(DM_MOTOR_MODE_POS_VEL)) {
        profiles |= BUS_MOTOR_PROFILE_POSITION;
    }
    if(protocol->supports_mode(DM_MOTOR_MODE_VEL)) {
        profiles |= BUS_MOTOR_PROFILE_VELOCITY;
    }
    if(protocol->supports_mode(DM_MOTOR_MODE_MIT)) {
        profiles |= BUS_MOTOR_PROFILE_TORQUE | BUS_MOTOR_PROFILE_IMPEDANCE;
    }

    return profiles;
}

/**
 * @brief 将公共 Profile 映射为达妙硬件模式
 */
static BusMotorStatus dm_motor_profile_to_mode(BusMotorProfile profile, DmMotorMode* mode) {
    if(mode == 0) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    switch(profile) {
        case BUS_MOTOR_PROFILE_POSITION:
            *mode = DM_MOTOR_MODE_POS_VEL;
            return MOTOR_STATUS_OK;
        case BUS_MOTOR_PROFILE_VELOCITY:
            *mode = DM_MOTOR_MODE_VEL;
            return MOTOR_STATUS_OK;
        case BUS_MOTOR_PROFILE_TORQUE:
        case BUS_MOTOR_PROFILE_IMPEDANCE:
            *mode = DM_MOTOR_MODE_MIT;
            return MOTOR_STATUS_OK;
        default:
            return MOTOR_STATUS_UNSUPPORTED;
    }
}

/**
 * @brief 发送达妙协议帧
 */
static BusMotorStatus dm_motor_send_frame(const DmMotorProtocolFrame* frame) {
    if(s_is_initialized == false) {
        return MOTOR_STATUS_NOT_INITIALIZE;
    }
    if(frame == 0) {
        return MOTOR_STATUS_INVALID_PARAM;
    }
    if(s_ops == 0 || s_ops->send == 0) {
        return MOTOR_STATUS_PORT_ERROR;
    }
    if(s_ops->send(frame->id, frame->data, frame->len) == false) {
        return MOTOR_STATUS_PORT_ERROR;
    }

    return MOTOR_STATUS_OK;
}

/**
 * @brief 发送达妙协议帧并执行帧间延时
 */
static BusMotorStatus dm_motor_send_frame_delayed(const DmMotorProtocolFrame* frame) {
    BusMotorStatus status = dm_motor_send_frame(frame);

    if(status == MOTOR_STATUS_OK && s_ops != 0 && s_ops->delay_ms != 0) {
        s_ops->delay_ms(1u);
    }

    return status;
}

/**
 * @brief 发送达妙特殊控制命令
 */
static BusMotorStatus dm_motor_send_special(DmMotorInstance* context, DmMotorSpecialCommand command) {
    DmMotorProtocolFrame frame;
    const DmMotorProtocol* protocol;
    BusMotorStatus status;

    if(context == 0) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    protocol = dm_motor_get_protocol(context);
    if(protocol == 0 || protocol->build_special == 0) {
        return MOTOR_STATUS_UNSUPPORTED;
    }

    status = protocol->build_special(context, command, &frame);
    if(status != MOTOR_STATUS_OK) {
        return status;
    }

    return dm_motor_send_frame_delayed(&frame);
}

/**
 * @brief 切换达妙电机硬件模式
 */
static BusMotorStatus dm_motor_switch_mode_context(DmMotorInstance* context, DmMotorMode mode) {
    DmMotorProtocolFrame frame;
    const DmMotorProtocol* protocol;
    BusMotorStatus status;
    uint32_t ack_sequence;
    uint32_t elapsed_ms;

    if(context == 0) {
        return MOTOR_STATUS_INVALID_PARAM;
    }
    if(context->mode == mode && context->mode_confirmed) {
        return MOTOR_STATUS_OK;
    }

    protocol = dm_motor_get_protocol(context);
    if(protocol == 0 || protocol->supports_mode == 0 || protocol->build_switch_mode == 0 ||
       protocol->supports_mode(mode) == false) {
        return MOTOR_STATUS_UNSUPPORTED;
    }

    ack_sequence = context->mode_ack_sequence;
    status = protocol->build_switch_mode(context, mode, &frame);
    if(status != MOTOR_STATUS_OK) {
        return status;
    }

    status = dm_motor_send_frame_delayed(&frame);
    if(status != MOTOR_STATUS_OK) {
        return status;
    }
    if(s_ops == 0 || s_ops->delay_ms == 0) {
        return MOTOR_STATUS_PORT_ERROR;
    }

    for(elapsed_ms = 0u; elapsed_ms < DM_MOTOR_MODE_ACK_TIMEOUT_MS; ++elapsed_ms) {
        if(context->mode_ack_sequence != ack_sequence && context->mode_ack_value == (uint32_t)mode) {
            context->mode = mode;
            context->mode_confirmed = true;
            return MOTOR_STATUS_OK;
        }
        s_ops->delay_ms(1u);
    }

    return MOTOR_STATUS_TIMEOUT;
}

/**
 * @brief 按当前硬件模式发送控制指令
 */
static BusMotorStatus dm_motor_apply_command(DmMotorInstance* context) {
    DmMotorProtocolFrame frame;
    const DmMotorProtocol* protocol;
    BusMotorStatus status;

    if(context == 0) {
        return MOTOR_STATUS_INVALID_PARAM;
    }
    if(context->mode == DM_MOTOR_MODE_POS_FORCE) {
        return MOTOR_STATUS_UNSUPPORTED;
    }

    protocol = dm_motor_get_protocol(context);
    if(protocol == 0 || protocol->build_control == 0) {
        return MOTOR_STATUS_UNSUPPORTED;
    }

    status = protocol->build_control(context, &frame);
    if(status != MOTOR_STATUS_OK) {
        return status;
    }

    return dm_motor_send_frame(&frame);
}

/**
 * @brief bus_motor 基础动作接口
 */
static BusMotorStatus dm_motor_driver_basic(uint16_t instance, BusMotorBasicAction action) {
    DmMotorInstance* context = dm_motor_registry_at(&s_registry, instance);

    if(context == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }

    switch(action) {
        case BUS_MOTOR_BASIC_ENABLE:
            return dm_motor_send_special(context, DM_MOTOR_SPECIAL_ENABLE);

        case BUS_MOTOR_BASIC_DISABLE:
            return dm_motor_send_special(context, DM_MOTOR_SPECIAL_DISABLE);

        case BUS_MOTOR_BASIC_STOP:
            context->velocity = 0.0f;
            context->torque = 0.0f;
            return dm_motor_apply_command(context);

        case BUS_MOTOR_BASIC_BRAKE:
            if(context->mode == DM_MOTOR_MODE_VEL || context->mode == DM_MOTOR_MODE_POS_FORCE) {
                return MOTOR_STATUS_UNSUPPORTED;
            }
            if(context->has_feedback) {
                context->position = context->feedback.position;
            }
            context->velocity = 0.0f;
            context->torque = 0.0f;
            if(context->mode == DM_MOTOR_MODE_MIT) {
                context->kp = DM_MOTOR_DEFAULT_KP;
                context->kd = DM_MOTOR_DEFAULT_KD;
            }
            return dm_motor_apply_command(context);

        default:
            return MOTOR_STATUS_INVALID_PARAM;
    }
}

/**
 * @brief bus_motor Profile 激活接口
 */
static BusMotorStatus dm_motor_driver_activate(uint16_t instance, BusMotorProfile profile) {
    DmMotorInstance* context = dm_motor_registry_at(&s_registry, instance);
    DmMotorMode mode;
    BusMotorStatus status;

    if(context == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }

    status = dm_motor_profile_to_mode(profile, &mode);
    if(status != MOTOR_STATUS_OK) {
        return status;
    }

    return dm_motor_switch_mode_context(context, mode);
}

/**
 * @brief bus_motor 控制命令接口
 */
static BusMotorStatus dm_motor_driver_command(uint16_t instance, BusMotorCommand command) {
    DmMotorInstance* context = dm_motor_registry_at(&s_registry, instance);
    const BusMotorImpedanceCommand* imp;

    if(context == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }

    switch(command.type) {
        case BUS_MOTOR_CMD_POSITION:
            if(context->mode != DM_MOTOR_MODE_POS_VEL) {
                return MOTOR_STATUS_PROFILE_MISMATCH;
            }
            context->position = command.data.scalar;
            return dm_motor_apply_command(context);

        case BUS_MOTOR_CMD_VELOCITY:
            if(context->mode != DM_MOTOR_MODE_VEL) {
                return MOTOR_STATUS_PROFILE_MISMATCH;
            }
            context->velocity = command.data.scalar;
            return dm_motor_apply_command(context);

        case BUS_MOTOR_CMD_TORQUE:
            if(context->mode != DM_MOTOR_MODE_MIT || command.data.scalar < -context->limits.tau_max ||
               command.data.scalar > context->limits.tau_max) {
                return MOTOR_STATUS_INVALID_PARAM;
            }
            context->position = 0.0f;
            context->velocity = 0.0f;
            context->kp = 0.0f;
            context->kd = 0.0f;
            context->torque = command.data.scalar;
            return dm_motor_apply_command(context);

        case BUS_MOTOR_CMD_IMPEDANCE:
            imp = &command.data.imp;
            if(context->mode != DM_MOTOR_MODE_MIT || imp->position < -context->limits.q_max ||
               imp->position > context->limits.q_max || imp->velocity < -context->limits.dq_max ||
               imp->velocity > context->limits.dq_max || imp->kp < 0.0f || imp->kp > DM_MOTOR_MIT_KP_MAX ||
               imp->kd < 0.0f || imp->kd > DM_MOTOR_MIT_KD_MAX || imp->torque < -context->limits.tau_max ||
               imp->torque > context->limits.tau_max) {
                return MOTOR_STATUS_INVALID_PARAM;
            }
            context->position = imp->position;
            context->velocity = imp->velocity;
            context->kp = imp->kp;
            context->kd = imp->kd;
            context->torque = imp->torque;
            return dm_motor_apply_command(context);

        default:
            return MOTOR_STATUS_UNSUPPORTED;
    }
}

/**
 * @brief bus_motor 反馈接口
 */
static BusMotorStatus dm_motor_driver_feedback(uint16_t instance, BusMotorFeedback* feedback) {
    const DmMotorInstance* context = dm_motor_registry_at_const(&s_registry, instance);

    if(context == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }
    if(feedback == 0) {
        return MOTOR_STATUS_INVALID_PARAM;
    }
    if(context->has_feedback == false) {
        return MOTOR_STATUS_NO_FEEDBACK;
    }

    *feedback = context->feedback;
    return MOTOR_STATUS_OK;
}
