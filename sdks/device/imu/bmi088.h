#ifndef _bmi088_h_
#define _bmi088_h_

#include <stdbool.h>
#include <stdint.h>

#include "imu/imu.h"

// ! ========================= 接 口 变 量 / Typedef 声 明 ========================= ! //

/**
 * @brief BMI088 初始化错误码
 */
typedef enum {
    BMI088_ERROR_NO_ERROR = 0x00,
    BMI088_ERROR_ACC_PWR_CTRL = 0x01,
    BMI088_ERROR_ACC_PWR_CONF = 0x02,
    BMI088_ERROR_ACC_CONF = 0x03,
    BMI088_ERROR_ACC_SELF_TEST = 0x04,
    BMI088_ERROR_ACC_RANGE = 0x05,
    BMI088_ERROR_INT1_IO_CTRL = 0x06,
    BMI088_ERROR_INT_MAP_DATA = 0x07,
    BMI088_ERROR_GYRO_RANGE = 0x08,
    BMI088_ERROR_GYRO_BANDWIDTH = 0x09,
    BMI088_ERROR_GYRO_LPM1 = 0x0A,
    BMI088_ERROR_GYRO_CTRL = 0x0B,
    BMI088_ERROR_GYRO_INT3_INT4_IO_CONF = 0x0C,
    BMI088_ERROR_GYRO_INT3_INT4_IO_MAP = 0x0D,
    BMI088_ERROR_SELF_TEST_ACCEL = 0x80,
    BMI088_ERROR_SELF_TEST_GYRO = 0x40,
    BMI088_ERROR_NO_SENSOR = 0xFF,
} Bmi088Error;

typedef struct {
    void (*accel_cs_low)(void);
    void (*accel_cs_high)(void);
    void (*gyro_cs_low)(void);
    void (*gyro_cs_high)(void);
    uint8_t(*read_write_byte)(uint8_t tx_data);
    bool (*transmit_receive_dma)(uint8_t* tx_data, uint8_t* rx_data, uint16_t len);
    void* (*get_spi_handle)(void);
    uint32_t(*now_ms)(void);
    void (*delay_ms)(uint32_t ms);
    void (*delay_us)(uint16_t us);
} Bmi088PortOps;

typedef struct {
    const Bmi088PortOps* ops;
    float accel_sen;
    float gyro_sen;
    uint16_t accel_int_pin;
    uint16_t gyro_int_pin;
    ImuAttitudeConfig attitude;
} Bmi088Config;

/**
 * @brief BMI088 IT + DMA IMU 实例
 */
extern const ImuInterface bmi088_instance;

/**
 * @brief BMI088 阻塞式 IMU 实例
 */
extern const ImuInterface bmi088_blocking_instance;

// ! ========================= 接 口 函 数 声 明 ========================= ! //

ImuStatus bmi088_make_config(Bmi088Config* config, const Bmi088PortOps* ops, const uint16_t accel_int_pin, const uint16_t gyro_int_pin);
const char* bmi088_error_str(Bmi088Error error);
Bmi088Error bmi088_get_init_error(void);
float bmi088_get_temp(void);
void bmi088_exti_callback(uint16_t gpio_pin);
void bmi088_spi_txrx_cplt_callback(void* spi_handle);
void bmi088_spi_error_callback(void* spi_handle);

#endif
