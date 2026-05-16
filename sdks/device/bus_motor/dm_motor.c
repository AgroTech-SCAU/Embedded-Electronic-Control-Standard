#include "dm_motor.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// ! ========================= 变 量 声 明 ========================= ! //

static const BusMotorPortOps* s_ops;
static uint32_t s_feedback_timeout_ms;
static bool s_initialized;

// ! ========================= 私 有 函 数 声 明 ========================= ! //

static BusMotorStatus dm_motor_init(const BusMotorConfig* config);
static const char* dm_motor_status_str(BusMotorStatus status);
static const char* dm_motor_mode_str(BusMotorMode mode);
static BusMotorStatus dm_motor_enable(uint16_t id);
static BusMotorStatus dm_motor_disable(uint16_t id);
static BusMotorStatus dm_motor_set_mit(uint16_t id, float pos_rad, float spd_rad_s, float kp, float kd, float torque_a);
static BusMotorStatus dm_motor_set_pos_spd(uint16_t id, float pos_rad, float spd_rad_s);
static BusMotorStatus dm_motor_set_spd(uint16_t id, float spd_rad_s);
static BusMotorStatus dm_motor_set_pos_spd_cur(uint16_t id, float pos_rad, float spd_rad_s, float cur_a);
static BusMotorStatus dm_motor_request_feedback(uint16_t id);
static BusMotorStatus dm_motor_get_feedback(uint16_t id, BusMotorFeedback* feedback);
static BusMotorStatus dm_motor_update(BusMotorFeedback* feedback);

static BusMotorStatus dm_motor_send(uint32_t id, const uint8_t data[DM_MOTOR_CMD_LEN]);
static BusMotorStatus dm_motor_read(uint32_t* id, uint8_t data[DM_MOTOR_CMD_LEN]);
static void dm_motor_write_register(uint16_t id, uint8_t rid, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3);
static void dm_motor_switch_mode(uint16_t id, BusMotorMode mode);
static uint16_t dm_motor_f32_to_u16(float val, float min, float max, uint8_t bits);
static float dm_motor_u16_to_f32(uint16_t val, float min, float max, uint8_t bits);
static BusMotorStatus dm_motor_decode_feedback(uint32_t frame_id, const uint8_t data[DM_MOTOR_CMD_LEN], BusMotorFeedback* feedback);

// ! ========================= 接 口 函 数 实 现 ========================= ! //

const BusMotorInterface dm_motor_instance = {
    .init = dm_motor_init,
    .status_str = dm_motor_status_str,
    .mode_str = dm_motor_mode_str,
    .enable = dm_motor_enable,
    .disable = dm_motor_disable,
    .set_mit = dm_motor_set_mit,
    .set_pos_spd = dm_motor_set_pos_spd,
    .set_spd = dm_motor_set_spd,
    .set_pos_spd_cur = dm_motor_set_pos_spd_cur,
    .request_feedback = dm_motor_request_feedback,
    .get_feedback = dm_motor_get_feedback,
    .update = dm_motor_update,
};

static BusMotorStatus dm_motor_init(const BusMotorConfig* config) {
    if(config == 0 || config->ops == 0) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    if(config->ops->send == 0 || config->ops->read == 0 || config->ops->now_ms == 0) {
        return MOTOR_STATUS_PORT_ERROR;
    }

    s_ops = config->ops;
    s_feedback_timeout_ms = config->feedback_timeout_ms;
    s_initialized = true;

    return MOTOR_STATUS_OK;
}

static const char* dm_motor_status_str(BusMotorStatus status) {
    switch(status) {
#define X(name, value) case MOTOR_STATUS_##name: return #name;
        MOTOR_STATUS_TABLE
#undef X
        default: return "UNKNOWN";
    }
}

static const char* dm_motor_mode_str(BusMotorMode mode) {
    switch(mode) {
#define Y(name, value) case MOTOR_MODE_##name: return #name;
        MOTOR_MODE_TABLE
#undef Y
        default: return "UNKNOWN";
    }
}

static BusMotorStatus dm_motor_enable(uint16_t id) {
    uint8_t data[DM_MOTOR_CMD_LEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC };
    return dm_motor_send(id, data);
}

static BusMotorStatus dm_motor_disable(uint16_t id) {
    uint8_t data[DM_MOTOR_CMD_LEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD };
    return dm_motor_send(id, data);
}

