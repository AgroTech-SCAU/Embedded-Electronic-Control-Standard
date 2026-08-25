#include "motor_test.h"

#include "bus_motor/dm_motor.h"
#include "delay.h"
#include "log.h"
#include "main.h"
#include "rgb_led/rgb_led.h"

#define MOTOR_TEST_TARGET 3.14f
#define MOTOR_TEST_BUTTON_DEBOUNCE_MS 50u
#define MOTOR_TEST_COMMAND_PERIOD_MS 10u
#define MOTOR_TEST_LOG_PERIOD_MS 100u

/**
 * 电机测试模式只描述测试意图 具体控制协议由设备层适配
 */
typedef enum {
    MOTOR_TEST_MODE_DISABLED = 0u,
    MOTOR_TEST_MODE_POS_VEL,
    MOTOR_TEST_MODE_VEL,
} MotorTestMode;

/** 电机测试模式在主循环中提交 任何切换失败都会回到失能状态 */
static MotorTestMode s_test_mode = MOTOR_TEST_MODE_DISABLED;
/** PA15 的最近一次原始按键采样结果 */
static bool s_button_raw_pressed = false;
/** PA15 经过去抖后确认的按键状态 */
static bool s_button_stable_pressed = false;
/** 原始按键电平最近一次变化的时间戳 */
static uint32_t s_button_change_ms = 0u;
/** 最近一次运动指令发送的时间戳 */
static uint32_t s_command_ms = 0u;
/** 最近一次反馈日志输出的时间戳 */
static uint32_t s_log_ms = 0u;
/** 中断写入且主循环读取的完整反馈快照 */
static BusMotorFeedback s_latest_feedback;
/** 是否至少收到过一帧属于本业务电机的反馈 */
static volatile bool s_feedback_valid = false;
/** 有效反馈累计次数用于观察链路活性 */
static volatile uint32_t s_feedback_count = 0u;
/** 中断记录的新故障等待主循环执行安全处理 */
static volatile bool s_motor_fault_pending = false;
/** 最近一次被识别为故障的驱动错误码 */
static volatile uint8_t s_motor_fault_code = 0u;
/** 故障锁存后必须先通过一次按键清错 */
static bool s_fault_latched = false;

static bool motor_test_set_mode(MotorTestMode mode);
static bool motor_test_recover_fault(void);
static BusMotorStatus motor_test_send_command(void);
static void motor_test_update_rgb(void);
static void motor_test_log_feedback(void);
static const char* motor_test_mode_str(MotorTestMode mode);

void motor_test_on_feedback_from_isr(const BusMotorFeedback* feedback) {
    /* 此函数运行在 FDCAN 中断上下文 因此只复制数据并记录事件 */
    if(feedback == 0 || feedback->id != (BusMotorId)MOTOR_TEST_ID_MAIN) {
        return;
    }

    s_latest_feedback = *feedback;
    s_feedback_count++;
    s_feedback_valid = true;

    if(feedback->error_code >= 0x08u && feedback->error_code <= 0x0Eu) {
        s_motor_fault_code = feedback->error_code;
        s_motor_fault_pending = true;
    }
}

bool motor_test_init(void) {
    BusMotorStatus status;
    uint32_t now_ms;

    /* 业务初始化先要求驱动进入失能状态 再清除上次运行遗留的故障 */
    status = bus_motor.basic.disable((BusMotorId)MOTOR_TEST_ID_MAIN);
    if(status != MOTOR_STATUS_OK) {
        log_error("MOTOR test init disable failed: logical_id=%u status=%s",
                  (unsigned int)MOTOR_TEST_ID_MAIN,
                  bus_motor.status_str(status));
        return false;
    }

    delay_ms(20u);
    status = dm_motor_clear_error((BusMotorId)MOTOR_TEST_ID_MAIN);
    if(status != MOTOR_STATUS_OK) {
        log_error("MOTOR test init clear failed: logical_id=%u status=%s",
                  (unsigned int)MOTOR_TEST_ID_MAIN,
                  bus_motor.status_str(status));
        return false;
    }

    /* 在临界区外读取硬件输入和时间基准 避免延长 FDCAN 中断的关闭时间 */
    s_button_raw_pressed = HAL_GPIO_ReadPin(USER_KEY_GPIO_Port, USER_KEY_Pin) == GPIO_PIN_RESET;
    now_ms = HAL_GetTick();

    /* 原子重置全部业务状态 防止上一轮的反馈或故障在重初始化后被当作新事件 */
    __disable_irq();
    s_test_mode = MOTOR_TEST_MODE_DISABLED;
    s_button_stable_pressed = s_button_raw_pressed;
    s_button_change_ms = now_ms;
    s_command_ms = now_ms;
    s_log_ms = now_ms;
    s_latest_feedback = (BusMotorFeedback){0};
    s_feedback_valid = false;
    s_feedback_count = 0u;
    s_motor_fault_pending = false;
    s_motor_fault_code = 0u;
    s_fault_latched = false;
    __enable_irq();
    motor_test_update_rgb();

    log_info("MOTOR test ready: logical_id=%u mode=%s",
             (unsigned int)MOTOR_TEST_ID_MAIN,
             motor_test_mode_str(s_test_mode));
    return true;
}

