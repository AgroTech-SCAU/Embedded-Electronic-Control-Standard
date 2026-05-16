#include "bus_motor.h"

// ! ========================= 变 量 声 明 ========================= ! //

const BusMotorInterface* bus_motor_instance = 0;

// ! ========================= 私 有 函 数 声 明 ========================= ! //



// ! ========================= 接 口 函 数 实 现 ========================= ! //

BusMotorStatus bus_motor_set_instance(const BusMotorInterface* instance) {
    if(instance == 0) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    bus_motor_instance = instance;
    return MOTOR_STATUS_OK;
}

BusMotorStatus bus_motor_init(const BusMotorConfig* config) {
    if(bus_motor_instance == 0 || bus_motor_instance->init == 0) {
        return MOTOR_STATUS_NO_INSTANCE;
    }

    return bus_motor_instance->init(config);
}

const char* bus_motor_status_str(BusMotorStatus status) {
    if(bus_motor_instance != 0 && bus_motor_instance->status_str != 0) {
        return bus_motor_instance->status_str(status);
    }

    switch(status) {
#define X(name, value) case MOTOR_STATUS_##name: return #name;
        MOTOR_STATUS_TABLE
#undef X
        default: return "UNKNOWN";
    }
}

const char* bus_motor_mode_str(BusMotorMode mode) {
    if(bus_motor_instance != 0 && bus_motor_instance->mode_str != 0) {
        return bus_motor_instance->mode_str(mode);
    }

    switch(mode) {
#define Y(name, value) case MOTOR_MODE_##name: return #name;
        MOTOR_MODE_TABLE
#undef Y
        default: return "UNKNOWN";
    }
}

BusMotorStatus bus_motor_enable(uint16_t id) {
    if(bus_motor_instance == 0 || bus_motor_instance->enable == 0) {
        return MOTOR_STATUS_NO_INSTANCE;
    }

    return bus_motor_instance->enable(id);
}

BusMotorStatus bus_motor_disable(uint16_t id) {
    if(bus_motor_instance == 0 || bus_motor_instance->disable == 0) {
        return MOTOR_STATUS_NO_INSTANCE;
    }

    return bus_motor_instance->disable(id);
}

BusMotorStatus bus_motor_set_mit(uint16_t id, float pos_rad, float spd_rad_s, float kp, float kd, float torque_a) {
    if(bus_motor_instance == 0 || bus_motor_instance->set_mit == 0) {
        return MOTOR_STATUS_NO_INSTANCE;
    }

    return bus_motor_instance->set_mit(id, pos_rad, spd_rad_s, kp, kd, torque_a);
}

BusMotorStatus bus_motor_set_pos_spd(uint16_t id, float pos_rad, float spd_rad_s) {
    if(bus_motor_instance == 0 || bus_motor_instance->set_pos_spd == 0) {
        return MOTOR_STATUS_NO_INSTANCE;
    }

    return bus_motor_instance->set_pos_spd(id, pos_rad, spd_rad_s);
}

BusMotorStatus bus_motor_set_spd(uint16_t id, float spd_rad_s) {
    if(bus_motor_instance == 0 || bus_motor_instance->set_spd == 0) {
        return MOTOR_STATUS_NO_INSTANCE;
    }

    return bus_motor_instance->set_spd(id, spd_rad_s);
}

BusMotorStatus bus_motor_set_pos_spd_cur(uint16_t id, float pos_rad, float spd_rad_s, float cur_a) {
    if(bus_motor_instance == 0 || bus_motor_instance->set_pos_spd_cur == 0) {
        return MOTOR_STATUS_NO_INSTANCE;
    }

    return bus_motor_instance->set_pos_spd_cur(id, pos_rad, spd_rad_s, cur_a);
}

BusMotorStatus bus_motor_request_feedback(uint16_t id) {
    if(bus_motor_instance == 0 || bus_motor_instance->request_feedback == 0) {
        return MOTOR_STATUS_NO_INSTANCE;
    }

    return bus_motor_instance->request_feedback(id);
}

BusMotorStatus bus_motor_get_feedback(uint16_t id, BusMotorFeedback* feedback) {
    if(bus_motor_instance == 0 || bus_motor_instance->get_feedback == 0) {
        return MOTOR_STATUS_NO_INSTANCE;
    }

    return bus_motor_instance->get_feedback(id, feedback);
}

BusMotorStatus bus_motor_update(BusMotorFeedback* feedback) {
    if(bus_motor_instance == 0 || bus_motor_instance->update == 0) {
        return MOTOR_STATUS_NO_INSTANCE;
    }

    return bus_motor_instance->update(feedback);
}

// ! ========================= 私 有 函 数 实 现 ========================= ! //
