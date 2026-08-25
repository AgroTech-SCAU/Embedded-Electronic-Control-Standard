#include "assemble.h"

#include "delay.h"
#include "stm32_hal_dwt.h"
#include "main.h" // IWYU pragma: keep

// ! ========================= 变 量 声 明 ========================= ! //

/** 上电后给电源、调试串口和外设时钟预留的稳定时间 */
#define ASSEMBLE_BOOT_SETTLE_DELAY_MS 1500u

// ! ========================= 私 有 函 数 声 明 ========================= ! //



// ! ========================= 接 口 函 数 实 现 ========================= ! //

SystemStatus assemble_delay(void) {
    /* DWT 必须先于微秒延时服务初始化 */
    dwt_init();

    delay_ms_init(HAL_GetTick);
    delay_us_init(dwt_get_us);

    delay_ms(ASSEMBLE_BOOT_SETTLE_DELAY_MS);

    return SYSTEM_STATUS_OK;
}

// ! ========================= 私 有 函 数 实 现 ========================= ! //