void motor_test_process(void) {
    bool pressed;
    uint32_t now_ms = HAL_GetTick();

    /* 中断只置位故障 主循环在临界区取走事件后执行可能耗时的失能与日志 */
    if(s_motor_fault_pending) {
        uint8_t fault_code;

        __disable_irq();
        fault_code = s_motor_fault_code;
        s_motor_fault_pending = false;
        __enable_irq();
        s_fault_latched = true;
        (void)motor_test_set_mode(MOTOR_TEST_MODE_DISABLED);
        log_error("MOTOR fault latched: code=0x%02X; press USER_KEY to clear", fault_code);
        return;
    }

    /* PA15 使用上拉输入 低电平表示用户按下按键 */
    pressed = HAL_GPIO_ReadPin(USER_KEY_GPIO_Port, USER_KEY_Pin) == GPIO_PIN_RESET;
    if(pressed != s_button_raw_pressed) {
        s_button_raw_pressed = pressed;
        s_button_change_ms = now_ms;
    }
    /* 原始电平连续稳定后才确认状态 仅按下沿触发业务动作 */
    if(pressed != s_button_stable_pressed &&
       (now_ms - s_button_change_ms) >= MOTOR_TEST_BUTTON_DEBOUNCE_MS) {
        s_button_stable_pressed = pressed;
        if(pressed) {
            if(s_fault_latched) {
                (void)motor_test_recover_fault();
            }
            else {
                MotorTestMode next_mode = s_test_mode == MOTOR_TEST_MODE_VEL
                                              ? MOTOR_TEST_MODE_DISABLED
                                              : (MotorTestMode)(s_test_mode + 1u);
                (void)motor_test_set_mode(next_mode);
            }
        }
    }

    /* 运动模式按固定周期重发目标 防止控制指令丢失造成链路中断 */
    if(s_test_mode != MOTOR_TEST_MODE_DISABLED &&
       (now_ms - s_command_ms) >= MOTOR_TEST_COMMAND_PERIOD_MS) {
        BusMotorStatus status;

        s_command_ms = now_ms;
        status = motor_test_send_command();
        if(status != MOTOR_STATUS_OK) {
            log_error("MOTOR command failed: logical_id=%u mode=%s status=%s",
                      (unsigned int)MOTOR_TEST_ID_MAIN,
                      motor_test_mode_str(s_test_mode),
                      bus_motor.status_str(status));
            (void)motor_test_set_mode(MOTOR_TEST_MODE_DISABLED);
        }
    }

    /* 即使尚未收到反馈也周期输出状态 便于辨别控制和接收链路问题 */
    if((now_ms - s_log_ms) >= MOTOR_TEST_LOG_PERIOD_MS) {
        s_log_ms = now_ms;
        motor_test_log_feedback();
    }
}

static bool motor_test_set_mode(MotorTestMode mode) {
    BusMotorProfile profile;
    BusMotorStatus status;
    BusMotorStatus cleanup_status;

    /* 每次切换先提交本地失能并发送失能请求 让失败路径拥有唯一安全落点 */
    s_test_mode = MOTOR_TEST_MODE_DISABLED;
    motor_test_update_rgb();
    status = bus_motor.basic.disable((BusMotorId)MOTOR_TEST_ID_MAIN);
    if(status != MOTOR_STATUS_OK) {
        s_fault_latched = true;
        log_error("MOTOR disable request failed: status=%s; motor safety unconfirmed and fault latched",
                  bus_motor.status_str(status));
        return false;
    }

    if(mode == MOTOR_TEST_MODE_DISABLED) {
        log_info("MOTOR mode=%s", motor_test_mode_str(s_test_mode));
        return true;
    }

    /* 位置速度和纯速度测试分别选择匹配的通用控制 profile */
    profile = mode == MOTOR_TEST_MODE_POS_VEL
                  ? BUS_MOTOR_PROFILE_POSITION
                  : BUS_MOTOR_PROFILE_VELOCITY;
    status = bus_motor.profile.activate((BusMotorId)MOTOR_TEST_ID_MAIN, profile);
    if(status == MOTOR_STATUS_OK) {
        status = bus_motor.basic.enable((BusMotorId)MOTOR_TEST_ID_MAIN);
    }
    if(status == MOTOR_STATUS_OK) {
        s_test_mode = mode;
        status = motor_test_send_command();
    }
    if(status != MOTOR_STATUS_OK) {
        cleanup_status = bus_motor.basic.disable((BusMotorId)MOTOR_TEST_ID_MAIN);
        s_test_mode = MOTOR_TEST_MODE_DISABLED;
        motor_test_update_rgb();
        if(cleanup_status != MOTOR_STATUS_OK) {
            s_fault_latched = true;
            log_error("MOTOR mode switch unsafe: target=%s status=%s cleanup_disable=%s; fault latched",
                      motor_test_mode_str(mode),
                      bus_motor.status_str(status),
                      bus_motor.status_str(cleanup_status));
            return false;
        }
        log_error("MOTOR mode switch failed: target=%s status=%s",
                  motor_test_mode_str(mode),
                  bus_motor.status_str(status));
        return false;
    }

    /* 首帧成功后重置命令计时 防止同一主循环重复发送目标 */
    s_command_ms = HAL_GetTick();
    motor_test_update_rgb();
    log_info("MOTOR mode=%s target_pos_mrad=%ld target_vel_mrad_s=%ld",
             motor_test_mode_str(s_test_mode),
             (long)(mode == MOTOR_TEST_MODE_POS_VEL ? 3140 : 0),
             3140L);
    return true;
}

