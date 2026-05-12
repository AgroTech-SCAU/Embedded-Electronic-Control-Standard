#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <stdint.h>

// ! ========================= 接 口 变 量 / Typedef 声 明 ========================= ! //

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
     * @return 0 表示成功，非 0 表示输出失败
     */
    int (*write)(const char* data, uint32_t len);
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
