#ifndef WS2812_RGB_LED_H
#define WS2812_RGB_LED_H

#include "rgb_led.h"

// ! ========================= 接 口 变 量 / Typedef 声 明 ========================= ! //

/**
 * @brief WS2812 单个 RGB 灯珠编码后的 bit 数
 */
#define WS2812_RGB_LED_BITS_PER_PIXEL 24u

/**
 * @brief WS2812 RGB LED 实例
 *
 * service 可通过 rgb_led_set_instance(&ws2812_rgb_led_instance)
 * 将其绑定为 rgb_led 统一入口的具体实现。
 */
extern const RgbLedInterface ws2812_rgb_led_instance;

// ! ========================= 接 口 函 数 声 明 ========================= ! //

#endif