static bool motor_test_recover_fault(void) {
    BusMotorStatus status;

    /* 清错期间保持本地失能与红灯 并在临界区丢弃尚未消费的旧故障事件 */
    s_test_mode = MOTOR_TEST_MODE_DISABLED;
    motor_test_update_rgb();
    __disable_irq();
    s_motor_fault_pending = false;
    __enable_irq();
    status = bus_motor.basic.disable((BusMotorId)MOTOR_TEST_ID_MAIN);
    if(status == MOTOR_STATUS_OK) {
        delay_ms(20u);
        status = dm_motor_clear_error((BusMotorId)MOTOR_TEST_ID_MAIN);
    }
    if(status != MOTOR_STATUS_OK) {
        log_error("MOTOR fault clear failed: %s", bus_motor.status_str(status));
        return false;
    }

    s_fault_latched = false;
    log_info("MOTOR fault cleared; mode=DISABLED, press USER_KEY again to enter POS_VEL");
    return true;
}

static BusMotorStatus motor_test_send_command(void) {
    /* 失能模式不发送运动目标 其余模式均使用同一业务逻辑标识 */
    switch(s_test_mode) {
        case MOTOR_TEST_MODE_POS_VEL:
            return dm_motor_set_pos_vel((BusMotorId)MOTOR_TEST_ID_MAIN,
                                        MOTOR_TEST_TARGET,
                                        MOTOR_TEST_TARGET);
        case MOTOR_TEST_MODE_VEL:
            return bus_motor.vel((BusMotorId)MOTOR_TEST_ID_MAIN, MOTOR_TEST_TARGET);
        case MOTOR_TEST_MODE_DISABLED:
        default:
            return MOTOR_STATUS_OK;
    }
}

static void motor_test_update_rgb(void) {
    uint8_t r = 0u;
    uint8_t g = 0u;
    uint8_t b = 0u;

    /* 低亮度红蓝绿分别表示失能 位置速度和纯速度测试模式 */
    switch(s_test_mode) {
        case MOTOR_TEST_MODE_DISABLED:
            r = 32u;
            break;
        case MOTOR_TEST_MODE_POS_VEL:
            b = 32u;
            break;
        case MOTOR_TEST_MODE_VEL:
            g = 32u;
            break;
        default:
            r = 32u;
            break;
    }

    if(rgb_led.fill(r, g, b) == RGB_LED_STATUS_OK) {
        (void)rgb_led.show();
    }
}

static void motor_test_log_feedback(void) {
    BusMotorFeedback feedback;
    bool valid;
    uint32_t count;

    /* 从中断共享状态复制一致快照后立即恢复中断 日志格式化不阻塞反馈接收 */
    __disable_irq();
    feedback = s_latest_feedback;
    valid = s_feedback_valid;
    count = s_feedback_count;
    __enable_irq();

    if(!valid) {
        log_info("MOTOR feedback: mode=%s waiting rx=%lu",
                 motor_test_mode_str(s_test_mode),
                 (unsigned long)count);
        return;
    }

    /* 用整数工程单位输出反馈 兼容未启用浮点格式化的精简 printf */
    log_info("MOTOR feedback: mode=%s rx=%lu pos_mrad=%ld vel_mrad_s=%ld tor_mNm=%ld temp_dC=%ld err=0x%02X",
             motor_test_mode_str(s_test_mode),
             (unsigned long)count,
             (long)(feedback.position * 1000.0f),
             (long)(feedback.velocity * 1000.0f),
             (long)(feedback.torque * 1000.0f),
             (long)(feedback.temperature.motor * 10.0f),
             feedback.error_code);
}

static const char* motor_test_mode_str(MotorTestMode mode) {
    /* 日志只返回静态文本 未知模式也明确显示为 UNKNOWN */
    switch(mode) {
        case MOTOR_TEST_MODE_DISABLED:
            return "DISABLED";
        case MOTOR_TEST_MODE_POS_VEL:
            return "POS_VEL";
        case MOTOR_TEST_MODE_VEL:
            return "VEL";
        default:
            return "UNKNOWN";
    }
}
