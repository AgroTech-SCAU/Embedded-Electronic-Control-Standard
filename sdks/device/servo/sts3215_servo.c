#include "sts3215_servo.h"

#define STS3215_HEADER 0xFFu
#define STS3215_INST_PING 0x01u
#define STS3215_INST_READ 0x02u
#define STS3215_INST_WRITE 0x03u
#define STS3215_INST_REG_WRITE 0x04u
#define STS3215_INST_ACTION 0x05u
#define STS3215_INST_SYNC_READ 0x82u
#define STS3215_INST_SYNC_WRITE 0x83u
#define STS3215_INST_RESET 0x0Au
#define STS3215_INST_OFSCAL 0x0Bu

#define STS3215_FRAME_MAX 128u
#define STS3215_POSITION_PACKET_LEN 7u
#define STS3215_FEEDBACK_LEN (STS3215_PRESENT_CURRENT_H - STS3215_PRESENT_POSITION_L + 1u)

/**
 * @brief STS3215 实例运行上下文
 *
 * 保存 service 注入的端口函数、协议超时配置、字节序和最近一次舵机错误码
 */
typedef struct {
    const ServoPortOps* ops;
    uint32_t timeout_ms;
    uint8_t retry_count;
    ServoEndian endian;
    bool initialized;
    uint8_t last_error_code;
} Sts3215Context;

static Sts3215Context ctx;

/**
 * @brief 计算 SCS 协议校验和
 *
 * 校验范围不包含 0xFF 0xFF 帧头，计算方式为按字节求和后取反
 */
static uint8_t checksum(const uint8_t* data, uint16_t len) {
    uint16_t sum = 0u;

    for(uint16_t i = 0u; i < len; i++) {
        sum += (uint8_t)data[i];
    }

    return (uint8_t)(~sum);
}

/**
 * @brief 按当前实例字节序组合 16 位寄存器值
 */
static uint16_t make_u16(uint8_t low, uint8_t high) {
    if(ctx.endian == SERVO_ENDIAN_BIG) {
        return ((uint16_t)low << 8) | high;
    }

    return ((uint16_t)high << 8) | low;
}

/**
 * @brief 按当前实例字节序拆分 16 位寄存器值
 */
static void split_u16(uint16_t value, uint8_t* low, uint8_t* high) {
    if(ctx.endian == SERVO_ENDIAN_BIG) {
        *low = (uint8_t)(value >> 8);
        *high = (uint8_t)(value & 0xFFu);
    }
    else {
        *low = (uint8_t)(value & 0xFFu);
        *high = (uint8_t)(value >> 8);
    }
}

/**
 * @brief 将 STS3215 的方向位编码转换为 C 有符号数
 *
 * 例如位置/速度/电流使用 bit15 表示方向，负载使用 bit10 表示方向
 */
int16_t sts3215_servo_raw_to_signed(uint16_t value, uint8_t sign_bit) {
    uint16_t sign_mask = (uint16_t)(1u << sign_bit);

    if((value & sign_mask) != 0u) {
        return (int16_t)(-(int16_t)(value & (uint16_t)~sign_mask));
    }

    return (int16_t)value;
}

/**
 * @brief 将 C 有符号数转换为 STS3215 的 bit15 方向位编码
 */
uint16_t sts3215_servo_signed_to_raw(int16_t value) {
    uint16_t raw;

    if(value < 0) {
        raw = (uint16_t)(-value);
        raw |= (uint16_t)(1u << 15);
    }
    else {
        raw = (uint16_t)value;
    }

    return raw;
}

/**
 * @brief 检查实例是否完成初始化，并确认必要 PortOps 已注入
 */
static ServoStatus validate_initialized(void) {
    if(!ctx.initialized) {
        return SERVO_STATUS_NOT_INITIALIZE;
    }

    if(ctx.ops == 0 || ctx.ops->write == 0 || ctx.ops->read == 0) {
        return SERVO_STATUS_PORT_ERROR;
    }

    return SERVO_STATUS_OK;
}

/**
 * @brief 在超时时间内读满指定长度
 *
 * 底层 read() 允许一次只返回部分数据，因此这里循环补齐；若到达 timeout_ms
 * 仍未读满则返回 SERVO_STATUS_TIMEOUT
 */
