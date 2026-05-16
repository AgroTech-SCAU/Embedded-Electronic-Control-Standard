#ifndef MOTOR_H
#define MOTOR_H

#include <stdbool.h>
#include <stdint.h>

// ! ========================= 接 口 变 量 / Typedef 声 明 ========================= ! //

/**
 * @brief 电机入口单例，用户上层统一调用 bus_motor.xxx 或 bus_motor_xxx
 */
#define bus_motor (*bus_motor_instance)

/**
 * @brief 通用电机状态码表，使用 X-Macro 定义，方便维护和扩展
 * @param OK 操作成功
 * @param ERROR 操作失败
 * @param INVALID_PARAM 参数无效
 * @param PORT_ERROR PortOps 未提供或底层端口失败
 * @param TIMEOUT 等待反馈超时
 * @param ID_MISMATCH 反馈 ID 与期望 ID 不一致
 * @param NO_INSTANCE 未绑定具体电机实例
 * @param NOT_INITIALIZE 具体电机实例尚未初始化
 */
#define MOTOR_STATUS_TABLE \
    X(OK, 0) \
    X(ERROR, 1) \
    X(INVALID_PARAM, 2) \
    X(PORT_ERROR, 3) \
    X(TIMEOUT, 4) \
    X(ID_MISMATCH, 5) \
    X(NO_INSTANCE, 6) \
    X(NOT_INITIALIZE, 7)

/**
 * @brief 通用电机控制模式表
 */
#define MOTOR_MODE_TABLE \
    Y(MIT, 1) \
    Y(POS_SPD, 2) \
    Y(SPD, 3) \
    Y(POS_SPD_CUR, 4)

#define X(name, value) MOTOR_STATUS_##name = value,
typedef enum {
    MOTOR_STATUS_TABLE
} BusMotorStatus;
#undef X

#define Y(name, value) MOTOR_MODE_##name = value,
typedef enum {
    MOTOR_MODE_TABLE
} BusMotorMode;
#undef Y

/**
 * @brief 电机反馈信息
 * @param id 电机反馈 ID
 * @param err_code 电机原始错误码
 * @param pos_rad 位置，单位 rad
 * @param spd_rad_s 速度，单位 rad/s
 * @param torque_a 转矩电流，单位 A
 */
typedef struct {
    uint16_t id;
    uint8_t err_code;
    float pos_rad;
    float spd_rad_s;
    float torque_a;
} BusMotorFeedback;

/**
 * @brief 电机底层端口函数表，由 service init 绑定 platform 或 adapter
 * @param send 发送一帧电机总线数据，返回 true 表示成功
 * @param read 读取一帧电机总线数据，返回 true 表示成功
 * @param now_ms 获取系统毫秒时基
 * @param delay_ms 可选阻塞延时函数，用于部分电机协议的短等待
 */
typedef struct {
    bool(*send)(uint32_t id, const uint8_t* data, uint8_t len);
    bool(*read)(uint32_t* id, uint8_t* data, uint8_t* len);
    uint32_t(*now_ms)(void);
    void(*delay_ms)(uint32_t ms);
} BusMotorPortOps;

/**
 * @brief 电机初始化配置
 * @param ops 底层端口函数表
 * @param feedback_timeout_ms 等待反馈超时时间，单位 ms
 */
typedef struct {
    const BusMotorPortOps* ops;
    uint32_t feedback_timeout_ms;
} BusMotorConfig;

/**
 * @brief 电机统一接口表，不同电机实例提供同一组接口
 */
typedef struct {
    BusMotorStatus(*init)(const BusMotorConfig* config);
    const char* (*status_str)(BusMotorStatus status);
    const char* (*mode_str)(BusMotorMode mode);
    BusMotorStatus(*enable)(uint16_t id);
    BusMotorStatus(*disable)(uint16_t id);
    BusMotorStatus(*set_mit)(uint16_t id, float pos_rad, float spd_rad_s, float kp, float kd, float torque_a);
    BusMotorStatus(*set_pos_spd)(uint16_t id, float pos_rad, float spd_rad_s);
    BusMotorStatus(*set_spd)(uint16_t id, float spd_rad_s);
    BusMotorStatus(*set_pos_spd_cur)(uint16_t id, float pos_rad, float spd_rad_s, float cur_a);
    BusMotorStatus(*request_feedback)(uint16_t id);
    BusMotorStatus(*get_feedback)(uint16_t id, BusMotorFeedback* feedback);
    BusMotorStatus(*update)(BusMotorFeedback* feedback);
} BusMotorInterface;

extern const BusMotorInterface* bus_motor_instance;

// ! ========================= 接 口 函 数 声 明 ========================= ! //

BusMotorStatus bus_motor_set_instance(const BusMotorInterface* instance);
BusMotorStatus bus_motor_init(const BusMotorConfig* config);
const char* bus_motor_status_str(BusMotorStatus status);
const char* bus_motor_mode_str(BusMotorMode mode);
BusMotorStatus bus_motor_enable(uint16_t id);
BusMotorStatus bus_motor_disable(uint16_t id);
BusMotorStatus bus_motor_set_mit(uint16_t id, float pos_rad, float spd_rad_s, float kp, float kd, float torque_a);
BusMotorStatus bus_motor_set_pos_spd(uint16_t id, float pos_rad, float spd_rad_s);
BusMotorStatus bus_motor_set_spd(uint16_t id, float spd_rad_s);
BusMotorStatus bus_motor_set_pos_spd_cur(uint16_t id, float pos_rad, float spd_rad_s, float cur_a);
BusMotorStatus bus_motor_request_feedback(uint16_t id);
BusMotorStatus bus_motor_get_feedback(uint16_t id, BusMotorFeedback* feedback);
BusMotorStatus bus_motor_update(BusMotorFeedback* feedback);

#endif
