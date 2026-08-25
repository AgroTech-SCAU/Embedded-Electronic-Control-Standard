#ifndef _dm_motor_core_h_
#define _dm_motor_core_h_

#include "bus_motor/dm_motor.h"

#include <stdbool.h>
#include <stdint.h>

// ! ========================= 接 口 变 量 / Typedef 声 明 ========================= ! //

/**
 * @brief 达妙电机型号参数
 */
typedef struct {
    DmMotorModel model;       /**< 电机型号 */
    DmMotorLimitParam limits; /**< MIT 编解码量程 */
} DmMotorModelSpec;

/**
 * @brief 达妙电机实例注册表
 */
typedef struct {
    DmMotorInstance* instances; /**< assemble 提供的电机实例数组 */
    uint16_t capacity;          /**< 实例数组容量 */
    uint16_t count;             /**< 已注册电机数量 */
} DmMotorRegistry;

// ! ========================= 接 口 函 数 声 明 ========================= ! //

/**
 * @brief 获取指定达妙电机型号参数
 * @param model 电机型号
 * @return 型号参数指针，不支持时返回 0
 */
const DmMotorModelSpec* dm_motor_model_get(DmMotorModel model);

/**
 * @brief 初始化达妙电机注册表
 * @param registry 注册表
 * @param instances assemble 提供的电机实例数组
 * @param capacity 实例数组容量
 * @return 电机状态码
 */
BusMotorStatus dm_motor_registry_init(DmMotorRegistry* registry, DmMotorInstance* instances, uint16_t capacity);

/**
 * @brief 注册达妙电机实例
 * @param registry 注册表
 * @param motor_id 业务逻辑电机 ID
 * @param config 电机实例配置
 * @param instance_out 输出厂家实例编号，可为 0
 * @param instance_ptr_out 输出电机实例指针，可为 0
 * @return 电机状态码
 */
BusMotorStatus dm_motor_registry_register(DmMotorRegistry* registry, BusMotorId motor_id, const DmMotorConfig* config,
                                          uint16_t* instance_out, DmMotorInstance** instance_ptr_out);

/**
 * @brief 注销达妙电机实例
 * @param registry 注册表
 * @param instance 厂家实例编号
 * @return 电机状态码
 */
BusMotorStatus dm_motor_registry_unregister(DmMotorRegistry* registry, uint16_t instance);

/**
 * @brief 按厂家实例编号获取达妙电机实例
 * @param registry 注册表
 * @param instance 厂家实例编号
 * @return 电机实例指针，未找到时返回 0
 */
DmMotorInstance* dm_motor_registry_at(DmMotorRegistry* registry, uint16_t instance);

/**
 * @brief 按厂家实例编号获取只读达妙电机实例
 * @param registry 注册表
 * @param instance 厂家实例编号
 * @return 电机实例指针，未找到时返回 0
 */
const DmMotorInstance* dm_motor_registry_at_const(const DmMotorRegistry* registry, uint16_t instance);

/**
 * @brief 按业务逻辑电机 ID 查找达妙电机实例
 * @param registry 注册表
 * @param motor_id 业务逻辑电机 ID
 * @return 电机实例指针，未找到时返回 0
 */
DmMotorInstance* dm_motor_registry_find_by_motor_id(DmMotorRegistry* registry, BusMotorId motor_id);

/**
 * @brief 按 CAN ID 查找达妙电机实例
 * @param registry 注册表
 * @param can_id 电机 CAN ID
 * @return 电机实例指针，未找到时返回 0
 */
DmMotorInstance* dm_motor_registry_find_by_can_id(DmMotorRegistry* registry, uint16_t can_id);

/**
 * @brief 按反馈帧路由规则查找达妙电机实例
 * @param registry 注册表
 * @param master_id 反馈 CAN ID，非 0 时按 Master ID 查找
 * @param can_id Master ID 为 0 时使用的电机 CAN ID
 * @return 电机实例指针，未找到时返回 0
 */
DmMotorInstance* dm_motor_registry_find_by_feedback(DmMotorRegistry* registry, uint16_t master_id, uint16_t can_id);

/**
 * @brief 按 Master ID 查找达妙电机实例
 * @param registry 注册表
 * @param master_id 电机反馈使用的 Master ID
 * @return 电机实例指针，未找到时返回 0
 */
DmMotorInstance* dm_motor_registry_find_by_master_id(DmMotorRegistry* registry, uint16_t master_id);

/**
 * @brief 将浮点值映射为无符号整数
 * @param value 浮点值
 * @param min 最小值
 * @param max 最大值
 * @param bits 有效位数
 * @return 映射后的无符号整数
 */
uint16_t dm_motor_codec_f32_to_uint(float value, float min, float max, uint8_t bits);

/**
 * @brief 将无符号整数映射为浮点值
 * @param value 无符号整数
 * @param min 最小值
 * @param max 最大值
 * @param bits 有效位数
 * @return 映射后的浮点值
 */
float dm_motor_codec_uint_to_f32(uint16_t value, float min, float max, uint8_t bits);

/**
 * @brief 按小端序编码 float
 * @param out 输出 4 字节缓冲区
 * @param value 浮点值
 */
void dm_motor_codec_pack_f32_le(uint8_t out[4], float value);

/**
 * @brief 按小端序解析 float
 * @param in 输入 4 字节缓冲区
 * @return 浮点值
 */
float dm_motor_codec_unpack_f32_le(const uint8_t in[4]);

/**
 * @brief 编码 MIT 控制帧
 * @param limits 电机运行量程
 * @param position 目标位置
 * @param velocity 目标速度
 * @param kp 位置环比例系数
 * @param kd 位置环微分系数
 * @param torque 前馈扭矩
 * @param out 输出 8 字节缓冲区
 */
void dm_motor_codec_encode_mit(const DmMotorLimitParam* limits, float position, float velocity,
                               float kp, float kd, float torque, uint8_t out[8]);

/**
 * @brief 编码 POS_VEL 控制帧
 * @param position 目标位置
 * @param velocity 目标速度
 * @param out 输出 8 字节缓冲区
 */
void dm_motor_codec_encode_pos_vel(float position, float velocity, uint8_t out[8]);

/**
 * @brief 编码 VEL 控制帧
 * @param velocity 目标速度
 * @param out 输出 4 字节缓冲区
 */
void dm_motor_codec_encode_vel(float velocity, uint8_t out[4]);

/**
 * @brief 编码 POS_FORCE 控制帧
 * @param position 目标位置
 * @param velocity 速度控制值，范围 0-10000
 * @param current 电流控制值，范围 0-10000
 * @param out 输出 8 字节缓冲区
 */
void dm_motor_codec_encode_pos_force(float position, uint16_t velocity, uint16_t current, uint8_t out[8]);

/**
 * @brief 解析达妙电机反馈数据
 * @param limits 电机运行量程
 * @param data 输入 8 字节反馈数据
 * @param position 输出位置
 * @param velocity 输出速度
 * @param torque 输出扭矩
 */
void dm_motor_codec_decode_feedback(const DmMotorLimitParam* limits, const uint8_t data[8],
                                    float* position, float* velocity, float* torque);

#endif
