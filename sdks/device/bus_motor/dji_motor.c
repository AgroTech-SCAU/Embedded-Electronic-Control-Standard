#include "dji_motor.h"

#include <math.h>
#include <float.h>
#include <string.h>

// ! ========================= 变 量 声 明 ========================= ! //
#define DJI_RAD_PER_ROUND 6.28318530718f
static const BusMotorPortOps* s_ops;
static DjiMotorInstance* s_instances;
static uint16_t s_capacity;
static bool s_zero_pending[2];

static DjiMotorInstance* dji_at(uint16_t index) {
    return s_instances != 0 && index < s_capacity && s_instances[index].registered ? &s_instances[index] : 0;
}
static float dji_ratio(const DjiMotorInstance* motor) {
    return motor->config.model == DJI_MOTOR_MODEL_M2006 ? 36.0f : 3591.0f / 187.0f;
}
static float dji_command_scale(const DjiMotorInstance* motor) {
    return motor->config.model == DJI_MOTOR_MODEL_M2006 ? 1000.0f : 16384.0f / 20.0f;
}
static uint8_t dji_bank(const DjiMotorInstance* motor) { return (uint8_t)((motor->config.can_id - 1u) / 4u); }
static void dji_reset_output(DjiMotorInstance* motor) {
    motor->current_a = 0.0f;
    motor->integral = 0.0f;
    motor->last_error = 0.0f;
    motor->pid_started = false;
}
static BusMotorStatus dji_fresh(const DjiMotorInstance* motor) {
    if(!motor->has_feedback) return MOTOR_STATUS_NO_FEEDBACK;
    if((uint32_t)(s_ops->now_ms() - motor->last_rx_ms) >= motor->config.feedback_timeout_ms) return MOTOR_STATUS_TIMEOUT;
    return MOTOR_STATUS_OK;
}

/** @brief 一帧发送同一 bank，未绑定槽位清零，发送失败锁存全部软件失能 */
static BusMotorStatus dji_send_bank(uint8_t bank) {
    uint8_t data[8] = { 0 };
    uint16_t i;
    for(i = 0u; i < s_capacity; ++i) {
        DjiMotorInstance* motor = dji_at(i);
        if(motor == 0 || dji_bank(motor) != bank) continue;
        if(dji_fresh(motor) != MOTOR_STATUS_OK) {
            dji_reset_output(motor);
            motor->enabled = false;
        }
        if(motor->enabled) {
            int16_t current = (int16_t)lroundf(motor->current_a * dji_command_scale(motor));
            uint8_t slot = (uint8_t)((motor->config.can_id - 1u) % 4u);
            data[slot * 2u] = (uint8_t)((uint16_t)current >> 8);
            data[slot * 2u + 1u] = (uint8_t)current;
        }
    }
    if(!s_ops->send(bank == 0u ? 0x200u : 0x1FFu, data, 8u)) {
        for(i = 0u; i < s_capacity; ++i) {
            DjiMotorInstance* motor = dji_at(i);
            if(motor != 0) { dji_reset_output(motor); motor->enabled = false; s_zero_pending[dji_bank(motor)] = true; }
        }
        return MOTOR_STATUS_PORT_ERROR;
    }
    s_zero_pending[bank] = false;
    return MOTOR_STATUS_OK;
}

