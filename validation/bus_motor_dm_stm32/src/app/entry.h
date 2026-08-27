#ifndef _app_entry_h_
#define _app_entry_h_

#include "assemble/assemble.h"
#include "delay.h"
#include "log.h"
#include "main.h" // IWYU pragma: keep
#include "motor_test.h"

#include <stdbool.h>

// ! ========================= 变 量 声 明 ========================= ! //

/** 硬件装配和业务初始化全部成功后才开启主循环 */
static bool init_ok = false;

// ! ========================= 接 口 函 数 实 现 ========================= ! //

static inline void entry_init(void) {
    /* 按依赖顺序装配硬件，最后初始化电机测试业务 */
    if(assemble_delay() != SYSTEM_STATUS_OK)
        return;
    delay_ms(100);

    if(assemble_log() != SYSTEM_STATUS_OK)
        return;
    log_info("BOOT log ready");
    delay_ms(100);

    if(assemble_rgb() != SYSTEM_STATUS_OK)
        return;
    log_info("BOOT rgb ready");
    delay_ms(100);

    if(assemble_motor() != SYSTEM_STATUS_OK)
        return;
    if(!motor_test_init())
        return;
    log_info("BOOT motor assembly ready");
    delay_ms(100);
    log_info("BOOT motor test ready");
    delay_ms(100);

    init_ok = true;
    log_info("System initialized successfully");
    delay_ms(100);
}

static inline void entry_loop(void) {
    if(!init_ok) {
        return;
    }

    motor_test_process();
}

#endif
