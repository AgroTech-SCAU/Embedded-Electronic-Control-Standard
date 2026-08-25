#include "assemble.h"

#include "rgb_led/rgb_led.h"
#include "rgb_led/ws2812_rgb_led.h"
#include "stm32_hal_spi.h"

// ! ========================= 变 量 声 明 ========================= ! //

static uint8_t rgb_color_buffer[WS2812_RGB_LED_DEFAULT_PIXEL_COUNT * RGB_LED_COLOR_BYTES];
/**
 * WS2812 SPI 波形缓冲区；DMA 位于 D2 域而 CPU 数据缓存位于 D1 域，因此
 * 放入专用 D3 RAM 并按 32 字节对齐，避免缓存一致性造成灯色偶发错误
 */
static uint8_t rgb_tx_buffer[WS2812_RGB_LED_DEFAULT_PIXEL_COUNT * WS2812_RGB_LED_BITS_PER_PIXEL + WS2812_RGB_LED_DEFAULT_RESET_BYTES]
    __attribute__((section(".ram_d3"), aligned(32)));

static const RgbLedPortOps rgb_ops = {
    .write = spi_write,
};

// ! ========================= 私 有 函 数 声 明 ========================= ! //

/**
 * @brief SPI6 DMA 完成后通知 RGB 驱动释放 busy 状态
 * @param hspi SPI 句柄指针
 */
static void rgb_led_write_complete_callback(SPI_HandleTypeDef* hspi);

// ! ========================= 接 口 函 数 实 现 ========================= ! //

SystemStatus assemble_rgb(void) {
    RgbLedConfig rgb_config;

    if(rgb_led_set_instance(&ws2812_rgb_led_instance) != RGB_LED_STATUS_OK) {
        return SYSTEM_STATUS_ERROR;
    }

    if(ws2812_rgb_led_make_config(
           &rgb_config,
           &rgb_ops,
           rgb_color_buffer,
           sizeof(rgb_color_buffer),
           rgb_tx_buffer,
           sizeof(rgb_tx_buffer)) != RGB_LED_STATUS_OK) {
        return SYSTEM_STATUS_ERROR;
    }

    /* SPI6 使用 DMA；show() 只启动传输，完成由 HAL 回调异步通知 */
    rgb_config.async_write = true;

    if(rgb_led.init(&rgb_config) != RGB_LED_STATUS_OK) {
        return SYSTEM_STATUS_ERROR;
    }

    /* 回调必须先注册，确保第一次 show() 完成时驱动能清除发送状态 */
    spi_register_tx_complete_callback(&hspi6, rgb_led_write_complete_callback);

    rgb_led.fill(255U, 0U, 0U);
    rgb_led.show();
    return SYSTEM_STATUS_OK;
}

// ! ========================= 私 有 函 数 实 现 ========================= ! //

static void rgb_led_write_complete_callback(SPI_HandleTypeDef* hspi) {
    (void)hspi;
    /* 完成或错误回调都释放驱动持有的发送缓冲区 */
    (void)rgb_led_write_complete();
}
