#include "dm_motor_core.h"

#include <stddef.h>
#include <string.h>

// ! ========================= 变 量 声 明 ========================= ! //

/**
 * @brief 达妙电机型号参数表
 */
static const DmMotorModelSpec s_model_specs[] = {
    { DM_MOTOR_MODEL_DM3507, { 12.566f, 50.0f, 5.0f } },
    { DM_MOTOR_MODEL_DM4310, { 12.5f, 30.0f, 10.0f } },
    { DM_MOTOR_MODEL_DM4310_48V, { 12.5f, 50.0f, 10.0f } },
    { DM_MOTOR_MODEL_DM4340, { 12.5f, 10.0f, 28.0f } },
    { DM_MOTOR_MODEL_DM4340_48V, { 12.5f, 20.0f, 28.0f } },
    { DM_MOTOR_MODEL_DM6006, { 12.5f, 45.0f, 12.0f } },
    { DM_MOTOR_MODEL_DM6248, { 12.566f, 20.0f, 120.0f } },
    { DM_MOTOR_MODEL_DM8006, { 12.5f, 45.0f, 20.0f } },
    { DM_MOTOR_MODEL_DM8009, { 12.5f, 45.0f, 54.0f } },
    { DM_MOTOR_MODEL_DM10010L, { 12.5f, 25.0f, 200.0f } },
    { DM_MOTOR_MODEL_DM10010, { 12.5f, 20.0f, 200.0f } },
    { DM_MOTOR_MODEL_DMH3510, { 12.5f, 280.0f, 1.0f } },
    { DM_MOTOR_MODEL_DMH6215, { 12.5f, 45.0f, 10.0f } },
    { DM_MOTOR_MODEL_DMS3519, { 12.5f, 2000.0f, 2.0f } },
    { DM_MOTOR_MODEL_DMG6220, { 12.5f, 45.0f, 10.0f } },
};

// ! ========================= 接 口 函 数 实 现 ========================= ! //

/**
 * @brief 获取指定达妙电机型号参数
 */
const DmMotorModelSpec* dm_motor_model_get(DmMotorModel model) {
    size_t i;

    for(i = 0u; i < sizeof(s_model_specs) / sizeof(s_model_specs[0]); ++i) {
        if(s_model_specs[i].model == model) {
            return &s_model_specs[i];
        }
    }

    return 0;
}

/**
 * @brief 初始化达妙电机注册表
 */
BusMotorStatus dm_motor_registry_init(DmMotorRegistry* registry, DmMotorInstance* instances, uint16_t capacity) {
    if(registry == 0 || instances == 0 || capacity == 0u) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    memset(instances, 0, sizeof(instances[0]) * capacity);
    registry->instances = instances;
    registry->capacity = capacity;
    registry->count = 0u;
    return MOTOR_STATUS_OK;
}

/**
 * @brief 注册达妙电机实例
 */
BusMotorStatus dm_motor_registry_register(DmMotorRegistry* registry, BusMotorId motor_id, const DmMotorConfig* config,
                                          uint16_t* instance_out, DmMotorInstance** instance_ptr_out) {
    const DmMotorModelSpec* spec;
    DmMotorInstance* instance_ptr;
    uint16_t instance;

    if(registry == 0 || registry->instances == 0 || config == 0 || config->can_id == 0u || config->can_id > 0x7FFu || config->master_id > 0x7FFu ||
       (config->master_id == 0u && config->can_id > 0x0Fu)) {
        return MOTOR_STATUS_INVALID_PARAM;
    }
    if(config->firmware.major != DM_MOTOR_FIRMWARE_V3 && config->firmware.major != DM_MOTOR_FIRMWARE_V4) {
        return MOTOR_STATUS_INVALID_PARAM;
    }
    if(dm_motor_registry_find_by_motor_id(registry, motor_id) != 0 ||
       dm_motor_registry_find_by_can_id(registry, config->can_id) != 0) {
        return MOTOR_STATUS_ALREADY_BOUND;
    }
    if(config->master_id != 0u && dm_motor_registry_find_by_master_id(registry, config->master_id) != 0) {
        return MOTOR_STATUS_ALREADY_BOUND;
    }
    if(registry->count >= registry->capacity) {
        return MOTOR_STATUS_NO_RESOURCE;
    }

    spec = dm_motor_model_get(config->model);
    if(spec == 0) {
        return MOTOR_STATUS_UNSUPPORTED;
    }

    instance = registry->count;
    instance_ptr = &registry->instances[instance];
    memset(instance_ptr, 0, sizeof(*instance_ptr));
    instance_ptr->registered = true;
    instance_ptr->motor_id = motor_id;
    instance_ptr->can_id = config->can_id;
    instance_ptr->master_id = config->master_id;
    instance_ptr->model = config->model;
    instance_ptr->firmware = config->firmware;
    instance_ptr->limits = spec->limits;
    instance_ptr->mode = config->default_mode;
    instance_ptr->kp = DM_MOTOR_DEFAULT_KP;
    instance_ptr->kd = DM_MOTOR_DEFAULT_KD;
    instance_ptr->torque = DM_MOTOR_DEFAULT_TORQUE;
    instance_ptr->feedback.id = motor_id;

    registry->count++;
    if(instance_out != 0) {
        *instance_out = instance;
    }
    if(instance_ptr_out != 0) {
        *instance_ptr_out = instance_ptr;
    }

    return MOTOR_STATUS_OK;
}

