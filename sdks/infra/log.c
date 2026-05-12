#include "log.h"

#include <stdarg.h>
#include <stdio.h>

// ! ========================= 变 量 声 明 ========================= ! //

#define LOG_COLOR_RED "\x1b[31m"
#define LOG_COLOR_YELLOW "\x1b[33m"
#define LOG_COLOR_BLUE "\x1b[34m"
#define LOG_COLOR_RESET "\x1b[0m"

static const LogPortOps* s_ops;
static LogLevel s_level = LOG_LEVEL_NONE;
static bool s_enable_color;
static bool s_async_write;
static volatile bool s_write_busy;
static char s_tx_queue[LOG_QUEUE_DEPTH][LOG_BUFFER_SIZE];
static uint32_t s_tx_len_queue[LOG_QUEUE_DEPTH];
static uint32_t s_tx_head;
static uint32_t s_tx_tail;
static uint32_t s_tx_count;

// ! ========================= 私 有 函 数 声 明 ========================= ! //

static LogStatus log_vwrite(LogLevel level, const char* color, const char* tag, const char* format, va_list args);
static uint32_t log_append_text(char* buffer, uint32_t size, uint32_t pos, const char* text);
static LogStatus log_start_async_write(void);

// ! ========================= 接 口 函 数 实 现 ========================= ! //

LogStatus log_init(const LogConfig* config) {
    if(config == 0 || config->ops == 0 || config->ops->write == 0) {
        return LOG_STATUS_INVALID_PARAM;
    }

    s_ops = config->ops;
    s_level = config->level;
    s_enable_color = config->enable_color;
    s_async_write = config->async_write;
    s_write_busy = false;
    s_tx_head = 0u;
    s_tx_tail = 0u;
    s_tx_count = 0u;

    return LOG_STATUS_OK;
}

LogStatus log_set_level(LogLevel level) {
    if(level > LOG_LEVEL_INFO) {
        return LOG_STATUS_INVALID_PARAM;
    }

    s_level = level;
    return LOG_STATUS_OK;
}

void log_write_complete(void) {
    if(s_async_write && s_tx_count > 0u) {
        s_tx_head = (s_tx_head + 1u) % LOG_QUEUE_DEPTH;
        s_tx_count--;
    }

    s_write_busy = false;
    (void)log_start_async_write();
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
        case LOG_STATUS_BUSY:
            return "BUSY";
        default:
            return "UNKNOWN";
    }
}

// ! ========================= 私 有 函 数 实 现 ========================= ! //

static LogStatus log_vwrite(LogLevel level, const char* color, const char* tag, const char* format, va_list args) {
    int len;
    uint32_t pos = 0u;
    uint32_t remain;
    char* tx_buffer;

    if(format == 0) {
        return LOG_STATUS_INVALID_PARAM;
    }

    if(s_ops == 0 || s_ops->write == 0) {
        return LOG_STATUS_NOT_INITIALIZED;
    }

    if(s_level < level) {
        return LOG_STATUS_OK;
    }

    if(s_async_write) {
        if(s_tx_count >= LOG_QUEUE_DEPTH) {
            return LOG_STATUS_BUSY;
        }

        tx_buffer = s_tx_queue[s_tx_tail];
    }
    else {
        tx_buffer = s_tx_queue[0];
    }

    if(s_enable_color) {
        pos = log_append_text(tx_buffer, LOG_BUFFER_SIZE, pos, color);
    }

    pos = log_append_text(tx_buffer, LOG_BUFFER_SIZE, pos, tag);

    if(pos >= LOG_BUFFER_SIZE) {
        pos = LOG_BUFFER_SIZE - 1u;
    }

    remain = LOG_BUFFER_SIZE - pos;
    len = vsnprintf(&tx_buffer[pos], remain, format, args);
    if(len < 0) {
        return LOG_STATUS_INVALID_PARAM;
    }

    if((uint32_t)len >= remain) {
        pos = LOG_BUFFER_SIZE - 1u;
    }
    else {
        pos += (uint32_t)len;
    }

    if(s_enable_color) {
        pos = log_append_text(tx_buffer, LOG_BUFFER_SIZE, pos, LOG_COLOR_RESET);
    }

    pos = log_append_text(tx_buffer, LOG_BUFFER_SIZE, pos, "\r\n");

    if(pos >= LOG_BUFFER_SIZE) {
        pos = LOG_BUFFER_SIZE - 1u;
    }

    if(s_async_write) {
        s_tx_len_queue[s_tx_tail] = pos;
        s_tx_tail = (s_tx_tail + 1u) % LOG_QUEUE_DEPTH;
        s_tx_count++;
        return log_start_async_write();
    }

    if(s_ops->write(tx_buffer, pos) == false) {
        return LOG_STATUS_PORT_ERROR;
    }

    return LOG_STATUS_OK;
}

static uint32_t log_append_text(char* buffer, uint32_t size, uint32_t pos, const char* text) {
    uint32_t i = 0u;

    if(buffer == 0 || size == 0u || text == 0) {
        return pos;
    }

    while(text[i] != '\0' && pos + 1u < size) {
        buffer[pos] = text[i];
        pos++;
        i++;
    }

    if(pos < size) {
        buffer[pos] = '\0';
    }

    return pos;
}

static LogStatus log_start_async_write(void) {
    if(!s_async_write || s_write_busy || s_tx_count == 0u) {
        return LOG_STATUS_OK;
    }

    s_write_busy = true;

    if(s_ops->write(s_tx_queue[s_tx_head], s_tx_len_queue[s_tx_head]) == false) {
        s_write_busy = false;
        return LOG_STATUS_PORT_ERROR;
    }

    return LOG_STATUS_OK;
}
