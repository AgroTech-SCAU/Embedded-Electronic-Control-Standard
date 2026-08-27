#ifndef _stm32_hal_dwt_h_
#define _stm32_hal_dwt_h_

#include <stdint.h>

/**
 * @file stm32_hal_dwt.h
 * @brief DWT 微秒时基接口
 */

/**
 * @brief 开启并清零 DWT CYCCNT，同时建立周期到微秒的换算状态
 */
void dwt_init(void);
/**
 * @brief 累计自初始化以来经过的微秒数
 * @return uint32_t 微秒时间戳；未初始化时返回 0
 */
uint32_t dwt_get_us(void);

#endif