static ServoStatus read_exact(uint8_t* data, uint16_t len) {
    uint16_t offset = 0u;
    uint32_t start = ctx.ops->now_ms();

    while(offset < len) {
        int got = ctx.ops->read(data + offset, (uint16_t)(len - offset));

        if(got > 0) {
            offset = (uint16_t)(offset + (uint16_t)got);
            continue;
        }

        if(ctx.timeout_ms == 0u || (ctx.ops->now_ms() - start) >= ctx.timeout_ms) {
            return SERVO_STATUS_TIMEOUT;
        }
    }

    return SERVO_STATUS_OK;
}

/**
 * @brief 读取并解析一帧 SCS 应答包
 *
 * 流程：
 * 1. 在接收流中寻找连续两个 0xFF 帧头
 * 2. 读取 ID、长度、错误码、参数和校验和
 * 3. 校验 ID、参数长度和 checksum
 */
static ServoStatus read_status_packet(uint8_t expected_id, uint8_t* params, uint8_t params_cap, uint8_t* out_params_len, uint8_t* out_error) {
    uint8_t byte = 0u;
    uint8_t prev = 0u;
    uint32_t start = ctx.ops->now_ms();

    while(1) {
        int got = ctx.ops->read(&byte, 1u);
        if(got == 1) {
            if(prev == STS3215_HEADER && byte == STS3215_HEADER) {
                break;
            }
            prev = byte;
        }

        if(ctx.timeout_ms == 0u || (ctx.ops->now_ms() - start) >= ctx.timeout_ms) {
            return SERVO_STATUS_TIMEOUT;
        }
    }

    uint8_t head[3];
    ServoStatus status = read_exact(head, sizeof(head));
    if(status != SERVO_STATUS_OK) {
        return status;
    }

    uint8_t rx_id = head[0];
    uint8_t len = head[1];
    uint8_t err = head[2];

    if(expected_id != STS3215_BROADCAST_ID && rx_id != expected_id) {
        return SERVO_STATUS_ID_MISMATCH;
    }

    if(len < 2u) {
        return SERVO_STATUS_ERROR;
    }

    uint8_t param_len = (uint8_t)(len - 2u);
    if(param_len > params_cap) {
        return SERVO_STATUS_BUFFER_TOO_SMALL;
    }

    if(param_len > 0u) {
        status = read_exact(params, param_len);
        if(status != SERVO_STATUS_OK) {
            return status;
        }
    }

    uint8_t rx_checksum = 0u;
    status = read_exact(&rx_checksum, 1u);
    if(status != SERVO_STATUS_OK) {
        return status;
    }

    uint8_t check_buf[STS3215_FRAME_MAX];
    if((uint16_t)(3u + param_len) > sizeof(check_buf)) {
        return SERVO_STATUS_BUFFER_TOO_SMALL;
    }

    check_buf[0] = rx_id;
    check_buf[1] = len;
    check_buf[2] = err;
    for(uint8_t i = 0u; i < param_len; i++) {
        check_buf[3u + i] = (uint8_t)params[i];
    }

    if(checksum(check_buf, (uint16_t)(3u + param_len)) != rx_checksum) {
        return SERVO_STATUS_CHECKSUM_ERROR;
    }

    ctx.last_error_code = err;
    if(out_params_len != 0) {
        *out_params_len = param_len;
    }
    if(out_error != 0) {
        *out_error = err;
    }

    return SERVO_STATUS_OK;
}

/**
 * @brief 组装并发送一帧 SCS 指令包
 *
 * need_ack 为 true 时，发送后会等待普通状态应答；广播 ID 不等待应答
 */
