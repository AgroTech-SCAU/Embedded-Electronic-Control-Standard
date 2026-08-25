/**
 * @file assemble_motor.c
 * @brief 达妙电机硬件映射与 CAN1 驱动装配
 */

#include "assemble.h"

#include "bus_motor/bus_motor.h"
#include "bus_motor/dm_motor.h"
#include "delay.h"
#include "log.h"
#include "main.h"
#include "motor_test.h"
#include "stm32_hal_can.h"

#define MOTOR_CAN_ENABLE_DELAY_MS 10u

/**
 * 逻辑 ID 标识业务中的电机角色 can_id 是电机接收命令的物理 CAN 地址
 * master_id 是主控接收该电机反馈的物理 CAN 地址
 */
typedef struct {
    BusMotorId logical_id;
    DmMotorConfig dm_config;
} DmMotorBinding;

static const DmMotorBinding s_motor_bindings[] = {
    {
        .logical_id = (BusMotorId)MOTOR_TEST_ID_MAIN,
        .dm_config = {
            .can_id = 0x01u,
            .master_id = 0x11u,
            .model = DM_MOTOR_MODEL_DMG6220,
            .firmware = {
                .major = DM_MOTOR_FIRMWARE_V4,
            },
            .default_mode = DM_MOTOR_MODE_POS_VEL,
        },
    },
};

/** 驱动实例池与硬件绑定表保持一一对应 */
static DmMotorInstance s_dm_instances[sizeof(s_motor_bindings) / sizeof(s_motor_bindings[0])];

static bool assemble_motor_can_send(uint32_t id, const uint8_t* data, uint8_t len);

static const BusMotorPortOps s_motor_ops = {
    .send = assemble_motor_can_send,
    /* FDCAN RX 中断通过 can_register_rx_callback 推送帧 因此 read 不使用 */
    .read = 0,
    .now_ms = HAL_GetTick,
    .delay_ms = delay_ms,
    /* can_register_rx_callback 绑定的推送接收没有待清空队列 因此 flush_rx 不使用 */
    .flush_rx = 0,
};

static void assemble_motor_can_rx(FDCAN_HandleTypeDef* hcan,
                                  const FDCAN_RxHeaderTypeDef* header,
                                  const uint8_t data[8],
                                  void* user);

SystemStatus assemble_motor(void) {
    uint16_t i;

    if(bus_motor.init() != MOTOR_STATUS_OK) {
        return SYSTEM_STATUS_ERROR;
    }
    if(dm_motor_init(&s_motor_ops,
                     s_dm_instances,
                     (uint16_t)(sizeof(s_dm_instances) / sizeof(s_dm_instances[0]))) != MOTOR_STATUS_OK) {
        return SYSTEM_STATUS_ERROR;
    }

    for(i = 0u; i < (uint16_t)(sizeof(s_motor_bindings) / sizeof(s_motor_bindings[0])); i++) {
        if(dm_motor_bind(s_motor_bindings[i].logical_id, &s_motor_bindings[i].dm_config) != MOTOR_STATUS_OK) {
            return SYSTEM_STATUS_ERROR;
        }
    }

    for(i = 0u; i < (uint16_t)(sizeof(s_motor_bindings) / sizeof(s_motor_bindings[0])); i++) {
        const DmMotorBinding* binding = &s_motor_bindings[i];

        log_info("MOTOR binding: logical_id=%u can_id=0x%02X master_id=0x%02X model=%u firmware=%u default_mode=%u",
                 (unsigned int)binding->logical_id,
                 (unsigned int)binding->dm_config.can_id,
                 (unsigned int)binding->dm_config.master_id,
                 (unsigned int)binding->dm_config.model,
                 (unsigned int)binding->dm_config.firmware.major,
                 (unsigned int)binding->dm_config.default_mode);
    }

    /* PC14 高电平使能 CAN1 收发器 等待稳定后再启动 FDCAN */
    HAL_GPIO_WritePin(CAN1_EN_GPIO_Port, CAN1_EN_Pin, GPIO_PIN_SET);
    delay_ms(MOTOR_CAN_ENABLE_DELAY_MS);

    if(can_register_rx_callback(&hfdcan1, assemble_motor_can_rx, 0) != STM32_HAL_CAN_OK) {
        return SYSTEM_STATUS_ERROR;
    }
    if(can_filter_init(&hfdcan1) != STM32_HAL_CAN_OK) {
        return SYSTEM_STATUS_ERROR;
    }
    if(can_start(&hfdcan1) != STM32_HAL_CAN_OK) {
        return SYSTEM_STATUS_ERROR;
    }

    return SYSTEM_STATUS_OK;
}

static bool assemble_motor_can_send(uint32_t id, const uint8_t* data, uint8_t len) {
    return can_send(&hfdcan1, id, data, len) == STM32_HAL_CAN_OK;
}

static void assemble_motor_can_rx(FDCAN_HandleTypeDef* hcan,
                                  const FDCAN_RxHeaderTypeDef* header,
                                  const uint8_t data[8],
                                  void* user) {
    BusMotorFeedback feedback;

    (void)hcan;
    (void)user;

    if(header == 0) {
        return;
    }

    if(dm_motor_parse_parameter_frame(header->Identifier, data)) {
        return;
    }

    if(dm_motor_parse_feedback_frame(header->Identifier, data, &feedback) == MOTOR_STATUS_OK) {
        motor_test_on_feedback_from_isr(&feedback);
    }
}
