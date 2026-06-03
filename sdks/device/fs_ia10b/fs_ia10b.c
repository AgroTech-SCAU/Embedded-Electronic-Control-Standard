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


//32hal库的调用需解开
// #include "fs_ia10b_device.h"
// #include "stm32h7xx_hal.h"

// extern UART_HandleTypeDef huart5;
// static void (*s_rx_cb)(uint8_t) = NULL;

// static void platform_register_rx(void (*cb)(uint8_t)) { s_rx_cb = cb; }
// static uint32_t platform_now_ms(void) { return HAL_GetTick(); }

// static const FsIa10bPortOps s_port_ops = {
//     .register_rx = platform_register_rx,
//     .now_ms = platform_now_ms
// };

// void fs_ia10b_platform_init(void) {
//     fs_ia10b_init(&s_port_ops);
//     HAL_UART_Receive_IT(&huart5, (uint8_t[]){0}, 1); // 启动中断
// }

// // 供 STM32 回调
// void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
//     if (huart->Instance == UART5) {
//         uint8_t rx;
//         HAL_UART_Receive(&huart5, &rx, 1, 0); // 取出数据
//         if (s_rx_cb) s_rx_cb(rx);
//         HAL_UART_Receive_IT(&huart5, &rx, 1); // 继续接收
//     }
// }