static ServoStatus write_packet(uint8_t id, uint8_t instruction, const uint8_t* params, uint8_t params_len, bool need_ack) {
    ServoStatus status = validate_initialized();
    if(status != SERVO_STATUS_OK) {
        return status;
    }

    uint8_t frame[STS3215_FRAME_MAX];
    uint8_t len = (uint8_t)(params_len + 2u);
    uint16_t frame_len = (uint16_t)(params_len + 6u);

    if(frame_len > sizeof(frame)) {
        return SERVO_STATUS_BUFFER_TOO_SMALL;
    }

    frame[0] = STS3215_HEADER;
    frame[1] = STS3215_HEADER;
    frame[2] = id;
    frame[3] = len;
    frame[4] = instruction;
    for(uint8_t i = 0u; i < params_len; i++) {
        frame[5u + i] = params[i];
    }
    frame[5u + params_len] = checksum(frame + 2u, (uint16_t)(3u + params_len));

    if(!ctx.ops->write(frame, frame_len)) {
        return SERVO_STATUS_PORT_ERROR;
    }

    if(ctx.ops->delay_ms != 0) {
        ctx.ops->delay_ms(1u);
    }

    if(need_ack && id != STS3215_BROADCAST_ID) {
        uint8_t error = 0u;
        status = read_status_packet(id, 0, 0u, 0, &error);
        if(status != SERVO_STATUS_OK) {
            return status;
        }
        if(error != 0u) {
            return SERVO_STATUS_ERROR;
        }
    }

    return SERVO_STATUS_OK;
}

/**
 * @brief 读取舵机控制表中的一段连续寄存器
 */
static ServoStatus read_data(uint8_t id, uint8_t addr, uint8_t* data, uint8_t len) {
    uint8_t params[2];
    uint8_t rx_len = 0u;
    uint8_t error = 0u;
    ServoStatus status;

    if(data == 0 || len == 0u) {
        return SERVO_STATUS_INVALID_PARAM;
    }

    params[0] = addr;
    params[1] = len;
    status = write_packet(id, STS3215_INST_READ, params, sizeof(params), false);
    if(status != SERVO_STATUS_OK) {
        return status;
    }

    status = read_status_packet(id, data, len, &rx_len, &error);
    if(status != SERVO_STATUS_OK) {
        return status;
    }
    if(error != 0u) {
        return SERVO_STATUS_ERROR;
    }
    if(rx_len != len) {
        return SERVO_STATUS_BUFFER_TOO_SMALL;
    }

    return SERVO_STATUS_OK;
}

/**
 * @brief 写入舵机控制表中的一段连续寄存器
 *
 * instruction 可为 WRITE 或 REG_WRITE，用于普通写和异步预写复用
 */
static ServoStatus write_data(uint8_t id, uint8_t addr, const uint8_t* data, uint8_t len, uint8_t instruction) {
    uint8_t params[STS3215_FRAME_MAX];

    if(data == 0 || len == 0u) {
        return SERVO_STATUS_INVALID_PARAM;
    }
    if((uint16_t)(len + 1u) > sizeof(params)) {
        return SERVO_STATUS_BUFFER_TOO_SMALL;
    }

    params[0] = addr;
    for(uint8_t i = 0u; i < len; i++) {
        params[1u + i] = data[i];
    }

    return write_packet(id, instruction, params, (uint8_t)(len + 1u), true);
}

/**
 * @brief 初始化 STS3215 实例上下文
 */
static ServoStatus sts3215_init(const ServoConfig* config) {
    if(config == 0 || config->ops == 0 || config->ops->write == 0 || config->ops->read == 0 || config->ops->now_ms == 0) {
        return SERVO_STATUS_INVALID_PARAM;
    }

    ctx.ops = config->ops;
    ctx.timeout_ms = (config->timeout_ms == 0u) ? 20u : config->timeout_ms;
    ctx.retry_count = config->retry_count;
    ctx.endian = config->endian;
    ctx.last_error_code = 0u;
    ctx.initialized = true;

    return SERVO_STATUS_OK;
}

/**
 * @brief STS3215 状态码字符串转换
 */
static const char* sts3215_status_str(ServoStatus status) {
    switch(status) {
#define X(name, value) case SERVO_STATUS_##name: return #name;
        SERVO_STATUS_TABLE
#undef X
        default: return "UNKNOWN";
    }
}

/**
 * @brief 发送 PING 指令，用于检测指定 ID 舵机是否在线
 */
static ServoStatus sts3215_ping(uint8_t id, uint8_t* out_id) {
    uint8_t error = 0u;
    ServoStatus status = write_packet(id, STS3215_INST_PING, 0, 0u, false);
    if(status != SERVO_STATUS_OK) {
        return status;
    }

    status = read_status_packet(id, 0, 0u, 0, &error);
    if(status != SERVO_STATUS_OK) {
        return status;
    }
    if(error != 0u) {
        return SERVO_STATUS_ERROR;
    }
    if(out_id != 0) {
        *out_id = id;
    }

    return SERVO_STATUS_OK;
}

