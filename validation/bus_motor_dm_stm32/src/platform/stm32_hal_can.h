#ifndef _stm32_hal_can_h_
#define _stm32_hal_can_h_

#include "main.h" // IWYU pragma: keep

#include <stdint.h>

// ! ========================= 接 口 变 量 / Typedef 声 明 ========================= ! //

extern FDCAN_HandleTypeDef hfdcan1;

#define STM32_HAL_CAN_STATUS_TABLE                               \
    X(OK, "OK")                                                  \
    X(INVALID_PARAM, "Invalid Parameter")                        \
    X(INVALID_DLC, "Invalid DLC")                                \
    X(FILTER_CONFIG_FAILED, "CAN Filter Config Failed")          \
    X(START_FAILED, "CAN Start Failed")                          \
    X(NOTIFICATION_FAILED, "CAN Notification Activation Failed") \
    X(TX_MAILBOX_TIMEOUT, "CAN TX Mailbox Timeout")              \
    X(TX_FAILED, "CAN TX Failed")                                \
    X(RX_FAILED, "CAN RX Failed")                                \
    X(NO_CALLBACK_SLOT, "No CAN RX Callback Slot")

#define X(name, str) STM32_HAL_CAN_##name,
/**
 * @brief CAN 平台操作结果
 */
typedef enum {
    STM32_HAL_CAN_STATUS_TABLE
} BspCanStatus;
#undef X

/**
 * @brief RX FIFO0 帧监听回调
 * @param hcan 产生该帧的 FDCAN 实例
 * @param header HAL 接收头，只在回调期间有效
 * @param data 固定 8 字节工作缓冲，短帧尾部补零，只在回调期间有效
 * @param user 注册时提供的用户上下文
 */
typedef void (*STM32HalCanRxCallback)(FDCAN_HandleTypeDef* hcan,
                                      const FDCAN_RxHeaderTypeDef* header,
                                      const uint8_t data[8],
                                      void* user);

// ! ========================= 接 口 函 数 声 明 ========================= ! //

/**
 * @brief 配置测试用全局过滤器：接收数据帧到 FIFO0、拒绝远程帧
 * @param hcan FDCAN 句柄指针
 * @return 状态码
 */
BspCanStatus can_filter_init(FDCAN_HandleTypeDef* hcan);
/**
 * @brief 使能 FIFO0 通知、拉高 PC14 并启动 FDCAN
 * @param hcan FDCAN 句柄指针
 * @return 状态码
 */
BspCanStatus can_start(FDCAN_HandleTypeDef* hcan);
/**
 * @brief 非阻塞提交一帧 0~8 字节经典 CAN 数据
 * @param hcan FDCAN 句柄指针
 * @param id 设备或协议 ID
 * @param data 数据缓冲区指针
 * @param len 数据长度
 * @return 状态码
 */
BspCanStatus can_send(FDCAN_HandleTypeDef* hcan, uint32_t id, const uint8_t* data, uint8_t len);
/**
 * @brief 注册或更新一个同步 RX 帧监听者
 * @param hcan FDCAN 句柄指针
 * @param callback 回调函数指针
 * @param user 用户上下文指针
 * @return 状态码
 */
BspCanStatus can_register_rx_callback(FDCAN_HandleTypeDef* hcan, STM32HalCanRxCallback callback, void* user);
/**
 * @brief 把平台状态转换为静态诊断字符串
 * @param status 状态值
 * @return 字符串指针
 */
const char* can_error_code_to_str(BspCanStatus status);

#endif
