#ifndef _motor_test_h_
#define _motor_test_h_

#include "bus_motor/bus_motor.h"

#include <stdbool.h>

/**
 * @brief 电机测试业务使用的逻辑电机标识
 *
 * 此标识只表达被测试电机在业务中的身份 不编码 CAN 地址或任何硬件配置
 */
typedef enum {
    MOTOR_TEST_ID_MAIN = 0u,
} MotorTestMotorId;

/**
 * @brief 初始化电机测试业务状态机
 * @return 初始化并完成安全失能和清错时返回 true
 */
bool motor_test_init(void);

/**
 * @brief 推进电机测试业务状态机
 */
void motor_test_process(void);

/**
 * @brief 从中断上下文交付已解析的电机反馈
 * @param feedback 由设备层解析完成的业务逻辑反馈
 */
void motor_test_on_feedback_from_isr(const BusMotorFeedback* feedback);

#endif