/** @brief 写 1 字节控制表寄存器 */
static ServoStatus sts3215_write_u8(uint8_t id, uint8_t addr, uint8_t value) {
    return write_data(id, addr, &value, 1u, STS3215_INST_WRITE);
}

/** @brief 写 2 字节控制表寄存器 */
static ServoStatus sts3215_write_u16(uint8_t id, uint8_t addr, uint16_t value) {
    uint8_t data[2];

    split_u16(value, &data[0], &data[1]);
    return write_data(id, addr, data, sizeof(data), STS3215_INST_WRITE);
}

/** @brief 读 1 字节控制表寄存器 */
static ServoStatus sts3215_read_u8(uint8_t id, uint8_t addr, uint8_t* value) {
    if(value == 0) {
        return SERVO_STATUS_INVALID_PARAM;
    }

    return read_data(id, addr, value, 1u);
}

/** @brief 读 2 字节控制表寄存器 */
static ServoStatus sts3215_read_u16(uint8_t id, uint8_t addr, uint16_t* value) {
    uint8_t data[2];
    ServoStatus status;

    if(value == 0) {
        return SERVO_STATUS_INVALID_PARAM;
    }

    status = read_data(id, addr, data, sizeof(data));
    if(status != SERVO_STATUS_OK) {
        return status;
    }

    *value = make_u16(data[0], data[1]);
    return SERVO_STATUS_OK;
}

/**
 * @brief 构造 STS3215 位置控制数据区
 *
 * 数据格式从 STS3215_ACC 开始，共 7 字节：
 * ACC + GoalPosition(2) + GoalTime(2) + GoalSpeed(2)
 */
static void build_position_data(uint8_t* data, int16_t position, uint16_t speed, uint8_t acc) {
    uint16_t raw_position = sts3215_servo_signed_to_raw(position);

    data[0] = acc;
    split_u16(raw_position, &data[1], &data[2]);
    split_u16(0u, &data[3], &data[4]);
    split_u16(speed, &data[5], &data[6]);
}

/** @brief 打开舵机扭矩输出 */
static ServoStatus sts3215_enable_torque(uint8_t id) {
    return sts3215_write_u8(id, STS3215_TORQUE_ENABLE, 1u);
}

/** @brief 关闭舵机扭矩输出 */
static ServoStatus sts3215_disable_torque(uint8_t id) {
    return sts3215_write_u8(id, STS3215_TORQUE_ENABLE, 0u);
}

/** @brief 立即写入位置控制命令 */
static ServoStatus sts3215_set_position(uint8_t id, int16_t position, uint16_t speed, uint8_t acc) {
    uint8_t data[STS3215_POSITION_PACKET_LEN];

    build_position_data(data, position, speed, acc);
    return write_data(id, STS3215_ACC, data, sizeof(data), STS3215_INST_WRITE);
}

/** @brief 预写位置控制命令，等待 ACTION 触发 */
static ServoStatus sts3215_reg_set_position(uint8_t id, int16_t position, uint16_t speed, uint8_t acc) {
    uint8_t data[STS3215_POSITION_PACKET_LEN];

    build_position_data(data, position, speed, acc);
    return write_data(id, STS3215_ACC, data, sizeof(data), STS3215_INST_REG_WRITE);
}

/** @brief 触发 REG_WRITE 预写命令 */
static ServoStatus sts3215_action(uint8_t id) {
    return write_packet(id, STS3215_INST_ACTION, 0, 0u, id != STS3215_BROADCAST_ID);
}

/**
 * @brief 同步写多个舵机的位置控制命令
 *
 * speeds 或 accs 允许传 NULL，传 NULL 时对应值按 0 处理
 */
static ServoStatus sts3215_sync_set_position(const uint8_t* ids, uint8_t count, const int16_t* positions, const uint16_t* speeds, const uint8_t* accs) {
    uint8_t params[STS3215_FRAME_MAX];
    uint16_t need_len;

    if(ids == 0 || positions == 0 || count == 0u) {
        return SERVO_STATUS_INVALID_PARAM;
    }

    need_len = (uint16_t)(2u + ((uint16_t)STS3215_POSITION_PACKET_LEN + 1u) * count);
    if(need_len > sizeof(params)) {
        return SERVO_STATUS_BUFFER_TOO_SMALL;
    }

    params[0] = STS3215_ACC;
    params[1] = STS3215_POSITION_PACKET_LEN;
    for(uint8_t i = 0u; i < count; i++) {
        uint8_t* item = &params[2u + i * (STS3215_POSITION_PACKET_LEN + 1u)];
        item[0] = ids[i];
        build_position_data(item + 1u, positions[i], (speeds != 0) ? speeds[i] : 0u, (accs != 0) ? accs[i] : 0u);
    }

    return write_packet(STS3215_BROADCAST_ID, STS3215_INST_SYNC_WRITE, params, (uint8_t)need_len, false);
}

