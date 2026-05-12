#include "motor_device.h"

#include <stddef.h>
#include <string.h>

static const MotorDevicePortOps* s_ops;
static uint32_t s_feedback_timeout_ms;
static MotorDeviceFeedback s_feedback;

static bool port_ready(void) {
    return s_ops != NULL && s_ops->send != NULL && s_ops->read != NULL && s_ops->now_ms != NULL;
}

MotorDeviceStatus motor_device_init(const MotorDeviceConfig* config) {
    if(config == NULL || config->ops == NULL) {
        return MOTOR_DEVICE_INVALID_PARAM;
    }

    s_ops = config->ops;

    if(!port_ready()) {
        s_ops = NULL;
        return MOTOR_DEVICE_PORT_ERROR;
    }

    s_feedback_timeout_ms = config->feedback_timeout_ms;
    memset(&s_feedback, 0, sizeof(s_feedback));
    return MOTOR_DEVICE_OK;
}

MotorDeviceStatus motor_device_set_speed(uint16_t id, float speed_rad_s) {
    uint8_t frame[8] = { 0 };
    int16_t speed_mrad_s;

    if(!port_ready()) {
        return MOTOR_DEVICE_NOT_INITIALIZED;
    }

    speed_mrad_s = (int16_t)(speed_rad_s * 1000.0f);
    frame[0] = (uint8_t)(speed_mrad_s >> 8);
    frame[1] = (uint8_t)(speed_mrad_s & 0xFF);

    if(s_ops->send(0x200u + id, frame, sizeof(frame)) == false) {
        return MOTOR_DEVICE_PORT_ERROR;
    }

    return MOTOR_DEVICE_OK;
}

MotorDeviceStatus motor_device_update(MotorDeviceFeedback* out) {
    uint32_t frame_id;
    uint8_t data[8];
    uint8_t len = sizeof(data);
    int16_t speed_mrad_s;

    if(!port_ready()) {
        return MOTOR_DEVICE_NOT_INITIALIZED;
    }

    if(out == NULL) {
        return MOTOR_DEVICE_INVALID_PARAM;
    }

    if(s_ops->read(&frame_id, data, &len) == false || len < 2u) {
        return MOTOR_DEVICE_PORT_ERROR;
    }

    speed_mrad_s = (int16_t)(((uint16_t)data[0] << 8) | data[1]);

    s_feedback.id = (uint16_t)(frame_id & 0xFFu);
    s_feedback.speed_rad_s = (float)speed_mrad_s / 1000.0f;
    s_feedback.last_update_ms = s_ops->now_ms();
    s_feedback.online = true;

    *out = s_feedback;
    return MOTOR_DEVICE_OK;
}

MotorDeviceStatus motor_device_stop(uint16_t id) {
    return motor_device_set_speed(id, 0.0f);
}

bool motor_device_is_timeout(uint16_t id) {
    uint32_t now;

    if(!port_ready()) {
        return true;
    }

    if(s_feedback.id != id || !s_feedback.online) {
        return true;
    }

    now = s_ops->now_ms();
    return (now - s_feedback.last_update_ms) > s_feedback_timeout_ms;
}
