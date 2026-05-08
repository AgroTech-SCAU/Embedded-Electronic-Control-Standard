# rgb_led SDK 接口文档

> `sdks/device/rgb_led/` 提供 RGB 灯设备层统一入口；`ws2812_rgb_led.*` 是 WS2812 灯珠的具体实例

---

## 1. 模块定位

`rgb_led.*` 是 RGB 灯统一接口入口，供 service/app 上层调用

`ws2812_rgb_led.*` 是 WS2812 实例，实现 GRB 颜色缓存、bit 编码和底层写入 PortOps

推荐使用方式：

```text
service init → 组装 RgbLedPortOps →
rgb_led_set_instance(&ws2812_rgb_led_instance) → rgb_led_init(&config)→
app/service 统一调用 rgb_led.xxx 或 rgb_led_xxx
```

---

## 2. 文件结构

```text
sdks/device/rgb_led/
├── README.md
├── rgb_led.h
├── rgb_led.c
├── ws2812_rgb_led.h
└── ws2812_rgb_led.c
```

---

## 3. 统一接口

```c
#define rgb_led (*rgb_led_instance)

extern const RgbLedInterface* rgb_led_instance;

RgbLedStatus rgb_led_set_instance(const RgbLedInterface* instance);
```

上层不直接依赖某个具体 RGB 灯实现，只绑定实例后调用统一入口

```c
rgb_led_set_instance(&ws2812_rgb_led_instance);
rgb_led_init(&config);
rgb_led.set_rgb(0, 20, 0, 0);
rgb_led.show();
```

---

## 4. PortOps

```c
typedef struct {
    int (*write)(const uint8_t* data, uint32_t len);
} RgbLedPortOps;
```

PortOps 由 service 绑定 platform 或 adapter，RGB LED SDK 不直接依赖 HAL/FSP/CubeMX

---

## 5. 初始化示例

```c
#include "rgb_led.h"
#include "ws2812_rgb_led.h"
#include "platform_spi.h"

static uint8_t led_color_buffer[8 * RGB_LED_COLOR_BYTES];
static uint8_t led_tx_buffer[8 * WS2812_RGB_LED_BITS_PER_PIXEL + 80];

static const RgbLedPortOps led_ops = {
    .write = platform_spi_write,
};

void led_service_init(void)
{
    RgbLedConfig config = {
        .ops = &led_ops,
        .pixel_count = 8,
        .color_buffer = led_color_buffer,
        .color_buffer_size = sizeof(led_color_buffer),
        .tx_buffer = led_tx_buffer,
        .tx_buffer_size = sizeof(led_tx_buffer),
        .reset_bytes = 80,
    };

    rgb_led_set_instance(&ws2812_rgb_led_instance);
    rgb_led_init(&config);
}
```

---

## 6. 设计约束

- 上层只依赖 `rgb_led.h`
- 具体实例只负责自己的编码和发送实现
- `ws2812_rgb_led.*` 不直接 include platform 头文件
- platform 对接统一放在 service init 或 adapter 中