/**
 * @brief 注销达妙电机实例
 */
BusMotorStatus dm_motor_registry_unregister(DmMotorRegistry* registry, uint16_t instance) {
    if(registry == 0 || instance >= registry->count || registry->instances[instance].registered == false) {
        return MOTOR_STATUS_NOT_FOUND;
    }
    if(instance + 1u != registry->count) {
        return MOTOR_STATUS_UNSUPPORTED;
    }

    memset(&registry->instances[instance], 0, sizeof(registry->instances[instance]));
    registry->count--;
    return MOTOR_STATUS_OK;
}

/**
 * @brief 按厂家实例编号获取达妙电机实例
 */
DmMotorInstance* dm_motor_registry_at(DmMotorRegistry* registry, uint16_t instance) {
    if(registry == 0 || instance >= registry->count || registry->instances[instance].registered == false) {
        return 0;
    }

    return &registry->instances[instance];
}

/**
 * @brief 按厂家实例编号获取只读达妙电机实例
 */
const DmMotorInstance* dm_motor_registry_at_const(const DmMotorRegistry* registry, uint16_t instance) {
    if(registry == 0 || instance >= registry->count || registry->instances[instance].registered == false) {
        return 0;
    }

    return &registry->instances[instance];
}

/**
 * @brief 按业务逻辑电机 ID 查找达妙电机实例
 */
DmMotorInstance* dm_motor_registry_find_by_motor_id(DmMotorRegistry* registry, BusMotorId motor_id) {
    uint16_t i;

    if(registry == 0) {
        return 0;
    }

    for(i = 0u; i < registry->count; ++i) {
        if(registry->instances[i].registered && registry->instances[i].motor_id == motor_id) {
            return &registry->instances[i];
        }
    }

    return 0;
}

/**
 * @brief 按 CAN ID 查找达妙电机实例
 */
DmMotorInstance* dm_motor_registry_find_by_can_id(DmMotorRegistry* registry, uint16_t can_id) {
    uint16_t i;

    if(registry == 0) {
        return 0;
    }

    for(i = 0u; i < registry->count; ++i) {
        if(registry->instances[i].registered && registry->instances[i].can_id == can_id) {
            return &registry->instances[i];
        }
    }

    return 0;
}

/**
 * @brief 按反馈帧路由规则查找达妙电机实例
 */
DmMotorInstance* dm_motor_registry_find_by_feedback(DmMotorRegistry* registry, uint16_t master_id, uint16_t can_id) {
    if(registry == 0) {
        return 0;
    }
    if(master_id != 0u) {
        return dm_motor_registry_find_by_master_id(registry, master_id);
    }

    return dm_motor_registry_find_by_can_id(registry, can_id);
}

/**
 * @brief 按 Master ID 查找达妙电机实例
 */
DmMotorInstance* dm_motor_registry_find_by_master_id(DmMotorRegistry* registry, uint16_t master_id) {
    uint16_t i;

    if(registry == 0) {
        return 0;
    }

    for(i = 0u; i < registry->count; ++i) {
        if(registry->instances[i].registered && registry->instances[i].master_id == master_id) {
            return &registry->instances[i];
        }
    }

    return 0;
}

/**
 * @brief 将浮点值映射为无符号整数
 */
uint16_t dm_motor_codec_f32_to_uint(float value, float min, float max, uint8_t bits) {
    float normalized;
    uint32_t max_bits_value;

    if(bits == 0u || bits > 16u || max <= min) {
        return 0u;
    }
    if(value < min) {
        value = min;
    }
    if(value > max) {
        value = max;
    }

    normalized = (value - min) / (max - min);
    max_bits_value = (1UL << bits) - 1UL;
    return (uint16_t)(normalized * (float)max_bits_value);
}

/**
 * @brief 将无符号整数映射为浮点值
 */
float dm_motor_codec_uint_to_f32(uint16_t value, float min, float max, uint8_t bits) {
    uint32_t max_bits_value;

    if(bits == 0u || bits > 16u || max <= min) {
        return 0.0f;
    }

    max_bits_value = (1UL << bits) - 1UL;
    return ((float)value) * (max - min) / (float)max_bits_value + min;
}

/**
 * @brief 按小端序编码 float
 */
