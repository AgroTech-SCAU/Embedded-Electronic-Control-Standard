#ifndef EXAMPLE_MOTOR_DEVICE_H
#define EXAMPLE_MOTOR_DEVICE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    MOTOR_DEVICE_OK = 0,
    MOTOR_DEVICE_INVALID_PARAM,
    MOTOR_DEVICE_PORT_ERROR,
    MOTOR_DEVICE_TIMEOUT,
    MOTOR_DEVICE_NOT_INITIALIZED,
} MotorDeviceStatus;

typedef struct {
    uint16_t id;
    float speed_rad_s;
    uint32_t last_update_ms;
    bool online;
} MotorDeviceFeedback;

typedef struct {
    bool(*send)(uint32_t id, const uint8_t* data, uint8_t len);
    bool(*read)(uint32_t* id, uint8_t* data, uint8_t* len);
    uint32_t(*now_ms)(void);
} MotorDevicePortOps;

typedef struct {
    const MotorDevicePortOps* ops;
    uint32_t feedback_timeout_ms;
} MotorDeviceConfig;

MotorDeviceStatus motor_device_init(const MotorDeviceConfig* config);
MotorDeviceStatus motor_device_set_speed(uint16_t id, float speed_rad_s);
MotorDeviceStatus motor_device_update(MotorDeviceFeedback* out);
MotorDeviceStatus motor_device_stop(uint16_t id);
bool motor_device_is_timeout(uint16_t id);

#endif
