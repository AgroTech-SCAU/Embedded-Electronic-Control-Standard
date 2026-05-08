#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// ! ========================= 变 量 声 明 ========================= ! //

#ifndef LOG_BUFFER_SIZE
#define LOG_BUFFER_SIZE 160u
#endif

#define LOG_COLOR_RED "\x1b[31m"
#define LOG_COLOR_YELLOW "\x1b[33m"
#define LOG_COLOR_BLUE "\x1b[34m"
#define LOG_COLOR_RESET "\x1b[0m"

static const LogPortOps* s_ops;
static LogLevel s_level = LOG_LEVEL_NONE;
static bool s_enable_color;

// ! ========================= 私 有 函 数 声 明 ========================= ! //

static LogStatus log_write_text(const char* text);
static LogStatus log_vwrite(LogLevel level, const char* color, const char* tag, const char* format, va_list args);

// ! ========================= 接 口 函 数 实 现 ========================= ! //

LogStatus log_init(const LogConfig* config) {
    if(config == 0 || config->ops == 0 || config->ops->write == 0) {
        return LOG_STATUS_INVALID_PARAM;
    }

    s_ops = config->ops;
    s_level = config->level;
    s_enable_color = config->enable_color;

    return LOG_STATUS_OK;
}

LogStatus log_set_level(LogLevel level) {
    if(level > LOG_LEVEL_INFO) {
        return LOG_STATUS_INVALID_PARAM;
    }

    s_level = level;
    return LOG_STATUS_OK;
}

LogStatus log_info(const char* format, ...) {
    va_list args;
    LogStatus status;

    va_start(args, format);
    status = log_vwrite(LOG_LEVEL_INFO, LOG_COLOR_BLUE, "[INFO] ", format, args);
    va_end(args);

    return status;
}

LogStatus log_warn(const char* format, ...) {
    va_list args;
    LogStatus status;

    va_start(args, format);
    status = log_vwrite(LOG_LEVEL_WARN, LOG_COLOR_YELLOW, "[WARN] ", format, args);
    va_end(args);

    return status;
}

LogStatus log_error(const char* format, ...) {
    va_list args;
    LogStatus status;

    va_start(args, format);
    status = log_vwrite(LOG_LEVEL_ERROR, LOG_COLOR_RED, "[ERROR] ", format, args);
    va_end(args);

    return status;
}

const char* log_status_str(LogStatus status) {
    switch(status) {
        case LOG_STATUS_OK:
            return "OK";
        case LOG_STATUS_INVALID_PARAM:
            return "INVALID_PARAM";
        case LOG_STATUS_PORT_ERROR:
            return "PORT_ERROR";
        case LOG_STATUS_NOT_INITIALIZED:
            return "NOT_INITIALIZED";
        default:
            return "UNKNOWN";
    }
}

// ! ========================= 私 有 函 数 实 现 ========================= ! //

static LogStatus log_write_text(const char* text) {
    if(s_ops == 0 || s_ops->write == 0) {
        return LOG_STATUS_NOT_INITIALIZED;
    }

    if(s_ops->write(text, (uint32_t)strlen(text)) != 0) {
        return LOG_STATUS_PORT_ERROR;
    }

    return LOG_STATUS_OK;
}

static LogStatus log_vwrite(LogLevel level, const char* color, const char* tag, const char* format, va_list args) {
    char buffer[LOG_BUFFER_SIZE];
    int len;
    LogStatus status;

    if(format == 0) {
        return LOG_STATUS_INVALID_PARAM;
    }

    if(s_ops == 0 || s_ops->write == 0) {
        return LOG_STATUS_NOT_INITIALIZED;
    }

    if(s_level < level) {
        return LOG_STATUS_OK;
    }

    if(s_enable_color) {
        status = log_write_text(color);
        if(status != LOG_STATUS_OK) {
            return status;
        }
    }

    status = log_write_text(tag);
    if(status != LOG_STATUS_OK) {
        return status;
    }

    len = vsnprintf(buffer, sizeof(buffer), format, args);
    if(len < 0) {
        return LOG_STATUS_INVALID_PARAM;
    }

    if((uint32_t)len >= sizeof(buffer)) {
        len = (int)sizeof(buffer) - 1;
    }

    if(s_ops->write(buffer, (uint32_t)len) != 0) {
        return LOG_STATUS_PORT_ERROR;
    }

    if(s_enable_color) {
        status = log_write_text(LOG_COLOR_RESET);
        if(status != LOG_STATUS_OK) {
            return status;
        }
    }

    return log_write_text("\r\n");
}
