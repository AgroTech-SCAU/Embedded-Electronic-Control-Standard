#ifndef _sts3215_servo_h_
#define _sts3215_servo_h_

#include "servo.h"

// ! ========================= 接 口 变 量 / Typedef 声 明 ========================= ! //

/** @brief SCS 协议广播 ID，除特殊命令外通常不等待应答 */
#define STS3215_BROADCAST_ID 0xFEu

/* STS3215 控制表地址：EPROM 区域 */
#define STS3215_MODEL_L 3u
#define STS3215_MODEL_H 4u
#define STS3215_ID 5u
#define STS3215_BAUD_RATE 6u
#define STS3215_MIN_ANGLE_LIMIT_L 9u
#define STS3215_MIN_ANGLE_LIMIT_H 10u
#define STS3215_MAX_ANGLE_LIMIT_L 11u
#define STS3215_MAX_ANGLE_LIMIT_H 12u
#define STS3215_CW_DEAD 26u
#define STS3215_CCW_DEAD 27u
#define STS3215_OFS_L 31u
#define STS3215_OFS_H 32u
#define STS3215_MODE 33u
/* STS3215 控制表地址：SRAM 可写控制区域 */
#define STS3215_TORQUE_ENABLE 40u
#define STS3215_ACC 41u
#define STS3215_GOAL_POSITION_L 42u
#define STS3215_GOAL_POSITION_H 43u
#define STS3215_GOAL_TIME_L 44u
#define STS3215_GOAL_TIME_H 45u
#define STS3215_GOAL_SPEED_L 46u
#define STS3215_GOAL_SPEED_H 47u
#define STS3215_LOCK 55u
/* STS3215 控制表地址：SRAM 只读反馈区域 */
#define STS3215_PRESENT_POSITION_L 56u
#define STS3215_PRESENT_POSITION_H 57u
#define STS3215_PRESENT_SPEED_L 58u
#define STS3215_PRESENT_SPEED_H 59u
#define STS3215_PRESENT_LOAD_L 60u
#define STS3215_PRESENT_LOAD_H 61u
#define STS3215_PRESENT_VOLTAGE 62u
#define STS3215_PRESENT_TEMPERATURE 63u
#define STS3215_MOVING 66u
#define STS3215_PRESENT_CURRENT_L 69u
#define STS3215_PRESENT_CURRENT_H 70u

/* STS3215 波特率寄存器取值 */
#define STS3215_BAUD_1M 0u
#define STS3215_BAUD_500K 1u
#define STS3215_BAUD_250K 2u
#define STS3215_BAUD_128K 3u
#define STS3215_BAUD_115200 4u
#define STS3215_BAUD_76800 5u
#define STS3215_BAUD_57600 6u
#define STS3215_BAUD_38400 7u

/**
 * @brief STS3215 舵机实例
 *
 * service 层通过 servo_set_instance(&sts3215_servo_instance) 绑定到统一入口
 */
extern const ServoInterface sts3215_servo_instance;

// ! ========================= 接 口 函 数 声 明 ========================= ! //

/**
 * @brief 将带方向位的协议原始值转换为有符号值
 * @param value 原始寄存器值
 * @param sign_bit 方向/符号位位置
 */
int16_t sts3215_servo_raw_to_signed(uint16_t value, uint8_t sign_bit);
/**
 * @brief 将有符号值转换为 STS3215 使用的方向位编码
 */
uint16_t sts3215_servo_signed_to_raw(int16_t value);

#endif
