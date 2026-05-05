#ifndef _protocol_parser_h_
#define _protocol_parser_h_

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 原子性操作，需要用户自行实现
 * @param protocol_parser_enter_critical 进入临界区，禁止中断或其他并发访问
 * @param protocol_parser_exit_critical 退出临界区，恢复中断或其他
 */
void protocol_parser_enter_critical(void);
void protocol_parser_exit_critical(void);

// ! ========================= 接 口 变 量 / Typedef 声 明 ========================= ! //

/**
 * @brief 环形缓冲区错误枚举类型
 * @param RING_BUF_SUCCESS: 操作成功
 * @param RING_BUF_ERR_NULL_PTR: 指针为 NULL 错误
 * @param RING_BUF_ERR_IN_USE: 缓冲区正被使用错误
 * @param RING_BUF_ERR_FULL: 缓冲区已满错误
 * @param RING_BUF_ERR_EMPTY: 缓冲区为空错误
 */
typedef enum {
    RING_BUF_SUCCESS = 0,
    RING_BUF_ERR_NULL_PTR,
    RING_BUF_ERR_IN_USE,
    RING_BUF_ERR_FULL,
    RING_BUF_ERR_EMPTY,
} RingBufErrorCode;

/**
 * @brief 帧解析器错误枚举类型
 * @param FRAME_PARSER_SUCCESS: 操作成功
 * @param FRAME_PARSER_PROCESSING: 解析中，尚未完成
 * @param FRAME_PARSER_ERR_HEADER_TOO_SHORT: 帧头过短错误
 * @param FRAME_PARSER_ERR_NULL_PTR: 指针为 NULL 错误
 * @param FRAME_PARSER_ERR_INVALID_STATE: 无效状态错误
 * @param FRAME_PARSER_ERR_BUFFER_FULL: 缓冲区已满错误
 * @param FRAME_PARSER_ERR_LENGTH_EXCEED: 帧长度超过预期错误
 * @param FRAME_PARSER_ERR_NO_FRAME: 没有可用帧错误
 * @param FRAME_PARSER_ERR_CRC_MISMATCH: CRC 校验失败错误
 */
typedef enum {
    FRAME_PARSER_SUCCESS = 0,
    FRAME_PARSER_PROCESSING,
    FRAME_PARSER_ERR_HEADER_TOO_SHORT,
    FRAME_PARSER_ERR_NULL_PTR,
    FRAME_PARSER_ERR_BUF_TOO_SMALL,
    FRAME_PARSER_ERR_INVALID_STATE,
    FRAME_PARSER_ERR_BUFFER_FULL,
    FRAME_PARSER_ERR_LENGTH_EXCEED,
    FRAME_PARSER_ERR_NO_FRAME,
    FRAME_PARSER_ERR_CRC_MISMATCH,
} FrameParserErrorCode;

/**
 * @brief 帧解析器状态枚举类型
 * @param STATE_IDLE: 空闲状态
 * @param STATE_HEADER_MATCHING: 帧头匹配状态
 * @param STATE_READ_LENGTH: 读取帧长度状态
 * @param STATE_READ_PAYLOAD: 读取帧数据状态
 * @param STATE_READ_CRC: 读取 CRC 校验值状态
 * @param STATE_FRAME_COMPLETE: 帧解析完成状态
 */
typedef enum {
    STATE_IDLE = 0,
    STATE_HEADER_MATCHING,
    STATE_READ_LENGTH,
    STATE_READ_PAYLOAD,
    STATE_READ_CRC,
    STATE_FRAME_COMPLETE,
} FrameParserState;

/**
 * @brief 环形缓冲区结构体定义
 */
typedef struct RingBuf RingBuf;
struct RingBuf {
/// public:
    /**
     * @brief 向环形缓冲区写入单个数据
     * @param self 指向 RingBuf 结构体的指针
     * @param data 要写入的数据
     * @return RingBufErrorCode 枚举类型，表示操作结果
     */
    RingBufErrorCode(*write)(RingBuf* const self, const uint8_t data);
    /**
     * @brief 从环形缓冲区读取单个数据
     * @param self 指向 RingBuf 结构体的指针
     * @param data 指向存储读取数据的变量的指针
     * @return RingBufErrorCode 枚举类型，表示操作结果
     */
    RingBufErrorCode(*read)(RingBuf* const self, uint8_t* const data);
    /**
     * @brief 清空环形缓冲区
     * @param self 指向 RingBuf 结构体的指针
     * @return RingBufErrorCode 枚举类型，表示操作结果
     */
    RingBufErrorCode(*clear)(RingBuf* const self);