static BusMotorStatus dm_motor_set_mit(uint16_t id, float pos_rad, float spd_rad_s, float kp, float kd, float torque_a) {
    uint16_t pos_bits;
    uint16_t spd_bits;
    uint16_t kp_bits;
    uint16_t kd_bits;
    uint16_t torque_bits;
    uint8_t data[DM_MOTOR_CMD_LEN];

    dm_motor_switch_mode(id, MOTOR_MODE_MIT);

    pos_bits = dm_motor_f32_to_u16(pos_rad, -DM_MOTOR_POS_LIMIT, DM_MOTOR_POS_LIMIT, 16);
    spd_bits = dm_motor_f32_to_u16(spd_rad_s, -DM_MOTOR_SPD_LIMIT, DM_MOTOR_SPD_LIMIT, 12);
    kp_bits = dm_motor_f32_to_u16(kp, 0.0f, DM_MOTOR_KP_LIMIT, 12);
    kd_bits = dm_motor_f32_to_u16(kd, 0.0f, DM_MOTOR_KD_LIMIT, 12);
    torque_bits = dm_motor_f32_to_u16(torque_a, -DM_MOTOR_TORQUE_LIMIT, DM_MOTOR_TORQUE_LIMIT, 12);

    data[0] = (uint8_t)(pos_bits >> 8);
    data[1] = (uint8_t)(pos_bits & 0xFFu);
    data[2] = (uint8_t)(spd_bits >> 4);
    data[3] = (uint8_t)(((spd_bits & 0x0Fu) << 4) | (kp_bits >> 8));
    data[4] = (uint8_t)(kp_bits & 0xFFu);
    data[5] = (uint8_t)(kd_bits >> 4);
    data[6] = (uint8_t)(((kd_bits & 0x0Fu) << 4) | (torque_bits >> 8));
    data[7] = (uint8_t)(torque_bits & 0xFFu);

    return dm_motor_send(id, data);
}

static BusMotorStatus dm_motor_set_pos_spd(uint16_t id, float pos_rad, float spd_rad_s) {
    uint32_t target_id = (uint32_t)id + 0x100u;
    uint8_t data[DM_MOTOR_CMD_LEN];

    dm_motor_switch_mode(id, MOTOR_MODE_POS_SPD);

    memcpy(&data[0], &pos_rad, sizeof(float));
    memcpy(&data[4], &spd_rad_s, sizeof(float));

    return dm_motor_send(target_id, data);
}

static BusMotorStatus dm_motor_set_spd(uint16_t id, float spd_rad_s) {
    uint32_t target_id = (uint32_t)id + 0x200u;
    uint8_t data[DM_MOTOR_CMD_LEN] = { 0 };

    dm_motor_switch_mode(id, MOTOR_MODE_SPD);
    memcpy(&data[0], &spd_rad_s, sizeof(float));

    return dm_motor_send(target_id, data);
}

static BusMotorStatus dm_motor_set_pos_spd_cur(uint16_t id, float pos_rad, float spd_rad_s, float cur_a) {
    uint32_t target_id = (uint32_t)id + 0x300u;
    uint16_t spd_scaled = (uint16_t)(spd_rad_s * 100.0f);
    uint16_t cur_scaled = (uint16_t)(cur_a * 10000.0f);
    uint8_t data[DM_MOTOR_CMD_LEN];

    dm_motor_switch_mode(id, MOTOR_MODE_POS_SPD_CUR);

    memcpy(&data[0], &pos_rad, sizeof(float));
    data[4] = (uint8_t)(spd_scaled & 0xFFu);
    data[5] = (uint8_t)(spd_scaled >> 8);
    data[6] = (uint8_t)(cur_scaled & 0xFFu);
    data[7] = (uint8_t)(cur_scaled >> 8);

    return dm_motor_send(target_id, data);
}

static BusMotorStatus dm_motor_request_feedback(uint16_t id) {
    uint8_t can_id_l = (uint8_t)(id & 0xFFu);
    uint8_t can_id_h = (uint8_t)((id >> 8) & 0x07u);
    uint8_t data[DM_MOTOR_CMD_LEN] = { can_id_l, can_id_h, 0xCC, 0x00, 0, 0, 0, 0 };

    return dm_motor_send(0x7FFu, data);
}

static BusMotorStatus dm_motor_get_feedback(uint16_t id, BusMotorFeedback* feedback) {
    uint32_t start_ms;
    uint8_t expected_id;

    if(feedback == 0 || s_ops == 0 || s_ops->now_ms == 0) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    if(dm_motor_request_feedback(id) != MOTOR_STATUS_OK) {
        return MOTOR_STATUS_PORT_ERROR;
    }

    start_ms = s_ops->now_ms();
    expected_id = (uint8_t)(id & 0x0Fu);

    while((s_ops->now_ms() - start_ms) <= s_feedback_timeout_ms) {
        BusMotorStatus status = dm_motor_update(feedback);
        if(status == MOTOR_STATUS_OK) {
            return ((uint8_t)(feedback->id & 0x0Fu) == expected_id) ? MOTOR_STATUS_OK : MOTOR_STATUS_ID_MISMATCH;
        }
    }

    return MOTOR_STATUS_TIMEOUT;
}

