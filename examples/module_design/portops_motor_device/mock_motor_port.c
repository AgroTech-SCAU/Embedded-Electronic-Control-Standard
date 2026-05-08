#include "motor_device.h"

static uint32_t s_now_ms;
static uint32_t s_last_tx_id;
static uint8_t s_last_tx_data[8];

static int mock_send(uint32_t id, const uint8_t* data, uint8_t len) {
    uint8_t i;

    if(data == 0 || len > sizeof(s_last_tx_data)) {
        return -1;
    }

    s_last_tx_id = id;

    for(i = 0; i < len; i++) {
        s_last_tx_data[i] = data[i];
    }

    return 0;
}

static int mock_read(uint32_t* id, uint8_t* data, uint8_t* len) {
    if(id == 0 || data == 0 || len == 0 || *len < 2u) {
        return -1;
    }

    *id = s_last_tx_id;
    data[0] = s_last_tx_data[0];
    data[1] = s_last_tx_data[1];
    *len = 2u;
    return 0;
}

static uint32_t mock_now_ms(void) {
    return s_now_ms;
}

const MotorDevicePortOps mock_motor_port_ops = {
    .send = mock_send,
    .read = mock_read,
    .now_ms = mock_now_ms,
};

void mock_motor_port_tick(uint32_t dt_ms) {
    s_now_ms += dt_ms;
}
