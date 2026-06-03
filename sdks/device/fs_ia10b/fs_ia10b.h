#ifndef _FS_IA10B_DEVICE_H_
#define _FS_IA10B_DEVICE_H_

#include <stdbool.h>
#include <stdint.h>

#define FS_IA10B_CHANNEL_COUNT 14u

// PortOps: 设备通过函数指针请求平台提供能力
typedef struct {
    void (*register_rx)(void (*callback)(uint8_t byte));
    uint32_t (*now_ms)(void);
} FsIa10bPortOps;

typedef struct {
    uint16_t channel[FS_IA10B_CHANNEL_COUNT];
    uint32_t frame_count;
    uint32_t error_count;
    uint32_t last_update_ms;
    bool valid;
} FsIa10bData;

typedef enum {
    FS_IA10B_OK = 0,
    FS_IA10B_ERR_PARAM,
    FS_IA10B_ERR_PORT
} FsIa10bStatus;

// 模块初始化
FsIa10bStatus fs_ia10b_init(const FsIa10bPortOps* ops);
// 数据获取
bool fs_ia10b_get_data(FsIa10bData* out);
// 在线状态检测
bool fs_ia10b_is_online(uint32_t timeout_ms);
// 串口中断回调函数 (由平台层调用)
void fs_ia10b_rx_callback(uint8_t rx_byte);

#endif