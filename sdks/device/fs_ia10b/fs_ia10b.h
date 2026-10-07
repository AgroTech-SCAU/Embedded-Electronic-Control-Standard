#ifndef FS_IA10B_H
#define FS_IA10B_H
#include <stdbool.h>
#include <stdint.h>
#define FS_IA10B_CHANNEL_COUNT 14u
#define FS_IA10B_IBUS_FRAME_LEN 32u
typedef enum { FS_IA10B_STATUS_OK=0, FS_IA10B_STATUS_INVALID_PARAM,
    FS_IA10B_STATUS_NOT_INITIALIZED, FS_IA10B_STATUS_PORT_ERROR } FsIa10bStatus;
/* context identifies the UART and clock for each instance
 * critical_enter/exit must serialize callbacks, maintain and snapshots
 * start_receive arms one byte without invoking a completion inline
 * abort_receive synchronously cancels the previous buffer before returning */
typedef struct {
    bool (*start_receive)(void *context, uint8_t *data, uint16_t len);
    bool (*abort_receive)(void *context);
    uint32_t (*now_ms)(void *context);
    uint32_t (*critical_enter)(void *context);
    void (*critical_exit)(void *context, uint32_t state);
} FsIa10bPortOps;
typedef struct {
    const FsIa10bPortOps *ops;
    void *context;
    uint32_t frame_timeout_ms; /* 0 selects 100 ms */
    uint32_t restart_interval_ms; /* 0 selects 200 ms */
} FsIa10bConfig;
typedef struct {
    uint16_t channel[FS_IA10B_CHANNEL_COUNT];
    uint32_t frame_count, error_count, last_update_ms;
    bool valid; /* historical sample only, use is_online for freshness */
} FsIa10bData;
typedef struct {
    FsIa10bConfig config;
    FsIa10bData data;
    uint8_t rx_byte, frame[FS_IA10B_IBUS_FRAME_LEN], frame_index;
    uint32_t last_restart_ms;
    bool initialized, restart_pending;
} FsIa10b;
/* Initialize zeroed storage once while callbacks are disabled
 * A PORT_ERROR leaves an initialized instance recoverable by maintain */
FsIa10bStatus fs_ia10b_init(FsIa10b *self, const FsIa10bConfig *config);
FsIa10bStatus fs_ia10b_maintain(FsIa10b *self);
FsIa10bStatus fs_ia10b_on_rx_complete(FsIa10b *self);
FsIa10bStatus fs_ia10b_on_rx_error(FsIa10b *self);
FsIa10bStatus fs_ia10b_get_data(const FsIa10b *self, FsIa10bData *out);
bool fs_ia10b_is_online(const FsIa10b *self, uint32_t timeout_ms);
#endif