void dm_motor_codec_pack_f32_le(uint8_t out[4], float value) {
    uint32_t raw = 0u;

    if(out == 0) {
        return;
    }

    memcpy(&raw, &value, sizeof(raw));
    out[0] = (uint8_t)(raw & 0xFFu);
    out[1] = (uint8_t)((raw >> 8) & 0xFFu);
    out[2] = (uint8_t)((raw >> 16) & 0xFFu);
    out[3] = (uint8_t)((raw >> 24) & 0xFFu);
}

/**
 * @brief 按小端序解析 float
 */
float dm_motor_codec_unpack_f32_le(const uint8_t in[4]) {
    uint32_t raw;
    float value = 0.0f;

    if(in == 0) {
        return 0.0f;
    }

    raw = (uint32_t)in[0] |
          ((uint32_t)in[1] << 8) |
          ((uint32_t)in[2] << 16) |
          ((uint32_t)in[3] << 24);
    memcpy(&value, &raw, sizeof(value));
    return value;
}

/**
 * @brief 编码 MIT 控制指令
 */
void dm_motor_codec_encode_mit(const DmMotorLimitParam* limits, float position, float velocity,
                               float kp, float kd, float torque, uint8_t out[8]) {
    uint16_t pos_bits;
    uint16_t vel_bits;
    uint16_t kp_bits;
    uint16_t kd_bits;
    uint16_t tor_bits;

    if(limits == 0 || out == 0) {
        return;
    }

    pos_bits = dm_motor_codec_f32_to_uint(position, -limits->q_max, limits->q_max, 16u);
    vel_bits = dm_motor_codec_f32_to_uint(velocity, -limits->dq_max, limits->dq_max, 12u);
    kp_bits = dm_motor_codec_f32_to_uint(kp, 0.0f, DM_MOTOR_MIT_KP_MAX, 12u);
    kd_bits = dm_motor_codec_f32_to_uint(kd, 0.0f, DM_MOTOR_MIT_KD_MAX, 12u);
    tor_bits = dm_motor_codec_f32_to_uint(torque, -limits->tau_max, limits->tau_max, 12u);

    out[0] = (uint8_t)(pos_bits >> 8);
    out[1] = (uint8_t)(pos_bits & 0xFFu);
    out[2] = (uint8_t)(vel_bits >> 4);
    out[3] = (uint8_t)(((vel_bits & 0x0Fu) << 4) | (kp_bits >> 8));
    out[4] = (uint8_t)(kp_bits & 0xFFu);
    out[5] = (uint8_t)(kd_bits >> 4);
    out[6] = (uint8_t)(((kd_bits & 0x0Fu) << 4) | (tor_bits >> 8));
    out[7] = (uint8_t)(tor_bits & 0xFFu);
}

/**
 * @brief 编码 POS_VEL 控制指令
 */
void dm_motor_codec_encode_pos_vel(float position, float velocity, uint8_t out[8]) {
    if(out == 0) {
        return;
    }

    dm_motor_codec_pack_f32_le(&out[0], position);
    dm_motor_codec_pack_f32_le(&out[4], velocity);
}

/**
 * @brief 编码 VEL 控制指令
 */
void dm_motor_codec_encode_vel(float velocity, uint8_t out[4]) {
    if(out == 0) {
        return;
    }

    dm_motor_codec_pack_f32_le(out, velocity);
}

/**
 * @brief 编码 POS_FORCE 控制指令
 */
void dm_motor_codec_encode_pos_force(float position, uint16_t velocity, uint16_t current, uint8_t out[8]) {
    if(out == 0) {
        return;
    }

    dm_motor_codec_pack_f32_le(&out[0], position);
    out[4] = (uint8_t)(velocity & 0xFFu);
    out[5] = (uint8_t)(velocity >> 8);
    out[6] = (uint8_t)(current & 0xFFu);
    out[7] = (uint8_t)(current >> 8);
}

/**
 * @brief 解析 MIT 状态反馈
 */
void dm_motor_codec_decode_feedback(const DmMotorLimitParam* limits, const uint8_t data[8],
                                    float* position, float* velocity, float* torque) {
    uint16_t pos_bits;
    uint16_t vel_bits;
    uint16_t tor_bits;

    if(limits == 0 || data == 0) {
        return;
    }

    pos_bits = (uint16_t)(((uint16_t)data[1] << 8) | data[2]);
    vel_bits = (uint16_t)(((uint16_t)data[3] << 4) | (((uint16_t)data[4] & 0xF0u) >> 4));
    tor_bits = (uint16_t)((((uint16_t)data[4] & 0x0Fu) << 8) | data[5]);

    if(position != 0) {
        *position = dm_motor_codec_uint_to_f32(pos_bits, -limits->q_max, limits->q_max, 16u);
    }
    if(velocity != 0) {
        *velocity = dm_motor_codec_uint_to_f32(vel_bits, -limits->dq_max, limits->dq_max, 12u);
    }
    if(torque != 0) {
        *torque = dm_motor_codec_uint_to_f32(tor_bits, -limits->tau_max, limits->tau_max, 12u);
    }
}