/** @brief 切换为轮式恒速模式 */
static ServoStatus sts3215_set_wheel_mode(uint8_t id) {
    return sts3215_write_u8(id, STS3215_MODE, 1u);
}

/** @brief 轮式模式下设置转速 */
static ServoStatus sts3215_set_speed(uint8_t id, int16_t speed, uint8_t acc) {
    uint8_t data[STS3215_POSITION_PACKET_LEN];
    uint16_t raw_speed = sts3215_servo_signed_to_raw(speed);

    data[0] = acc;
    split_u16(0u, &data[1], &data[2]);
    split_u16(0u, &data[3], &data[4]);
    split_u16(raw_speed, &data[5], &data[6]);

    return write_data(id, STS3215_ACC, data, sizeof(data), STS3215_INST_WRITE);
}

/**
 * @brief 读取 STS3215 常用反馈
 *
 * 从 PRESENT_POSITION_L 到 PRESENT_CURRENT_H 连续读取，并拆出位置、速度、负载、
 * 电压、温度、运动状态和电流
 */
static ServoStatus sts3215_read_feedback(uint8_t id, ServoFeedback* feedback) {
    uint8_t data[STS3215_FEEDBACK_LEN];
    ServoStatus status;

    if(feedback == 0) {
        return SERVO_STATUS_INVALID_PARAM;
    }

    status = read_data(id, STS3215_PRESENT_POSITION_L, data, sizeof(data));
    if(status != SERVO_STATUS_OK) {
        return status;
    }

    feedback->id = id;
    feedback->error_code = ctx.last_error_code;
    feedback->position_raw = sts3215_servo_raw_to_signed(make_u16(data[0], data[1]), 15u);
    feedback->speed_raw = sts3215_servo_raw_to_signed(make_u16(data[2], data[3]), 15u);
    feedback->load_raw = sts3215_servo_raw_to_signed(make_u16(data[4], data[5]), 10u);
    feedback->voltage_raw = data[6];
    feedback->temperature_c = data[7];
    feedback->moving = data[10] != 0u;
    feedback->current_raw = sts3215_servo_raw_to_signed(make_u16(data[13], data[14]), 15u);

    return SERVO_STATUS_OK;
}

/** @brief 仅读取当前位置 */
static ServoStatus sts3215_read_position(uint8_t id, int16_t* position) {
    uint16_t raw_position = 0u;
    ServoStatus status;

    if(position == 0) {
        return SERVO_STATUS_INVALID_PARAM;
    }

    status = sts3215_read_u16(id, STS3215_PRESENT_POSITION_L, &raw_position);
    if(status != SERVO_STATUS_OK) {
        return status;
    }

    *position = sts3215_servo_raw_to_signed(raw_position, 15u);
    return SERVO_STATUS_OK;
}

/**
 * @brief STS3215 对外暴露的 ServoInterface 实例
 */
const ServoInterface sts3215_servo_instance = {
    .init = sts3215_init,
    .status_str = sts3215_status_str,
    .ping = sts3215_ping,
    .enable_torque = sts3215_enable_torque,
    .disable_torque = sts3215_disable_torque,
    .set_position = sts3215_set_position,
    .reg_set_position = sts3215_reg_set_position,
    .action = sts3215_action,
    .sync_set_position = sts3215_sync_set_position,
    .set_wheel_mode = sts3215_set_wheel_mode,
    .set_speed = sts3215_set_speed,
    .read_feedback = sts3215_read_feedback,
    .read_position = sts3215_read_position,
    .write_u8 = sts3215_write_u8,
    .write_u16 = sts3215_write_u16,
    .read_u8 = sts3215_read_u8,
    .read_u16 = sts3215_read_u16,
};
