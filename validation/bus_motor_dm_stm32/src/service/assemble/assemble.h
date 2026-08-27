#ifndef _assemble_h_
#define _assemble_h_

/**
 * @file assemble.h
 * @brief 达妙电机测试的硬件装配接口
 */

/**
 * @brief 装配状态码
 */
typedef enum {
    SYSTEM_STATUS_OK = 0,
    SYSTEM_STATUS_ERROR,
} SystemStatus;

/** 初始化 DWT 微秒时基和 HAL 毫秒时基，并等待上电稳定 */
SystemStatus assemble_delay(void);

/** 将日志服务连接到 USART1 的中断发送队列 */
SystemStatus assemble_log(void);

/** 将 RGB 抽象连接到 WS2812 编码器和 SPI6 DMA */
SystemStatus assemble_rgb(void);

/** 初始化 CAN1 并绑定达妙电机硬件与驱动 */
SystemStatus assemble_motor(void);

#endif
