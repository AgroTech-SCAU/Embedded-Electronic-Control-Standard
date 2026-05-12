# rgb_led SDK 接口文档

> `sdks/device/rgb_led/` 提供 RGB 灯设备层统一入口；`ws2812_rgb_led.*` 是 WS2812 灯珠的具体实现

---

## 1. 模块定位

`rgb_led.*` 是 RGB 灯统一接口入口，供 service/app 上层调用

`ws2812_rgb_led.*` 是 WS2812 实例，负责 GRB 颜色缓存、bit 编码和底层写入 PortOps

推荐使用方式：

```text
entry/service init -> 组装 RgbLedPortOps ->
rgb_led_set_instance(&ws2812_rgb_led_instance) ->
ws2812_rgb_led_make_config(...) ->
rgb_led.init(&config) ->
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

上层不直接依赖某一个具体 RGB 灯实现，只在初始化阶段绑定实例，之后调用统一接口

```c
rgb_led_set_instance(&ws2812_rgb_led_instance);
rgb_led.init(&config);
rgb_led.fill(255, 0, 0);
rgb_led.show();
```

也可以使用函数形式：

```c
rgb_led_init(&config);
rgb_led_fill(255, 0, 0);
rgb_led_show();
```

---

## 4. PortOps

```c
typedef struct {
    int (*write)(const uint8_t* data, uint32_t len);
} RgbLedPortOps;
```

PortOps 由 entry/service 绑定 platform 或 adapter；RGB LED SDK 不直接依赖 HAL/FSP/CubeMX

`write` 接收的是已经由具体实例编码好的时序发送缓存；返回 `0` 表示成功，非 `0` 表示底层输出失败

---

## 5. 完整初始化示例

下面示例展示在 `entry.h` 中绑定 WS2812 RGB LED 实例，并通过 STM32 SPI 输出时序数据

```c
#ifndef _entry_h_
#define _entry_h_

// ! app ! //



// ! service ! //



// ! device ! //
#include "rgb_led/rgb_led.h"
#include "rgb_led/ws2812_rgb_led.h"

// ! domain ! //



// ! infra ! //



// ! platform ! //
#include "stm32_hal_spi.h"


// ! ========================= 接 口 变 量 / Typedef 声 明 ========================= ! //

static uint8_t rgb_color_buffer[WS2812_RGB_LED_DEFAULT_PIXEL_COUNT * RGB_LED_COLOR_BYTES];
static uint8_t rgb_tx_buffer[WS2812_RGB_LED_DEFAULT_PIXEL_COUNT * WS2812_RGB_LED_BITS_PER_PIXEL + WS2812_RGB_LED_DEFAULT_RESET_BYTES];

static const RgbLedPortOps rgb_ops = {
    .write = spi_write,
};

// ! ========================= 接 口 函 数 声 明 ========================= ! //

/**
 * @brief 程序初始化入口函数
 */
static inline void entry_init(void) {
    RgbLedConfig rgb_config;
    rgb_led_set_instance(&ws2812_rgb_led_instance);
    if(ws2812_rgb_led_make_config(&rgb_config, &rgb_ops, rgb_color_buffer, sizeof(rgb_color_buffer), rgb_tx_buffer, sizeof(rgb_tx_buffer)) == RGB_LED_STATUS_OK) {
        rgb_led.init(&rgb_config);
        rgb_led.fill(255, 0, 0);
        rgb_led.show();
    }
}

/**
 * @brief 程序主循环入口函数
 */
static inline void entry_loop(void) {

}

#endif
```

初始化流程要点：

- `rgb_color_buffer` 保存每个像素的 RGB 颜色缓存
- `rgb_tx_buffer` 保存 WS2812 编码后的 SPI 发送数据和 reset 低电平数据
- `rgb_ops.write` 绑定平台层的 `spi_write`
- `rgb_led_set_instance(&ws2812_rgb_led_instance)` 先绑定具体实现
- `ws2812_rgb_led_make_config(...)` 统一填充 WS2812 默认配置
- `rgb_led.fill(...)` 只更新颜色缓存，`rgb_led.show()` 才真正编码并发送

---

## 6. 设计约束

- 上层优先依赖 `rgb_led.h` 统一接口
- 具体实现只负责自己的颜色缓存、时序编码和发送实现
- `ws2812_rgb_led.*` 不直接 include platform 头文件
- platform 对接统一放在 entry/service init 或 adapter 中
- 调用 `rgb_led.xxx` 前需要先 `rgb_led_set_instance(...)`
- 修改颜色后需要调用 `rgb_led.show()` 才会刷新到真实灯带
