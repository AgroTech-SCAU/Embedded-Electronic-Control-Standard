#include "assemble.h"

#include "log.h"
#include "stm32_hal_uart.h"

// ! ========================= 变 量 声 明 ========================= ! //

static const LogPortOps log_ops = {
    .write = uart1_write_it,
};

// ! ========================= 接 口 函 数 实 现 ========================= ! //

SystemStatus assemble_log(void) {
    /* 配置放在栈上仅用于初始化；log_init 会复制配置而不保留该指针 */
    LogConfig log_config = {
        .ops = &log_ops,
        .level = LOG_LEVEL_INFO,
        .enable_color = true,
        .async_write = false,    /* 由 UART 适配层自身队列化，日志层无需二次排队 */
    };

    if(log_init(&log_config) != LOG_STATUS_OK) {
        return SYSTEM_STATUS_ERROR;
    }

    return SYSTEM_STATUS_OK;
}