    /**
     * @brief 检查环形缓冲区是否已满
     * @param self 指向 RingBuf 结构体的指针
     * @return 1 表示已满，0 表示未满，-1 表示错误
     */
    int8_t(*is_full)(RingBuf* const self);
    /**
     * @brief 检查环形缓冲区是否为空
     * @param self 指向 RingBuf 结构体的指针
     * @return 1 表示为空，0 表示非空，-1 表示错误
     */
    int8_t(*is_empty)(RingBuf* const self);

    /**
     * @brief 获取环形缓冲区当前存储的数据量
     * @param self 指向 RingBuf 结构体的指针
     * @return 当前存储的数据量，-1 表示错误
     */
    int(*get_size)(RingBuf* const self);
    /**
     * @brief 获取环形缓冲区的总容量
     * @param self 指向 RingBuf 结构体的指针
     * @return 环形缓冲区的总容量，-1 表示错误
     */
    int(*get_capacity)(RingBuf* const self);

/// private:
    // 缓冲区指针
    uint8_t* _buf_;
    // 缓冲区当前数据量
    uint16_t _size_;
    // 缓冲区总容量
    uint16_t _capacity_;

    // 写入索引
    uint16_t _write_idx_;
    // 读取索引
    uint16_t _read_idx_;

    // 是否允许覆盖旧数据
    uint8_t _overwrite_;
};

/**
 * @brief 帧解析器结构体定义
 */
typedef struct FrameParser FrameParser;
struct FrameParser {
/// public:

    /**
     * @brief 向帧解析器写入单个数据
     * @param self 指向 FrameParser 结构体的指针
     * @param data 要写入的数据
     * @return FrameParserErrorCode 枚举类型，表示操作结果
     */
    FrameParserErrorCode(*write)(FrameParser* const self, const uint8_t data);

    /**
     * @brief 处理帧解析器状态机
     * @param self 指向 FrameParser 结构体的指针
     * @return FrameParserErrorCode 枚举类型，表示操作结果
     */
    FrameParserErrorCode(*process)(FrameParser* const self);

    /**
     * @brief 获取解析完成的帧数据，仅在 process() 返回成功且状态为 STATE_FRAME_COMPLETE 时有效
     * @param self 指向 FrameParser 结构体的指针
     * @param frame_buffer 输出参数，指向存储帧数据的缓冲区指针
     * @param frame_length 输出参数，指向存储帧数据长度的变量指针
     * @return FrameParserErrorCode 枚举类型，表示操作结果
     */
    FrameParserErrorCode(*get_frame)(FrameParser* const self, uint8_t** const frame_buffer, uint16_t* const frame_length);

    /**
     * @brief 标记当前帧已处理完毕，将状态机复位至空闲
     * @param self 指向 FrameParser 结构体的指针
     * @return FrameParserErrorCode 枚举类型，表示操作结果
     */
    FrameParserErrorCode(*finish)(FrameParser* const self);

    /**
     * @brief 完全重置帧解析器，同时清空环形缓冲区
     * @param self 指向 FrameParser 结构体的指针
     * @return FrameParserErrorCode 枚举类型，表示操作结果
     */
    FrameParserErrorCode(*reset)(FrameParser* const self);

/// private:

    // 指向关联的环形缓冲区的指针
    RingBuf* _ring_buf_;
    // 帧解析器当前状态
    FrameParserState _state_;

    // 帧头
    const uint8_t* _header_;
    // 帧头长度，最小为 2 字节
    uint8_t _header_length_;
    // 帧头匹配索引
    uint8_t _header_match_idx_;

    // 预期的帧长度
    uint16_t _expected_length_;
    // 已接收的帧长度
    uint16_t _received_length_;

    // 帧数据缓冲区指针
    uint8_t* _frame_buf_;
    // 帧数据缓冲区总容量
    uint16_t _frame_buf_capacity_;

    // 是否启用 CRC 校验
    bool _crc_enabled_;
    // CRC 校验累加器
    uint16_t _crc_accum_;
    // 已接收的 CRC 校验值
    uint16_t _received_crc_;
};

// ! ========================= 接 口 函 数 声 明 ========================= ! //

RingBufErrorCode RingBufCreate(RingBuf* const self, uint8_t* const buf, const uint16_t capacity, const uint8_t overwrite);
FrameParserErrorCode FrameParserCreate(FrameParser* const self, RingBuf* const ring_buf, const uint8_t* const header, const uint8_t header_length, uint8_t* const frame_buf, const uint16_t frame_buf_capacity, const bool crc_enabled);

#endif