static BusMotorStatus dm_motor_update(BusMotorFeedback* feedback) {
    uint32_t frame_id = 0;
    uint8_t data[DM_MOTOR_CMD_LEN] = { 0 };
    BusMotorStatus status;

    if(feedback == 0) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    status = dm_motor_read(&frame_id, data);
    if(status != MOTOR_STATUS_OK) {
        return status;
    }

    return dm_motor_decode_feedback(frame_id, data, feedback);
}

static BusMotorStatus dm_motor_send(uint32_t id, const uint8_t data[DM_MOTOR_CMD_LEN]) {
    if(s_initialized == false) {
        return MOTOR_STATUS_NOT_INITIALIZE;
    }

    if(s_ops == 0 || s_ops->send == 0 || data == 0) {
        return MOTOR_STATUS_PORT_ERROR;
    }

    if(s_ops->send(id, data, DM_MOTOR_CMD_LEN) == false) {
        return MOTOR_STATUS_PORT_ERROR;
    }

    if(s_ops->delay_ms != 0) {
        s_ops->delay_ms(1u);
    }

    return MOTOR_STATUS_OK;
}

static BusMotorStatus dm_motor_read(uint32_t* id, uint8_t data[DM_MOTOR_CMD_LEN]) {
    uint8_t len = DM_MOTOR_CMD_LEN;

    if(s_initialized == false) {
        return MOTOR_STATUS_NOT_INITIALIZE;
    }

    if(s_ops == 0 || s_ops->read == 0 || id == 0 || data == 0) {
        return MOTOR_STATUS_PORT_ERROR;
    }

    if(s_ops->read(id, data, &len) == false) {
        return MOTOR_STATUS_TIMEOUT;
    }

    if(len < DM_MOTOR_CMD_LEN) {
        memset(&data[len], 0, DM_MOTOR_CMD_LEN - len);
    }

    return MOTOR_STATUS_OK;
}

static void dm_motor_write_register(uint16_t id, uint8_t rid, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3) {
    uint8_t can_id_l = (uint8_t)(id & 0xFFu);
    uint8_t can_id_h = (uint8_t)((id >> 8) & 0x07u);
    uint8_t data[DM_MOTOR_CMD_LEN] = { can_id_l, can_id_h, 0x55, rid, d0, d1, d2, d3 };

    (void)dm_motor_send(0x7FFu, data);
}

static void dm_motor_switch_mode(uint16_t id, BusMotorMode mode) {
    dm_motor_write_register(id, 10u, (uint8_t)mode, 0, 0, 0);
}

static uint16_t dm_motor_f32_to_u16(float val, float min, float max, uint8_t bits) {
    float span;
    float normalized;
    uint32_t max_bits_val;

    if(bits == 0 || max <= min) {
        return 0;
    }

    if(val < min) {
        val = min;
    }
    if(val > max) {
        val = max;
    }

    span = max - min;
    normalized = (val - min) / span;
    max_bits_val = (1UL << bits) - 1UL;

    return (uint16_t)(normalized * (float)max_bits_val);
}

static float dm_motor_u16_to_f32(uint16_t val, float min, float max, uint8_t bits) {
    float span;
    uint32_t max_bits_val;

    if(bits == 0 || max <= min) {
        return 0.0f;
    }

    span = max - min;
    max_bits_val = (1UL << bits) - 1UL;

    return ((float)val) * span / (float)max_bits_val + min;
}

static BusMotorStatus dm_motor_decode_feedback(uint32_t frame_id, const uint8_t data[DM_MOTOR_CMD_LEN], BusMotorFeedback* feedback) {
    uint16_t pos_bits;
    uint16_t spd_bits;
    uint16_t torque_bits;

    if(data == 0 || feedback == 0) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    (void)frame_id;

    feedback->id = (uint16_t)(data[0] & 0x0Fu);
    feedback->err_code = (uint8_t)(data[0] >> 4);

    pos_bits = (uint16_t)(((uint16_t)data[1] << 8) | data[2]);
    spd_bits = (uint16_t)(((uint16_t)data[3] << 4) | ((uint16_t)(data[4] & 0xF0u) >> 4));
    torque_bits = (uint16_t)((((uint16_t)data[4] & 0x0Fu) << 8) | data[5]);

    feedback->pos_rad = dm_motor_u16_to_f32(pos_bits, -DM_MOTOR_POS_LIMIT, DM_MOTOR_POS_LIMIT, 16);
    feedback->spd_rad_s = dm_motor_u16_to_f32(spd_bits, -DM_MOTOR_SPD_LIMIT, DM_MOTOR_SPD_LIMIT, 12);
    feedback->torque_a = dm_motor_u16_to_f32(torque_bits, -DM_MOTOR_TORQUE_LIMIT, DM_MOTOR_TORQUE_LIMIT, 12);

    return MOTOR_STATUS_OK;
}

// ! ========================= 私 有 函 数 实 现 ========================= ! //
