/**
 * @file log.h
 * @brief 轻量级 printf 风格日志模块
 *
 * 本模块只依赖一个底层写接口 LogPortOps.write，可对接阻塞 UART、UART DMA、
 * RTT、USB CDC、文件或 mock buffer；日志层负责格式化、级别过滤、可选 ANSI
 * 颜色，以及异步输出时的缓冲区生命周期管理
 *
 * 最小同步用法：
 *
 * @code
 * static bool board_log_write(const char* data, uint32_t len) {
 *     return HAL_UART_Transmit(&huart1, (uint8_t*)data, (uint16_t)len, HAL_MAX_DELAY) == HAL_OK;
 * }
 *
 * static const LogPortOps log_ops = {
 *     .write = board_log_write,
 * };
 *
 * void app_init(void) {
 *     LogConfig config = {
 *         .ops = &log_ops,
 *         .level = LOG_LEVEL_INFO,
 *         .enable_color = true,
 *         .async_write = false,
 *     };
 *
 *     log_init(&config);
 *     log_info("system started");
 * }
 * @endcode
 *
 * UART DMA 异步用法：
 *
 * @code
 * static bool board_log_write(const char* data, uint32_t len) {
 *     return HAL_UART_Transmit_DMA(&huart1, (uint8_t*)data, (uint16_t)len) == HAL_OK;
 * }
 *
 * void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
 *     if(huart == &huart1) {
 *         log_write_complete();
 *     }
 * }
 *
 * void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {
 *     if(huart == &huart1) {
 *         log_write_complete();
 *     }
 * }
 *
 * void app_init(void) {
 *     LogConfig config = {
 *         .ops = &log_ops,
 *         .level = LOG_LEVEL_INFO,
 *         .enable_color = true,
 *         .async_write = true,
 *     };
 *
 *     log_init(&config);
 *     log_info("system started");
 * }
 * @endcode
 *
 * 注意：
 * - async_write=false 时，write() 必须在返回前完成对 data 的读取
 * - async_write=true 时，write() 可在返回后继续读取 data，但发送完成或错误后
 *   必须调用 log_write_complete()，否则队列不会继续发送
 * - 异步模式内部带有小队列，连续多条日志会排队发送；队列满时返回 LOG_STATUS_BUSY
 * - LOG_BUFFER_SIZE 控制单条日志最大长度，LOG_QUEUE_DEPTH 控制异步队列深度，
 *   二者可在编译选项中覆盖
 */

#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <stdint.h>

// ! ========================= 接 口 变 量 / Typedef 声 明 ========================= ! //

/**
 * @brief 日志输出缓冲区大小，单位 byte
 */
#define LOG_BUFFER_SIZE 160u

/**
 * @brief 日志异步输出队列深度
 */
#define LOG_QUEUE_DEPTH 4u

#if defined(__GNUC__)
/**
 * @brief 为日志格式化函数启用 printf 风格编译期检查
 */
#define LOG_PRINTF_FORMAT(format_index, first_arg) __attribute__((format(printf, format_index, first_arg)))
#else
#define LOG_PRINTF_FORMAT(format_index, first_arg)
#endif

/**
 * @brief 日志模块状态码
 */
typedef enum {
    /** 操作成功 */
    LOG_STATUS_OK = 0,
    /** 参数为空或参数值不合法 */
    LOG_STATUS_INVALID_PARAM,
    /** 底层输出端口返回错误 */
    LOG_STATUS_PORT_ERROR,
    /** 尚未调用 log_init() 或 PortOps 未绑定 */
    LOG_STATUS_NOT_INITIALIZED,
    /** 异步输出仍在发送上一条日志 */
    LOG_STATUS_BUSY,
} LogStatus;

/**
 * @brief 日志输出级别
 *
 * 数值越大，输出内容越多；例如 LOGGER_LEVEL_WARN 会输出 warn/error，
 * 不会输出 info
 */
typedef enum {
    /** 关闭所有日志输出 */
    LOG_LEVEL_NONE = 0,
    /** 只输出 error */
    LOG_LEVEL_ERROR = 1,
    /** 输出 warn 和 error */
    LOG_LEVEL_WARN = 2,
    /** 输出 info、warn 和 error */
    LOG_LEVEL_INFO = 3,
} LogLevel;

/**
 * @brief 日志底层输出端口函数表
 *
 * 由 service init 绑定到 platform 的 UART、USB CDC、RTT、文件输出或 mock buffer
 */
typedef struct {
    /**
     * @brief 写出一段日志文本
     * @param data 文本缓冲区指针
     * @param len 文本长度，单位 byte
     * @return true 表示成功，false 表示输出失败
     */
    bool (*write)(const char* data, uint32_t len);
} LogPortOps;

/**
 * @brief 日志初始化配置
 */
typedef struct {
    /** 底层输出端口函数表，不能为空 */
    const LogPortOps* ops;
    /** 初始日志级别 */
    LogLevel level;
    /** 是否输出 ANSI 颜色转义序列 */
    bool enable_color;
    /**
     * @brief true 表示 write() 返回后底层仍可能继续读取 data
     *
     * UART DMA 这类异步端口应设为 true，并在发送完成中断中调用 log_write_complete()
     * 阻塞 UART、RTT、mock buffer 这类同步端口保持 false 即可
     */
    bool async_write;
} LogConfig;

// ! ========================= 接 口 函 数 声 明 ========================= ! //

/**
 * @brief 初始化日志模块并绑定输出端口
 * @param config 日志配置，必须提供有效的 LogPortOps.write
 * @return LogStatus 状态码
 */
LogStatus log_init(const LogConfig* config);

/**
 * @brief 修改当前日志输出级别
 * @param level 新日志级别
 * @return LogStatus 状态码
 */
LogStatus log_set_level(LogLevel level);

/**
 * @brief 通知日志模块上一段异步输出已经完成
 *
 * 仅当 LogConfig.async_write 为 true 时需要调用，通常放在 UART DMA Tx complete 回调里
 */
void log_write_complete(void);

/**
 * @brief 输出 info 级别日志
 * @param format printf 风格格式字符串
 * @return LogStatus 状态码
 */
LogStatus log_info(const char* format, ...) LOG_PRINTF_FORMAT(1, 2);

/**
 * @brief 输出 warn 级别日志
 * @param format printf 风格格式字符串
 * @return LogStatus 状态码
 */
LogStatus log_warn(const char* format, ...) LOG_PRINTF_FORMAT(1, 2);

/**
 * @brief 输出 error 级别日志
 * @param format printf 风格格式字符串
 * @return LogStatus 状态码
 */
LogStatus log_error(const char* format, ...) LOG_PRINTF_FORMAT(1, 2);

/**
 * @brief 将日志状态码转换为静态字符串
 * @param status 日志状态码
 * @return const char* 状态码名称
 */
const char* log_status_str(LogStatus status);

#undef LOG_PRINTF_FORMAT

#endif