static BusMotorStatus dji_basic(uint16_t index, BusMotorBasicAction action) {
    DjiMotorInstance* motor = dji_at(index);
    if(motor == 0) return MOTOR_STATUS_NOT_FOUND;
    if(action == BUS_MOTOR_BASIC_BRAKE) return MOTOR_STATUS_UNSUPPORTED;
    if(action == BUS_MOTOR_BASIC_ENABLE) {
        if(s_zero_pending[dji_bank(motor)]) return MOTOR_STATUS_PORT_ERROR;
        BusMotorStatus status = dji_fresh(motor);
        if(status != MOTOR_STATUS_OK) return status;
        dji_reset_output(motor);
        motor->enabled = true;
        return MOTOR_STATUS_OK;
    }
    if(action != BUS_MOTOR_BASIC_STOP && action != BUS_MOTOR_BASIC_DISABLE) return MOTOR_STATUS_INVALID_PARAM;
    dji_reset_output(motor);
    if(action == BUS_MOTOR_BASIC_DISABLE) motor->enabled = false;
    return dji_send_bank(dji_bank(motor));
}
static BusMotorStatus dji_activate(uint16_t index, BusMotorProfile profile) {
    DjiMotorInstance* motor = dji_at(index);
    if(motor == 0) return MOTOR_STATUS_NOT_FOUND;
    if(profile != BUS_MOTOR_PROFILE_VELOCITY && profile != BUS_MOTOR_PROFILE_CURRENT_Q) return MOTOR_STATUS_UNSUPPORTED;
    /* 切换前发送零电流，失败时不提交新 Profile */
    dji_reset_output(motor);
    BusMotorStatus status = dji_send_bank(dji_bank(motor));
    if(status == MOTOR_STATUS_OK) motor->profile = profile;
    return status;
}
static BusMotorStatus dji_validate(DjiMotorInstance* motor, BusMotorCommand command) {
    if(!isfinite(command.data.scalar)) return MOTOR_STATUS_INVALID_PARAM;
    if(command.type != BUS_MOTOR_CMD_VELOCITY && command.type != BUS_MOTOR_CMD_CURRENT_Q) return MOTOR_STATUS_UNSUPPORTED;
    if((command.type == BUS_MOTOR_CMD_VELOCITY && motor->profile != BUS_MOTOR_PROFILE_VELOCITY) ||
       (command.type == BUS_MOTOR_CMD_CURRENT_Q && motor->profile != BUS_MOTOR_PROFILE_CURRENT_Q)) return MOTOR_STATUS_PROFILE_MISMATCH;
    if(command.type == BUS_MOTOR_CMD_CURRENT_Q && fabsf(command.data.scalar) > motor->config.current_limit_a) return MOTOR_STATUS_INVALID_PARAM;
    if(command.type == BUS_MOTOR_CMD_VELOCITY && fabsf(command.data.scalar) > motor->config.velocity_limit_rad_s) return MOTOR_STATUS_INVALID_PARAM;
    if(!motor->enabled) return MOTOR_STATUS_NOT_INITIALIZE;
    return dji_fresh(motor);
}
static void dji_apply(DjiMotorInstance* motor, BusMotorCommand command) {
    if(command.type == BUS_MOTOR_CMD_CURRENT_Q) { motor->current_a = command.data.scalar; return; }
    double error = (double)command.data.scalar - (double)motor->feedback.velocity;
    double integral = motor->integral + (double)motor->config.ki * error * motor->config.control_period_s;
    double limit = motor->config.current_limit_a;
    integral = fmax(-limit, fmin(limit, integral));
    double derivative = motor->pid_started ? (error - motor->last_error) / motor->config.control_period_s : 0.0f;
    double output = (double)motor->config.kp * error + integral + (double)motor->config.kd * derivative;
    motor->current_a = (float)fmax(-limit, fmin(limit, output));
    /* 饱和时冻结同向积分，禁止累计不可释放的输出 */
    if(fabs(output) <= limit || error * output <= 0.0) motor->integral = (float)integral;
    motor->last_error = (float)error;
    motor->pid_started = true;
}
static BusMotorStatus dji_command(uint16_t index, BusMotorCommand command) {
    DjiMotorInstance* motor = dji_at(index);
    if(motor == 0) return MOTOR_STATUS_NOT_FOUND;
    BusMotorStatus status = dji_validate(motor, command);
    if(status == MOTOR_STATUS_TIMEOUT || status == MOTOR_STATUS_NO_FEEDBACK) {
        dji_reset_output(motor); motor->enabled = false;
        BusMotorStatus sent = dji_send_bank(dji_bank(motor));
        return sent == MOTOR_STATUS_OK ? status : sent;
    }
    if(status != MOTOR_STATUS_OK) return status;
    dji_apply(motor, command);
    return dji_send_bank(dji_bank(motor));
}
static BusMotorStatus dji_feedback(uint16_t index, BusMotorFeedback* feedback) {
    DjiMotorInstance* motor = dji_at(index);
    if(motor == 0) return MOTOR_STATUS_NOT_FOUND;
    if(feedback == 0) return MOTOR_STATUS_INVALID_PARAM;
    BusMotorStatus status = dji_fresh(motor);
    if(status == MOTOR_STATUS_OK) *feedback = motor->feedback;
    return status;
}
static BusMotorStatus dji_group(const uint16_t* indices, const BusMotorCommand* commands, uint8_t count, BusMotorGroupPolicy policy) {
    uint8_t i, j, bank;
    DjiMotorInstance* first;
    if(indices == 0 || commands == 0 || count == 0u || count > 4u) return MOTOR_STATUS_UNSUPPORTED;
    if(policy != BUS_MOTOR_GROUP_POLICY_DEFAULT && policy != BUS_MOTOR_GROUP_POLICY_SYNCHRONIZED && policy != BUS_MOTOR_GROUP_POLICY_ATOMIC) return MOTOR_STATUS_INVALID_PARAM;
    first = dji_at(indices[0]);
    if(first == 0) return MOTOR_STATUS_NOT_FOUND;
    bank = dji_bank(first);
    for(i = 0u; i < count; ++i) {
        DjiMotorInstance* motor = dji_at(indices[i]);
        if(motor == 0) return MOTOR_STATUS_NOT_FOUND;
        if(dji_bank(motor) != bank) return MOTOR_STATUS_UNSUPPORTED;
        for(j = 0u; j < i; ++j) if(indices[j] == indices[i]) return MOTOR_STATUS_INVALID_PARAM;
    }
    /* 完整拓扑先验证，不合法的跨 bank 组不产生发送或软件状态副作用 */
    for(i = 0u; i < count; ++i) {
        DjiMotorInstance* motor = dji_at(indices[i]);
        BusMotorStatus status = dji_validate(motor, commands[i]);
        if(status == MOTOR_STATUS_TIMEOUT || status == MOTOR_STATUS_NO_FEEDBACK) {
            for(j = 0u; j < count; ++j) {
                DjiMotorInstance* member = dji_at(indices[j]);
                if(member != 0) { dji_reset_output(member); member->enabled = false; }
            }
            BusMotorStatus sent = dji_send_bank(bank);
            return sent == MOTOR_STATUS_OK ? status : sent;
        }
        if(status != MOTOR_STATUS_OK) return status;
    }
    for(i = 0u; i < count; ++i) dji_apply(dji_at(indices[i]), commands[i]);
    return dji_send_bank(bank);
}
static const BusMotorDriver s_driver = {
    .basic = dji_basic, .activate = dji_activate, .command = dji_command,
    .feedback = dji_feedback, .group_command = dji_group,
};

