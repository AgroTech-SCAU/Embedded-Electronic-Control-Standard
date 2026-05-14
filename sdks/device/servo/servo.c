#include "servo.h"

// ! ========================= 变 量 声 明 ========================= ! //

/**
 * @brief 当前绑定的具体舵机实例
 */
const ServoInterface* servo_instance = 0;

// ! ========================= 接 口 函 数 实 现 ========================= ! //

/**
 * @brief 绑定具体舵机实例
 */
ServoStatus servo_set_instance(const ServoInterface* instance) {
    if(instance == 0) {
        return SERVO_STATUS_INVALID_PARAM;
    }

    servo_instance = instance;
    return SERVO_STATUS_OK;
}

/**
 * @brief 初始化当前绑定的舵机实例
 */
ServoStatus servo_init(const ServoConfig* config) {
    if(servo_instance == 0 || servo_instance->init == 0) {
        return SERVO_STATUS_NO_INSTANCE;
    }

    return servo_instance->init(config);
}

/**
 * @brief 将状态码转换为常量字符串
 */
const char* servo_status_str(ServoStatus status) {
    if(servo_instance != 0 && servo_instance->status_str != 0) {
        return servo_instance->status_str(status);
    }

    switch(status) {
#define X(name, value) case SERVO_STATUS_##name: return #name;
        SERVO_STATUS_TABLE
#undef X
        default: return "UNKNOWN";
    }
}

/**
 * @brief 让舵机一直以指定速度旋转
 */
ServoStatus servo_set_speed(uint8_t id, float speed) {
    if(servo_instance == 0 || servo_instance->set_speed == 0) {
        return SERVO_STATUS_NO_INSTANCE;
    }

    return servo_instance->set_speed(id, speed);
}

/**
 * @brief 让舵机以指定速度旋转到指定位置
 */
ServoStatus servo_set_pos_spd(uint8_t id, float position, float velocity) {
    if(servo_instance == 0 || servo_instance->set_pos_spd == 0) {
        return SERVO_STATUS_NO_INSTANCE;
    }

    return servo_instance->set_pos_spd(id, position, velocity);
}

/**
 * @brief 让舵机以指定速度和扭矩旋转到指定位置并保持扭矩
 */
ServoStatus servo_set_pos_spd_tor(uint8_t id, float position, float velocity, float torque) {
    if(servo_instance == 0 || servo_instance->set_pos_spd_tor == 0) {
        return SERVO_STATUS_NO_INSTANCE;
    }

    return servo_instance->set_pos_spd_tor(id, position, velocity, torque);
}

/**
 * @brief 从本地反馈缓存获取最近解析的位置
 */
float servo_get_position(uint8_t id) {
    if(servo_instance == 0 || servo_instance->get_position == 0) {
        return 0.0f;
    }

    return servo_instance->get_position(id);
}

/**
 * @brief 从本地反馈缓存获取最近解析的速度
 */
float servo_get_speed(uint8_t id) {
    if(servo_instance == 0 || servo_instance->get_speed == 0) {
        return 0.0f;
    }

    return servo_instance->get_speed(id);
}

/**
 * @brief 从本地反馈缓存获取最近解析的扭矩
 */
float servo_get_torque(uint8_t id) {
    if(servo_instance == 0 || servo_instance->get_torque == 0) {
        return 0.0f;
    }

    return servo_instance->get_torque(id);
}

/**
 * @brief 主动请求并解析反馈, 然后更新本地反馈缓存
 */
ServoStatus servo_update_feedback(uint8_t id, ServoFeedback* feedback) {
    if(servo_instance == 0 || servo_instance->update_feedback == 0) {
        return SERVO_STATUS_NO_INSTANCE;
    }

    return servo_instance->update_feedback(id, feedback);
}
