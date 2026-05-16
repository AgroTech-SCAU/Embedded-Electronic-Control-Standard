#ifndef DM_MOTOR_H
#define DM_MOTOR_H

#include "bus_motor.h"

// ! ========================= 接 口 变 量 / Typedef 声 明 ========================= ! //

/**
 * @brief 达妙电机命令帧长度，单位 byte
 */
#define DM_MOTOR_CMD_LEN 8u

/**
 * @brief 达妙 MIT 模式位置限幅，单位 rad
 */
#define DM_MOTOR_POS_LIMIT 12.5f

/**
 * @brief 达妙 MIT 模式速度限幅，单位 rad/s
 */
#define DM_MOTOR_SPD_LIMIT 10.0f

/**
 * @brief 达妙 MIT 模式转矩电流限幅，单位 A
 */
#define DM_MOTOR_TORQUE_LIMIT 28.0f

/**
 * @brief 达妙 MIT 模式 kp 上限
 */
#define DM_MOTOR_KP_LIMIT 500.0f

/**
 * @brief 达妙 MIT 模式 kd 上限
 */
#define DM_MOTOR_KD_LIMIT 5.0f

/**
 * @brief 达妙电机实例，service 可通过 motor_set_instance(&dm_motor_instance) 绑定为统一入口
 */
extern const BusMotorInterface dm_motor_instance;

// ! ========================= 接 口 函 数 声 明 ========================= ! //

#endif