// ! ========================= 接 口 函 数 实 现 ========================= ! //
BusMotorStatus dji_motor_init(const BusMotorPortOps* ops, DjiMotorInstance* instances, uint16_t capacity) {
    if(ops == 0 || ops->send == 0 || ops->now_ms == 0 || instances == 0 || capacity == 0u || capacity > DJI_MOTOR_MAX_ID) return MOTOR_STATUS_INVALID_PARAM;
    /* 必须先重置公共注册表，防止重初始化销毁仍绑定的实例 */
    for(uint16_t i = 0u; i < s_capacity; ++i) {
        DjiMotorInstance* old = dji_at(i);
        if(old != 0 && bus_motor.profile.supports(old->motor_id, BUS_MOTOR_PROFILE_CURRENT_Q)) return MOTOR_STATUS_ALREADY_BOUND;
    }
    if(s_zero_pending[0] || s_zero_pending[1]) return MOTOR_STATUS_PORT_ERROR;
    memset(instances, 0, sizeof(*instances) * capacity);
    s_ops = ops; s_instances = instances; s_capacity = capacity;
    return MOTOR_STATUS_OK;
}
BusMotorStatus dji_motor_bind(BusMotorId motor_id, const DjiMotorConfig* config) {
    if(s_ops == 0) return MOTOR_STATUS_NOT_INITIALIZE;
    if(config == 0 || config->can_id == 0u || config->can_id > 8u ||
       (config->model != DJI_MOTOR_MODEL_M2006 && config->model != DJI_MOTOR_MODEL_M3508) ||
       config->feedback_timeout_ms == 0u || config->feedback_timeout_ms >= 0x80000000u ||
       !isfinite(config->current_limit_a) || config->current_limit_a <= 0.0f ||
       config->current_limit_a > (config->model == DJI_MOTOR_MODEL_M2006 ? 10.0f : 20.0f) ||
       !isfinite(config->velocity_limit_rad_s) || config->velocity_limit_rad_s <= 0.0f ||
       !isfinite(config->control_period_s) || config->control_period_s <= 0.0f ||
       !isfinite(config->kp) || config->kp < 0.0f || !isfinite(config->ki) || config->ki < 0.0f ||
       !isfinite(config->kd) || config->kd < 0.0f || !isfinite(config->feedback_current_a_per_lsb) ||
       config->feedback_current_a_per_lsb < 0.0f || config->feedback_current_a_per_lsb > FLT_MAX / 32768.0f) return MOTOR_STATUS_INVALID_PARAM;
    for(uint16_t i = 0u; i < s_capacity; ++i) {
        if(dji_at(i) != 0 && (s_instances[i].config.can_id == config->can_id || s_instances[i].motor_id == motor_id)) return MOTOR_STATUS_ALREADY_BOUND;
    }
    for(uint16_t i = 0u; i < s_capacity; ++i) {
        if(dji_at(i) != 0) continue;
        BusMotorStatus status = bus_motor_driver_register(motor_id, &s_driver, i, BUS_MOTOR_PROFILE_VELOCITY | BUS_MOTOR_PROFILE_CURRENT_Q);
        if(status != MOTOR_STATUS_OK) return status;
        s_instances[i] = (DjiMotorInstance){ .registered = true, .motor_id = motor_id, .config = *config };
        return MOTOR_STATUS_OK;
    }
    return MOTOR_STATUS_NO_RESOURCE;
}
BusMotorStatus dji_motor_parse_feedback_frame(uint32_t frame_id, const uint8_t data[8], BusMotorFeedback* feedback) {
    if(s_ops == 0) return MOTOR_STATUS_NOT_INITIALIZE;
    if(data == 0) return MOTOR_STATUS_INVALID_PARAM;
    if(frame_id < 0x201u || frame_id > 0x208u) return MOTOR_STATUS_ID_MISMATCH;
    for(uint16_t i = 0u; i < s_capacity; ++i) {
        DjiMotorInstance* motor = dji_at(i);
        if(motor == 0 || 0x200u + motor->config.can_id != frame_id) continue;
        uint16_t angle = (uint16_t)((uint16_t)data[0] << 8 | data[1]);
        if(angle >= 8192u) return MOTOR_STATUS_INVALID_PARAM;
        int rpm_bits = ((int)data[2] << 8) | data[3];
        int raw_bits = ((int)data[4] << 8) | data[5];
        int rpm = rpm_bits >= 32768 ? rpm_bits - 65536 : rpm_bits;
        int raw = raw_bits >= 32768 ? raw_bits - 65536 : raw_bits;
        int delta = motor->has_feedback ? (int)angle - (int)motor->last_angle : 0;
        if(delta > 4096) delta -= 8192;
        if(delta < -4096) delta += 8192;
        motor->total_ticks += delta;
        motor->last_angle = angle;
        motor->last_rx_ms = s_ops->now_ms();
        motor->has_feedback = true;
        motor->feedback = (BusMotorFeedback){
            .id = motor->motor_id,
            .valid = BUS_MOTOR_FEEDBACK_POSITION | BUS_MOTOR_FEEDBACK_VELOCITY | BUS_MOTOR_FEEDBACK_CURRENT_RAW,
            .position = (float)(motor->total_ticks * DJI_RAD_PER_ROUND / 8192.0 / dji_ratio(motor)),
            .velocity = (float)rpm * DJI_RAD_PER_ROUND / 60.0f / dji_ratio(motor),
            .current_raw = (int16_t)raw,
        };
        if(motor->config.feedback_current_a_per_lsb > 0.0f) {
            motor->feedback.current = (float)raw * motor->config.feedback_current_a_per_lsb;
            motor->feedback.valid |= BUS_MOTOR_FEEDBACK_CURRENT;
        }
        if(motor->config.model == DJI_MOTOR_MODEL_M3508) {
            motor->feedback.temperature.motor = (float)data[6];
            motor->feedback.valid |= BUS_MOTOR_FEEDBACK_TEMPERATURE;
        }
        if(feedback != 0) *feedback = motor->feedback;
        return MOTOR_STATUS_OK;
    }
    return MOTOR_STATUS_ID_MISMATCH;
}
BusMotorStatus dji_motor_update(void) {
    if(s_ops == 0) return MOTOR_STATUS_NOT_INITIALIZE;
    bool banks[2] = { s_zero_pending[0], s_zero_pending[1] };
    BusMotorStatus result = MOTOR_STATUS_OK;
    for(uint16_t i = 0u; i < s_capacity; ++i) {
        DjiMotorInstance* motor = dji_at(i);
        if(motor != 0 && motor->enabled && dji_fresh(motor) != MOTOR_STATUS_OK) {
            dji_reset_output(motor); motor->enabled = false;
            banks[dji_bank(motor)] = true;
            result = MOTOR_STATUS_TIMEOUT;
        }
    }
    for(uint8_t bank = 0u; bank < 2u; ++bank) if(banks[bank] && dji_send_bank(bank) != MOTOR_STATUS_OK) result = MOTOR_STATUS_PORT_ERROR;
    return result;
}
