#ifndef _stm32_hal_spi_h_
#define _stm32_hal_spi_h_

#include <stdbool.h>
#include <stdint.h>

#include "main.h" // IWYU pragma: keep

// ! ========================= 接 口 变 量 / Typedef 声 明 ========================= ! //

/**
 * @brief RGB 灯带所用 SPI6 句柄
 */
extern SPI_HandleTypeDef hspi6;

// ! ========================= 接 口 函 数 声 明 ========================= ! //

/**
 * @brief 通过 SPI6 发送一段 DMA 数据
 * @param data 数据缓冲区
 * @param len 数据长度，单位 byte
 * @return bool `true` 表示发送启动成功
 */
bool spi_write(const uint8_t* data, uint32_t len);

/**
 * @brief 注册 SPI 发送完成回调
 * @param hspi SPI 句柄
 * @param callback 回调函数
 */
void spi_register_tx_complete_callback(SPI_HandleTypeDef* hspi, void (*callback)(SPI_HandleTypeDef* hspi));

#endif
