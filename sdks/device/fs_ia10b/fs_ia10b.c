#include "fs_ia10b.h"
#include <string.h>

static FsIa10bData s_data;
static uint8_t s_frame[32];
static uint8_t s_frame_index = 0;
static const FsIa10bPortOps* s_ops = NULL;

FsIa10bStatus fs_ia10b_init(const FsIa10bPortOps* ops) {
    if (!ops || !ops->register_rx || !ops->now_ms) return FS_IA10B_ERR_PORT;
    s_ops = ops;
    memset((void*)&s_data, 0, sizeof(s_data));
    s_ops->register_rx(fs_ia10b_rx_callback);
    return FS_IA10B_OK;
}

void fs_ia10b_rx_callback(uint8_t rx_byte) {
    if (s_frame_index == 0 && rx_byte != 0x20) return;
    if (s_frame_index == 1 && rx_byte != 0x40) { s_frame_index = 0; return; }

    s_frame[s_frame_index++] = rx_byte;
    if (s_frame_index >= 32) {
        uint16_t checksum = 0xFFFF;
        for (int i = 0; i < 30; i++) checksum -= s_frame[i];
        
        if (checksum == (uint16_t)(s_frame[30] | (s_frame[31] << 8))) {
            for (int i = 0; i < 14; i++) 
                s_data.channel[i] = (uint16_t)(s_frame[2+i*2] | (s_frame[3+i*2] << 8));
            s_data.last_update_ms = s_ops->now_ms();
            s_data.valid = true;
            s_data.frame_count++;
        }
        s_frame_index = 0;
    }
}

bool fs_ia10b_get_data(FsIa10bData* out) {
    *out = s_data;
    return out->valid;
}

bool fs_ia10b_is_online(uint32_t timeout_ms) {
    return (s_data.valid && (s_ops->now_ms() - s_data.last_update_ms < timeout_ms));
}
