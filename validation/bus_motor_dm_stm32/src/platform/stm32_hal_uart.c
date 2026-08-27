#include "stm32_hal_uart.h"

#include <stddef.h>
#include <string.h>

/**
 * @file stm32_hal_uart.c
 * @brief 日志专用 USART1 非阻塞环形发送队列
 *
 * 前台把完整日志复制进环形缓冲区，HAL 中断发送连续的一段；发送完成后
 * 消费该段并自动启动下一段；队列预留一个字节区分“空”和“满”
 */

#define UART1_TX_QUEUE_SIZE 2048u

// ! ========================= 类 型 声 明 ========================= ! //

typedef struct {
    UART_HandleTypeDef* huart;
    uint8_t* buffer;
    uint16_t size;
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint16_t active_len;
    volatile bool busy;
} UartTxQueue;

// ! ========================= 变 量 声 明 ========================= ! //

static uint8_t s_uart1_tx_buffer[UART1_TX_QUEUE_SIZE] = { 0 };
static UartTxQueue s_uart1_tx = {
    .huart = &huart1,
    .buffer = s_uart1_tx_buffer,
    .size = UART1_TX_QUEUE_SIZE,
};

// ! ========================= 私 有 函 数 声 明 ========================= ! //

/**
 * @brief uart_write_it 内部辅助函数
 * @param queue 队列指针
 * @param data 数据缓冲区指针
 * @param len 数据长度
 * @return true 表示成功或条件成立，false 表示失败或条件不成立
 */
static bool uart_write_it(UartTxQueue* queue, const char* data, uint32_t len);
/**
 * @brief uart_tx_start 内部辅助函数
 * @param queue 队列指针
 */
static void uart_tx_start(UartTxQueue* queue);
/**
 * @brief uart_tx_complete 内部辅助函数
 * @param queue 队列指针
 */
static void uart_tx_complete(UartTxQueue* queue);
/**
 * @brief uart_tx_error 内部辅助函数
 * @param queue 队列指针
 */
static void uart_tx_error(UartTxQueue* queue);

// ! ========================= 接 口 函 数 实 现 ========================= ! //

bool uart1_write_it(const char* data, uint32_t len) {
    return uart_write_it(&s_uart1_tx, data, len);
}

// ! ========================= HAL 回 调 实 现 ========================= ! //

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
    if(huart == &huart1) {
        uart_tx_complete(&s_uart1_tx);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {
    if(huart == &huart1) {
        uart_tx_error(&s_uart1_tx);
    }
}

// ! ========================= 私 有 函 数 实 现 ========================= ! //

static bool uart_write_it(UartTxQueue* queue, const char* data, uint32_t len) {
    uint16_t used;
    uint16_t free_len;
    uint16_t first_len;

    if(queue == NULL || data == NULL || len == 0u || len >= queue->size) {
        return false;
    }

    /* 前台与中断共享 head/tail，完整复制后才提交新 head */
    __disable_irq();
    if(queue->head >= queue->tail) {
        used = (uint16_t)(queue->head - queue->tail);
    }
    else {
        used = (uint16_t)(queue->size - queue->tail + queue->head);
    }
    free_len = (uint16_t)(queue->size - 1u - used);
    if(len > free_len) {
        __enable_irq();
        return false;
    }

    first_len = (uint16_t)(queue->size - queue->head);
    if(first_len > len) {
        first_len = (uint16_t)len;
    }
    memcpy(&queue->buffer[queue->head], data, first_len);
    if(first_len < len) {
        memcpy(queue->buffer, &data[first_len], len - first_len);
    }
    queue->head = (uint16_t)((queue->head + len) % queue->size);
    __enable_irq();

    uart_tx_start(queue);
    return true;
}

static void uart_tx_start(UartTxQueue* queue) {
    uint16_t len;

    if(queue == NULL || queue->busy || queue->head == queue->tail) {
        return;
    }

    if(queue->head > queue->tail) {
        len = (uint16_t)(queue->head - queue->tail);
    }
    else {
        len = (uint16_t)(queue->size - queue->tail);
    }

    /* 调用 HAL 前先发布活动段，避免完成中断看到半提交状态 */
    queue->active_len = len;
    queue->busy = true;
    if(HAL_UART_Transmit_IT(queue->huart, &queue->buffer[queue->tail], len) != HAL_OK) {
        queue->active_len = 0u;
        queue->busy = false;
    }
}

static void uart_tx_complete(UartTxQueue* queue) {
    if(queue == NULL || !queue->busy) {
        return;
    }

    queue->tail = (uint16_t)((queue->tail + queue->active_len) % queue->size);
    queue->active_len = 0u;
    queue->busy = false;
    uart_tx_start(queue);
}

static void uart_tx_error(UartTxQueue* queue) {
    if(queue == NULL) {
        return;
    }

    /* 出错的活动段无法可靠续传，丢弃后继续发送剩余日志 */
    if(queue->busy) {
        queue->tail = (uint16_t)((queue->tail + queue->active_len) % queue->size);
        queue->active_len = 0u;
        queue->busy = false;
    }
    uart_tx_start(queue);
}
