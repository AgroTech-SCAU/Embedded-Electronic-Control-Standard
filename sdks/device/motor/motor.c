#include "motor.h"

// ! ========================= 变 量 声 明 ========================= ! //

const MotorInterface* motor_instance = 0;

// ! ========================= 私 有 函 数 声 明 ========================= ! //



// ! ========================= 接 口 函 数 实 现 ========================= ! //

MotorStatus motor_set_instance(const MotorInterface* instance) {
    if(instance == 0) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    motor_instance = instance;
    return MOTOR_STATUS_OK;
}

MotorStatus motor_init(const MotorConfig* config) {
    if(motor_instance == 0 || motor_instance->init == 0) {
        return MOTOR_STATUS_NO_INSTANCE;
    }

    return motor_instance->init(config);
}

const char* motor_status_str(MotorStatus status) {
    if(motor_instance != 0 && motor_instance->status_str != 0) {
        return motor_instance->status_str(status);
    }

    switch(status) {
        #define X(name, value) case MOTOR_STATUS_##name: return #name;
        MOTOR_STATUS_TABLE
        #undef X
        default: return "UNKNOWN";
    }
}

const char* motor_mode_str(MotorMode mode) {
    if(motor_instance != 0 && motor_instance->mode_str != 0) {
        return motor_instance->mode_str(mode);
    }

    switch(mode) {
        #define Y(name, value) case MOTOR_MODE_##name: return #name;
        MOTOR_MODE_TABLE
        #undef Y
        default: return "UNKNOWN";
    }
}

MotorStatus motor_enable(uint16_t id) {
    if(motor_instance == 0 || motor_instance->enable == 0) {
        return MOTOR_STATUS_NO_INSTANCE;
    }

    return motor_instance->enable(id);
}

MotorStatus motor_disable(uint16_t id) {
    if(motor_instance == 0 || motor_instance->disable == 0) {
        return MOTOR_STATUS_NO_INSTANCE;
    }

    return motor_instance->disable(id);
}

MotorStatus motor_set_mit(uint16_t id, float pos_rad, float spd_rad_s, float kp, float kd, float torque_a) {
    if(motor_instance == 0 || motor_instance->set_mit == 0) {
        return MOTOR_STATUS_NO_INSTANCE;
    }

    return motor_instance->set_mit(id, pos_rad, spd_rad_s, kp, kd, torque_a);
}

MotorStatus motor_set_pos_spd(uint16_t id, float pos_rad, float spd_rad_s) {
    if(motor_instance == 0 || motor_instance->set_pos_spd == 0) {
        return MOTOR_STATUS_NO_INSTANCE;
    }

    return motor_instance->set_pos_spd(id, pos_rad, spd_rad_s);
}

MotorStatus motor_set_spd(uint16_t id, float spd_rad_s) {
    if(motor_instance == 0 || motor_instance->set_spd == 0) {
        return MOTOR_STATUS_NO_INSTANCE;
    }

    return motor_instance->set_spd(id, spd_rad_s);
}

MotorStatus motor_set_pos_spd_cur(uint16_t id, float pos_rad, float spd_rad_s, float cur_a) {
    if(motor_instance == 0 || motor_instance->set_pos_spd_cur == 0) {
        return MOTOR_STATUS_NO_INSTANCE;
    }

    return motor_instance->set_pos_spd_cur(id, pos_rad, spd_rad_s, cur_a);
}

MotorStatus motor_request_feedback(uint16_t id) {
    if(motor_instance == 0 || motor_instance->request_feedback == 0) {
        return MOTOR_STATUS_NO_INSTANCE;
    }

    return motor_instance->request_feedback(id);
}

MotorStatus motor_get_feedback(uint16_t id, MotorFeedback* feedback) {
    if(motor_instance == 0 || motor_instance->get_feedback == 0) {
        return MOTOR_STATUS_NO_INSTANCE;
    }

    return motor_instance->get_feedback(id, feedback);
}

MotorStatus motor_update(MotorFeedback* feedback) {
    if(motor_instance == 0 || motor_instance->update == 0) {
        return MOTOR_STATUS_NO_INSTANCE;
    }

    return motor_instance->update(feedback);
}

// ! ========================= 私 有 函 数 实 现 ========================= ! //
