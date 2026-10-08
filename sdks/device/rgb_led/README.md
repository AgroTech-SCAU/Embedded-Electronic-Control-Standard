# RGB LED

统一 RGB 门面 当前提供 WS2812 实现

## 基本使用

```c
RgbLedConfig config;
ws2812_rgb_led_make_config(&config, &ops, pixel_count, tx_buffer, tx_buffer_size);

rgb_led_set_instance(&ws2812_rgb_led_instance);
rgb_led_init(&config);
rgb_led_fill(0, 32, 0);
rgb_led_show();
```

单灯可使用

```c
rgb_led_set_rgb(index, r, g, b);
```

异步发送完成后调用

```c
rgb_led_write_complete();
```

完整接口以 `rgb_led.h` 和 `ws2812_rgb_led.h` 为准

## PortOps

平台只需要提供发送能力

同步和异步发送都可以实现，但异步模式下配置和发送缓存必须保持有效直到完成通知

## 边界

- RGB 颜色业务语义由项目决定
- device 不固定 SPI DMA 或定时器实现
- WS2812 使用 GRB 编码细节由驱动内部处理
