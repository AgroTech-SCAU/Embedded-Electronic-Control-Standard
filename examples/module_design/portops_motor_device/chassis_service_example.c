#include "motor_device.h"

extern const MotorDevicePortOps mock_motor_port_ops;
void mock_motor_port_tick(uint32_t dt_ms);

typedef enum {
    CHASSIS_EXAMPLE_OK = 0,
    CHASSIS_EXAMPLE_DEVICE_ERROR,
    CHASSIS_EXAMPLE_TIMEOUT,
} ChassisExampleStatus;

static const uint16_t k_left_motor_id = 1u;

ChassisExampleStatus chassis_example_init(void) {
    MotorDeviceConfig config = {
        .ops = &mock_motor_port_ops,
        .feedback_timeout_ms = 50u,
    };

    if(motor_device_init(&config) != MOTOR_DEVICE_OK) {
        return CHASSIS_EXAMPLE_DEVICE_ERROR;
    }

    return CHASSIS_EXAMPLE_OK;
}

ChassisExampleStatus chassis_example_set_left_speed(float speed_rad_s) {
    if(motor_device_set_speed(k_left_motor_id, speed_rad_s) != MOTOR_DEVICE_OK) {
        return CHASSIS_EXAMPLE_DEVICE_ERROR;
    }

    return CHASSIS_EXAMPLE_OK;
}

ChassisExampleStatus chassis_example_update(void) {
    MotorDeviceFeedback feedback;

    mock_motor_port_tick(10u);

    if(motor_device_update(&feedback) != MOTOR_DEVICE_OK) {
        return CHASSIS_EXAMPLE_DEVICE_ERROR;
    }

    if(motor_device_is_timeout(k_left_motor_id)) {
        motor_device_stop(k_left_motor_id);
        return CHASSIS_EXAMPLE_TIMEOUT;
    }

    return CHASSIS_EXAMPLE_OK;
}
