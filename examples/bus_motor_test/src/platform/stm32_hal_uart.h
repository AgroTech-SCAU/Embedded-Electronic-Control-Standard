#ifndef _stm32_hal_uart_h_
#define _stm32_hal_uart_h_

#include "main.h" // IWYU pragma: keep

#include <stdbool.h>
#include <stdint.h>

/**
 * @file stm32_hal_uart.h
 * @brief USART1 中断发送接口
 */

extern UART_HandleTypeDef huart1;

// ! ========================= 接 口 函 数 声 明 ========================= ! //

/**
 * @brief 将一条完整日志复制到 USART1 环形发送队列
 * @param data 待发送字节；函数返回前已完成复制，调用方可立即复用
 * @param len 字节数，必须小于队列总容量
 * @return true 表示整条数据已入队，false 表示参数非法或剩余空间不足
 */
bool uart1_write_it(const char* data, uint32_t len);

#endif
