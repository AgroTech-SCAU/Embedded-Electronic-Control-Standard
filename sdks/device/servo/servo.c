#include "servo.h"

// ! ========================= 变 量 声 明 ========================= ! //

/**
 * @brief 当前绑定的舵机实例
 *
 * 统一入口只负责转发，真正协议实现由 service 层绑定的实例提供
 */
const ServoInterface* servo_instance = 0;

// ! ========================= 私 有 函 数 声 明 ========================= ! //



// ! ========================= 接 口 函 数 实 现 ========================= ! //

ServoStatus servo_set_instance(const ServoInterface* instance) {
    if(instance == 0) {
        return SERVO_STATUS_INVALID_PARAM;
    }

    servo_instance = instance;
    return SERVO_STATUS_OK;
}

ServoStatus servo_init(const ServoConfig* config) {
    if(servo_instance == 0 || servo_instance->init == 0) {
        return SERVO_STATUS_NO_INSTANCE;
    }

    return servo_instance->init(config);
}

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

ServoStatus servo_ping(uint8_t id, uint8_t* out_id) {
    if(servo_instance == 0 || servo_instance->ping == 0) {
        return SERVO_STATUS_NO_INSTANCE;
    }

    return servo_instance->ping(id, out_id);
}

ServoStatus servo_enable_torque(uint8_t id) {
    if(servo_instance == 0 || servo_instance->enable_torque == 0) {
        return SERVO_STATUS_NO_INSTANCE;
    }

    return servo_instance->enable_torque(id);
}

ServoStatus servo_disable_torque(uint8_t id) {
    if(servo_instance == 0 || servo_instance->disable_torque == 0) {
        return SERVO_STATUS_NO_INSTANCE;
    }

    return servo_instance->disable_torque(id);
}

ServoStatus servo_set_position(uint8_t id, int16_t position, uint16_t speed, uint8_t acc) {
    if(servo_instance == 0 || servo_instance->set_position == 0) {
        return SERVO_STATUS_NO_INSTANCE;
    }

    return servo_instance->set_position(id, position, speed, acc);
}

ServoStatus servo_reg_set_position(uint8_t id, int16_t position, uint16_t speed, uint8_t acc) {
    if(servo_instance == 0 || servo_instance->reg_set_position == 0) {
        return SERVO_STATUS_NO_INSTANCE;
    }

    return servo_instance->reg_set_position(id, position, speed, acc);
}

ServoStatus servo_action(uint8_t id) {
    if(servo_instance == 0 || servo_instance->action == 0) {
        return SERVO_STATUS_NO_INSTANCE;
    }

    return servo_instance->action(id);
}

ServoStatus servo_sync_set_position(const uint8_t* ids, uint8_t count, const int16_t* positions, const uint16_t* speeds, const uint8_t* accs) {
    if(servo_instance == 0 || servo_instance->sync_set_position == 0) {
        return SERVO_STATUS_NO_INSTANCE;
    }

    return servo_instance->sync_set_position(ids, count, positions, speeds, accs);
}

ServoStatus servo_set_wheel_mode(uint8_t id) {
    if(servo_instance == 0 || servo_instance->set_wheel_mode == 0) {
        return SERVO_STATUS_NO_INSTANCE;
    }

    return servo_instance->set_wheel_mode(id);
}

ServoStatus servo_set_speed(uint8_t id, int16_t speed, uint8_t acc) {
    if(servo_instance == 0 || servo_instance->set_speed == 0) {
        return SERVO_STATUS_NO_INSTANCE;
    }

    return servo_instance->set_speed(id, speed, acc);
}

ServoStatus servo_read_feedback(uint8_t id, ServoFeedback* feedback) {
    if(servo_instance == 0 || servo_instance->read_feedback == 0) {
        return SERVO_STATUS_NO_INSTANCE;
    }

    return servo_instance->read_feedback(id, feedback);
}

ServoStatus servo_read_position(uint8_t id, int16_t* position) {
    if(servo_instance == 0 || servo_instance->read_position == 0) {
        return SERVO_STATUS_NO_INSTANCE;
    }

    return servo_instance->read_position(id, position);
}

ServoStatus servo_write_u8(uint8_t id, uint8_t addr, uint8_t value) {
    if(servo_instance == 0 || servo_instance->write_u8 == 0) {
        return SERVO_STATUS_NO_INSTANCE;
    }

    return servo_instance->write_u8(id, addr, value);
}

ServoStatus servo_write_u16(uint8_t id, uint8_t addr, uint16_t value) {
    if(servo_instance == 0 || servo_instance->write_u16 == 0) {
        return SERVO_STATUS_NO_INSTANCE;
    }

    return servo_instance->write_u16(id, addr, value);
}

ServoStatus servo_read_u8(uint8_t id, uint8_t addr, uint8_t* value) {
    if(servo_instance == 0 || servo_instance->read_u8 == 0) {
        return SERVO_STATUS_NO_INSTANCE;
    }

    return servo_instance->read_u8(id, addr, value);
}

ServoStatus servo_read_u16(uint8_t id, uint8_t addr, uint16_t* value) {
    if(servo_instance == 0 || servo_instance->read_u16 == 0) {
        return SERVO_STATUS_NO_INSTANCE;
    }

    return servo_instance->read_u16(id, addr, value);
}

// ! ========================= 私 有 函 数 实 现 ========================= ! //

