#include "bus_motor.h"

#include <string.h>

// ! ========================= 变 量 声 明 ========================= ! //

/**
 * @brief 单台逻辑电机绑定项
 */
typedef struct {
    bool used;                       /**< 绑定项是否有效 */
    BusMotorId id;                   /**< 业务逻辑电机 ID */
    const BusMotorDriver* driver;    /**< 厂家驱动函数表 */
    uint16_t instance;               /**< 厂家驱动内部实例编号 */
    BusMotorProfileMask profiles;    /**< 支持的 Profile 位图 */
    BusMotorProfile current_profile; /**< 当前激活的 Profile */
} BusMotorEntry;

/**
 * @brief 逻辑电机组绑定项
 */
typedef struct {
    bool used;                                     /**< 绑定项是否有效 */
    BusMotorGroupId id;                            /**< 业务逻辑电机组 ID */
    BusMotorId motors[BUS_MOTOR_GROUP_MEMBER_MAX]; /**< 组内逻辑电机 ID */
    uint8_t count;                                 /**< 组内电机数量 */
} BusMotorGroupEntry;

static bool s_is_initialized = false;
static BusMotorEntry s_entries[BUS_MOTOR_MAX_COUNT];
static BusMotorGroupEntry s_groups[BUS_MOTOR_GROUP_MAX_COUNT];

// ! ========================= 私 有 函 数 声 明 ========================= ! //

/**
 * @brief 按逻辑电机 ID 查找绑定项
 * @param id 业务逻辑电机 ID
 * @return 绑定项指针，未找到时返回 0
 */
static BusMotorEntry* bus_motor_find(BusMotorId id);

/**
 * @brief 按逻辑电机组 ID 查找绑定项
 * @param group_id 业务逻辑电机组 ID
 * @return 电机组绑定项指针，未找到时返回 0
 */
static BusMotorGroupEntry* bus_motor_group_find(BusMotorGroupId group_id);

/**
 * @brief 查询命令要求的 Profile
 * @param type 控制命令类型
 * @return 对应 Profile，不要求 Profile 时返回 BUS_MOTOR_PROFILE_NONE
 */
static BusMotorProfile bus_motor_command_profile(BusMotorCommandType type);

/**
 * @brief 判断 Profile 是否为单一有效位
 * @param profile 控制 Profile
 * @return true 表示有效，false 表示无效
 */
static bool bus_motor_profile_is_single(BusMotorProfile profile);

/**
 * @brief 初始化 bus_motor 注册表
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_init_impl(void);

/**
 * @brief 电机状态码转字符串
 * @param status 电机状态码
 * @return 状态字符串
 */
static const char* bus_motor_status_str_impl(BusMotorStatus status);

/**
 * @brief 使能逻辑电机
 * @param id 业务逻辑电机 ID
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_enable_impl(BusMotorId id);

/**
 * @brief 失能逻辑电机
 * @param id 业务逻辑电机 ID
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_disable_impl(BusMotorId id);

/**
 * @brief 停止逻辑电机
 * @param id 业务逻辑电机 ID
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_stop_impl(BusMotorId id);

/**
 * @brief 制动逻辑电机
 * @param id 业务逻辑电机 ID
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_brake_impl(BusMotorId id);

/**
 * @brief 查询逻辑电机是否支持指定 Profile
 * @param id 业务逻辑电机 ID
 * @param profile 控制 Profile
 * @return true 表示支持，false 表示不支持
 */
static bool bus_motor_profile_supports_impl(BusMotorId id, BusMotorProfile profile);

/**
 * @brief 检查逻辑电机所需 Profile
 * @param id 业务逻辑电机 ID
 * @param profiles 所需 Profile 位图
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_profile_require_impl(BusMotorId id, BusMotorProfileMask profiles);

/**
 * @brief 激活逻辑电机 Profile
 * @param id 业务逻辑电机 ID
 * @param profile 控制 Profile
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_profile_activate_impl(BusMotorId id, BusMotorProfile profile);

/**
 * @brief 获取逻辑电机当前 Profile
 * @param id 业务逻辑电机 ID
 * @return 当前 Profile
 */
static BusMotorProfile bus_motor_profile_current_impl(BusMotorId id);

/**
 * @brief 获取逻辑电机完整反馈
 * @param id 业务逻辑电机 ID
 * @param feedback 输出反馈
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_feedback_all_impl(BusMotorId id, BusMotorFeedback* feedback);

/**
 * @brief 获取逻辑电机位置反馈
 * @param id 业务逻辑电机 ID
 * @param position 输出位置
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_feedback_position_impl(BusMotorId id, float* position);

/**
 * @brief 获取逻辑电机速度反馈
 * @param id 业务逻辑电机 ID
 * @param velocity 输出速度
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_feedback_velocity_impl(BusMotorId id, float* velocity);

/**
 * @brief 获取逻辑电机扭矩反馈
 * @param id 业务逻辑电机 ID
 * @param torque 输出扭矩
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_feedback_torque_impl(BusMotorId id, float* torque);

/**
 * @brief 获取逻辑电机温度反馈
 * @param id 业务逻辑电机 ID
 * @param temperature 输出温度
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_feedback_temperature_impl(BusMotorId id, BusMotorTemperature* temperature);

/**
 * @brief 绑定逻辑电机组
 * @param group_id 业务逻辑电机组 ID
 * @param motors 组内逻辑电机 ID 数组
 * @param count 组内电机数量
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_group_bind_impl(BusMotorGroupId group_id, const BusMotorId* motors, uint8_t count);

/**
 * @brief 使能逻辑电机组
 * @param group_id 业务逻辑电机组 ID
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_group_enable_impl(BusMotorGroupId group_id);

/**
 * @brief 失能逻辑电机组
 * @param group_id 业务逻辑电机组 ID
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_group_disable_impl(BusMotorGroupId group_id);

/**
 * @brief 停止逻辑电机组
 * @param group_id 业务逻辑电机组 ID
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_group_stop_impl(BusMotorGroupId group_id);

/**
 * @brief 制动逻辑电机组
 * @param group_id 业务逻辑电机组 ID
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_group_brake_impl(BusMotorGroupId group_id);

/**
 * @brief 激活逻辑电机组 Profile
 * @param group_id 业务逻辑电机组 ID
 * @param profile 控制 Profile
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_group_activate_impl(BusMotorGroupId group_id, BusMotorProfile profile);

/**
 * @brief 发送逻辑电机组控制命令
 * @param group_id 业务逻辑电机组 ID
 * @param commands 控制命令数组
 * @param count 控制命令数量
 * @param policy 电机组控制策略
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_group_cmd_impl(BusMotorGroupId group_id, const BusMotorCommand* commands,
                                               uint8_t count, BusMotorGroupPolicy policy);

/**
 * @brief 获取逻辑电机组反馈
 * @param group_id 业务逻辑电机组 ID
 * @param feedback 输出反馈数组
 * @param count 输出数组容量
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_group_feedback_impl(BusMotorGroupId group_id, BusMotorFeedback* feedback, uint8_t count);

/**
 * @brief 高频位置控制
 * @param id 业务逻辑电机 ID
 * @param position 目标位置，单位 rad
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_pos_impl(BusMotorId id, float position);

/**
 * @brief 高频速度控制
 * @param id 业务逻辑电机 ID
 * @param velocity 目标速度，单位 rad/s
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_vel_impl(BusMotorId id, float velocity);

/**
 * @brief 高频扭矩控制
 * @param id 业务逻辑电机 ID
 * @param torque 目标扭矩，单位 N*m
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_tor_impl(BusMotorId id, float torque);

/**
 * @brief 高频阻抗控制
 * @param id 业务逻辑电机 ID
 * @param command 阻抗控制指令
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_imp_impl(BusMotorId id, const BusMotorImpedanceCommand* command);

/**
 * @brief 发送统一控制命令
 * @param id 业务逻辑电机 ID
 * @param command 控制命令
 * @return 电机状态码
 */
static BusMotorStatus bus_motor_cmd_impl(BusMotorId id, BusMotorCommand command);

// ! ========================= 接 口 函 数 实 现 ========================= ! //

/**
 * @brief 总线电机统一接口单例
 */
const BusMotorInterface bus_motor = {
    .init = bus_motor_init_impl,
    .status_str = bus_motor_status_str_impl,
    .basic = {
        .enable = bus_motor_enable_impl,
        .disable = bus_motor_disable_impl,
        .stop = bus_motor_stop_impl,
        .brake = bus_motor_brake_impl,
    },
    .profile = {
        .supports = bus_motor_profile_supports_impl,
        .require = bus_motor_profile_require_impl,
        .activate = bus_motor_profile_activate_impl,
        .current = bus_motor_profile_current_impl,
    },
    .feedback = {
        .all = bus_motor_feedback_all_impl,
        .position = bus_motor_feedback_position_impl,
        .velocity = bus_motor_feedback_velocity_impl,
        .torque = bus_motor_feedback_torque_impl,
        .temperature = bus_motor_feedback_temperature_impl,
    },
    .group = {
        .bind = bus_motor_group_bind_impl,
        .enable = bus_motor_group_enable_impl,
        .disable = bus_motor_group_disable_impl,
        .stop = bus_motor_group_stop_impl,
        .brake = bus_motor_group_brake_impl,
        .activate = bus_motor_group_activate_impl,
        .cmd = bus_motor_group_cmd_impl,
        .feedback = bus_motor_group_feedback_impl,
    },
    .pos = bus_motor_pos_impl,
    .vel = bus_motor_vel_impl,
    .tor = bus_motor_tor_impl,
    .imp = bus_motor_imp_impl,
    .cmd = bus_motor_cmd_impl,
};

/**
 * @brief 注册厂家电机实例
 */
BusMotorStatus bus_motor_driver_register(BusMotorId id, const BusMotorDriver* driver, uint16_t instance,
                                         BusMotorProfileMask profiles) {
    uint16_t i;

    if(s_is_initialized == false) {
        return MOTOR_STATUS_NOT_INITIALIZE;
    }
    if(driver == 0 || driver->command == 0 || driver->activate == 0 || driver->feedback == 0 || profiles == 0u) {
        return MOTOR_STATUS_INVALID_PARAM;
    }
    if(bus_motor_find(id) != 0) {
        return MOTOR_STATUS_ALREADY_BOUND;
    }

    for(i = 0u; i < BUS_MOTOR_MAX_COUNT; ++i) {
        if(s_entries[i].used == false) {
            s_entries[i].used = true;
            s_entries[i].id = id;
            s_entries[i].driver = driver;
            s_entries[i].instance = instance;
            s_entries[i].profiles = profiles;
            s_entries[i].current_profile = BUS_MOTOR_PROFILE_NONE;
            return MOTOR_STATUS_OK;
        }
    }

    return MOTOR_STATUS_NO_RESOURCE;
}

/**
 * @brief 注销厂家电机实例
 */
BusMotorStatus bus_motor_driver_unregister(BusMotorId id) {
    BusMotorEntry* entry = bus_motor_find(id);

    if(entry == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }

    memset(entry, 0, sizeof(*entry));
    return MOTOR_STATUS_OK;
}

/**
 * @brief 清除公共 Profile 状态
 */
BusMotorStatus bus_motor_driver_reset_profile(BusMotorId id) {
    BusMotorEntry* entry = bus_motor_find(id);

    if(entry == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }

    entry->current_profile = BUS_MOTOR_PROFILE_NONE;
    return MOTOR_STATUS_OK;
}

// ! ========================= 私 有 函 数 实 现 ========================= ! //

/**
 * @brief 按逻辑电机 ID 查找绑定项
 */
static BusMotorEntry* bus_motor_find(BusMotorId id) {
    uint16_t i;

    for(i = 0u; i < BUS_MOTOR_MAX_COUNT; ++i) {
        if(s_entries[i].used && s_entries[i].id == id) {
            return &s_entries[i];
        }
    }

    return 0;
}

/**
 * @brief 按逻辑电机组 ID 查找绑定项
 */
static BusMotorGroupEntry* bus_motor_group_find(BusMotorGroupId group_id) {
    uint8_t i;

    for(i = 0u; i < BUS_MOTOR_GROUP_MAX_COUNT; ++i) {
        if(s_groups[i].used && s_groups[i].id == group_id) {
            return &s_groups[i];
        }
    }

    return 0;
}

/**
 * @brief 查询命令要求的 Profile
 */
static BusMotorProfile bus_motor_command_profile(BusMotorCommandType type) {
    switch(type) {
        case BUS_MOTOR_CMD_POSITION:
            return BUS_MOTOR_PROFILE_POSITION;
        case BUS_MOTOR_CMD_VELOCITY:
            return BUS_MOTOR_PROFILE_VELOCITY;
        case BUS_MOTOR_CMD_TORQUE:
            return BUS_MOTOR_PROFILE_TORQUE;
        case BUS_MOTOR_CMD_IMPEDANCE:
            return BUS_MOTOR_PROFILE_IMPEDANCE;
        case BUS_MOTOR_CMD_CURRENT_Q:
            return BUS_MOTOR_PROFILE_CURRENT_Q;
        case BUS_MOTOR_CMD_VOLTAGE_Q:
            return BUS_MOTOR_PROFILE_VOLTAGE_Q;
        case BUS_MOTOR_CMD_CURRENT_DQ:
            return BUS_MOTOR_PROFILE_CURRENT_DQ;
        case BUS_MOTOR_CMD_VOLTAGE_DQ:
            return BUS_MOTOR_PROFILE_VOLTAGE_DQ;
        case BUS_MOTOR_CMD_ACCELERATION:
            return BUS_MOTOR_PROFILE_ACCELERATION;
        default:
            return BUS_MOTOR_PROFILE_NONE;
    }
}

/**
 * @brief 判断 Profile 是否为单一有效位
 */
static bool bus_motor_profile_is_single(BusMotorProfile profile) {
    uint32_t value = (uint32_t)profile;

    return value != 0u && (value & (value - 1u)) == 0u;
}

/**
 * @brief 初始化 bus_motor 注册表
 */
static BusMotorStatus bus_motor_init_impl(void) {
    memset(s_entries, 0, sizeof(s_entries));
    memset(s_groups, 0, sizeof(s_groups));
    s_is_initialized = true;
    return MOTOR_STATUS_OK;
}

/**
 * @brief 电机状态码转字符串
 */
static const char* bus_motor_status_str_impl(BusMotorStatus status) {
    switch(status) {
#define X(name, value)        \
    case MOTOR_STATUS_##name: \
        return #name;
        MOTOR_STATUS_TABLE
#undef X
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief 使能逻辑电机
 */
static BusMotorStatus bus_motor_enable_impl(BusMotorId id) {
    BusMotorEntry* entry = bus_motor_find(id);

    if(entry == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }
    if(entry->driver->basic == 0) {
        return MOTOR_STATUS_UNSUPPORTED;
    }

    return entry->driver->basic(entry->instance, BUS_MOTOR_BASIC_ENABLE);
}

/**
 * @brief 失能逻辑电机
 */
static BusMotorStatus bus_motor_disable_impl(BusMotorId id) {
    BusMotorEntry* entry = bus_motor_find(id);

    if(entry == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }
    if(entry->driver->basic == 0) {
        return MOTOR_STATUS_UNSUPPORTED;
    }

    return entry->driver->basic(entry->instance, BUS_MOTOR_BASIC_DISABLE);
}

/**
 * @brief 停止逻辑电机
 */
static BusMotorStatus bus_motor_stop_impl(BusMotorId id) {
    BusMotorEntry* entry = bus_motor_find(id);

    if(entry == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }
    if(entry->driver->basic == 0) {
        return MOTOR_STATUS_UNSUPPORTED;
    }

    return entry->driver->basic(entry->instance, BUS_MOTOR_BASIC_STOP);
}

/**
 * @brief 制动逻辑电机
 */
static BusMotorStatus bus_motor_brake_impl(BusMotorId id) {
    BusMotorEntry* entry = bus_motor_find(id);

    if(entry == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }
    if(entry->driver->basic == 0) {
        return MOTOR_STATUS_UNSUPPORTED;
    }

    return entry->driver->basic(entry->instance, BUS_MOTOR_BASIC_BRAKE);
}

/**
 * @brief 查询逻辑电机是否支持指定 Profile
 */
static bool bus_motor_profile_supports_impl(BusMotorId id, BusMotorProfile profile) {
    BusMotorEntry* entry = bus_motor_find(id);

    if(entry == 0 || bus_motor_profile_is_single(profile) == false) {
        return false;
    }

    return (entry->profiles & (BusMotorProfileMask)profile) != 0u;
}

/**
 * @brief 检查逻辑电机所需 Profile
 */
static BusMotorStatus bus_motor_profile_require_impl(BusMotorId id, BusMotorProfileMask profiles) {
    BusMotorEntry* entry = bus_motor_find(id);

    if(entry == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }
    if(profiles == 0u) {
        return MOTOR_STATUS_INVALID_PARAM;
    }
    if((entry->profiles & profiles) != profiles) {
        return MOTOR_STATUS_UNSUPPORTED;
    }

    return MOTOR_STATUS_OK;
}

/**
 * @brief 激活逻辑电机 Profile
 */
static BusMotorStatus bus_motor_profile_activate_impl(BusMotorId id, BusMotorProfile profile) {
    BusMotorEntry* entry = bus_motor_find(id);
    BusMotorStatus status;

    if(entry == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }
    if(bus_motor_profile_is_single(profile) == false) {
        return MOTOR_STATUS_INVALID_PARAM;
    }
    if((entry->profiles & (BusMotorProfileMask)profile) == 0u) {
        return MOTOR_STATUS_UNSUPPORTED;
    }
    if(entry->current_profile == profile) {
        return MOTOR_STATUS_OK;
    }

    status = entry->driver->activate(entry->instance, profile);
    if(status == MOTOR_STATUS_OK) {
        entry->current_profile = profile;
    }

    return status;
}

/**
 * @brief 获取逻辑电机当前 Profile
 */
static BusMotorProfile bus_motor_profile_current_impl(BusMotorId id) {
    BusMotorEntry* entry = bus_motor_find(id);

    if(entry == 0) {
        return BUS_MOTOR_PROFILE_NONE;
    }

    return entry->current_profile;
}

/**
 * @brief 获取逻辑电机完整反馈
 */
static BusMotorStatus bus_motor_feedback_all_impl(BusMotorId id, BusMotorFeedback* feedback) {
    BusMotorEntry* entry = bus_motor_find(id);
    BusMotorStatus status;

    if(entry == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }
    if(feedback == 0) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    status = entry->driver->feedback(entry->instance, feedback);
    if(status == MOTOR_STATUS_OK) {
        feedback->id = id;
    }

    return status;
}

/**
 * @brief 获取逻辑电机位置反馈
 */
static BusMotorStatus bus_motor_feedback_position_impl(BusMotorId id, float* position) {
    BusMotorFeedback feedback;
    BusMotorStatus status;

    if(position == 0) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    status = bus_motor_feedback_all_impl(id, &feedback);
    if(status != MOTOR_STATUS_OK) {
        return status;
    }
    if((feedback.valid & BUS_MOTOR_FEEDBACK_POSITION) == 0u) {
        return MOTOR_STATUS_UNSUPPORTED;
    }

    *position = feedback.position;
    return MOTOR_STATUS_OK;
}

/**
 * @brief 获取逻辑电机速度反馈
 */
static BusMotorStatus bus_motor_feedback_velocity_impl(BusMotorId id, float* velocity) {
    BusMotorFeedback feedback;
    BusMotorStatus status;

    if(velocity == 0) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    status = bus_motor_feedback_all_impl(id, &feedback);
    if(status != MOTOR_STATUS_OK) {
        return status;
    }
    if((feedback.valid & BUS_MOTOR_FEEDBACK_VELOCITY) == 0u) {
        return MOTOR_STATUS_UNSUPPORTED;
    }

    *velocity = feedback.velocity;
    return MOTOR_STATUS_OK;
}

/**
 * @brief 获取逻辑电机扭矩反馈
 */
static BusMotorStatus bus_motor_feedback_torque_impl(BusMotorId id, float* torque) {
    BusMotorFeedback feedback;
    BusMotorStatus status;

    if(torque == 0) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    status = bus_motor_feedback_all_impl(id, &feedback);
    if(status != MOTOR_STATUS_OK) {
        return status;
    }
    if((feedback.valid & BUS_MOTOR_FEEDBACK_TORQUE) == 0u) {
        return MOTOR_STATUS_UNSUPPORTED;
    }

    *torque = feedback.torque;
    return MOTOR_STATUS_OK;
}

/**
 * @brief 获取逻辑电机温度反馈
 */
static BusMotorStatus bus_motor_feedback_temperature_impl(BusMotorId id, BusMotorTemperature* temperature) {
    BusMotorFeedback feedback;
    BusMotorStatus status;

    if(temperature == 0) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    status = bus_motor_feedback_all_impl(id, &feedback);
    if(status != MOTOR_STATUS_OK) {
        return status;
    }
    if((feedback.valid & BUS_MOTOR_FEEDBACK_TEMPERATURE) == 0u) {
        return MOTOR_STATUS_UNSUPPORTED;
    }

    *temperature = feedback.temperature;
    return MOTOR_STATUS_OK;
}

/**
 * @brief 绑定逻辑电机组
 */
static BusMotorStatus bus_motor_group_bind_impl(BusMotorGroupId group_id, const BusMotorId* motors, uint8_t count) {
    BusMotorGroupEntry* group;
    uint8_t i;
    uint8_t j;

    if(s_is_initialized == false) {
        return MOTOR_STATUS_NOT_INITIALIZE;
    }
    if(motors == 0 || count == 0u || count > BUS_MOTOR_GROUP_MEMBER_MAX) {
        return MOTOR_STATUS_INVALID_PARAM;
    }
    if(bus_motor_group_find(group_id) != 0) {
        return MOTOR_STATUS_ALREADY_BOUND;
    }

    for(i = 0u; i < count; ++i) {
        if(bus_motor_find(motors[i]) == 0) {
            return MOTOR_STATUS_NOT_FOUND;
        }
        for(j = 0u; j < i; ++j) {
            if(motors[i] == motors[j]) {
                return MOTOR_STATUS_INVALID_PARAM;
            }
        }
    }

    for(i = 0u; i < BUS_MOTOR_GROUP_MAX_COUNT; ++i) {
        if(s_groups[i].used == false) {
            group = &s_groups[i];
            memset(group, 0, sizeof(*group));
            group->used = true;
            group->id = group_id;
            group->count = count;
            memcpy(group->motors, motors, sizeof(BusMotorId) * count);
            return MOTOR_STATUS_OK;
        }
    }

    return MOTOR_STATUS_NO_RESOURCE;
}

/**
 * @brief 使能逻辑电机组
 */
static BusMotorStatus bus_motor_group_enable_impl(BusMotorGroupId group_id) {
    BusMotorGroupEntry* group = bus_motor_group_find(group_id);
    BusMotorStatus status;
    uint8_t i;

    if(group == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }

    for(i = 0u; i < group->count; ++i) {
        status = bus_motor_enable_impl(group->motors[i]);
        if(status != MOTOR_STATUS_OK) {
            return status;
        }
    }

    return MOTOR_STATUS_OK;
}

/**
 * @brief 失能逻辑电机组
 */
static BusMotorStatus bus_motor_group_disable_impl(BusMotorGroupId group_id) {
    BusMotorGroupEntry* group = bus_motor_group_find(group_id);
    BusMotorStatus status;
    uint8_t i;

    if(group == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }

    for(i = 0u; i < group->count; ++i) {
        status = bus_motor_disable_impl(group->motors[i]);
        if(status != MOTOR_STATUS_OK) {
            return status;
        }
    }

    return MOTOR_STATUS_OK;
}

/**
 * @brief 停止逻辑电机组
 */
static BusMotorStatus bus_motor_group_stop_impl(BusMotorGroupId group_id) {
    BusMotorGroupEntry* group = bus_motor_group_find(group_id);
    BusMotorStatus status;
    uint8_t i;

    if(group == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }

    for(i = 0u; i < group->count; ++i) {
        status = bus_motor_stop_impl(group->motors[i]);
        if(status != MOTOR_STATUS_OK) {
            return status;
        }
    }

    return MOTOR_STATUS_OK;
}

/**
 * @brief 制动逻辑电机组
 */
static BusMotorStatus bus_motor_group_brake_impl(BusMotorGroupId group_id) {
    BusMotorGroupEntry* group = bus_motor_group_find(group_id);
    BusMotorStatus status;
    uint8_t i;

    if(group == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }

    for(i = 0u; i < group->count; ++i) {
        status = bus_motor_brake_impl(group->motors[i]);
        if(status != MOTOR_STATUS_OK) {
            return status;
        }
    }

    return MOTOR_STATUS_OK;
}

/**
 * @brief 激活逻辑电机组 Profile
 */
static BusMotorStatus bus_motor_group_activate_impl(BusMotorGroupId group_id, BusMotorProfile profile) {
    BusMotorGroupEntry* group = bus_motor_group_find(group_id);
    BusMotorStatus status;
    uint8_t i;

    if(group == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }

    for(i = 0u; i < group->count; ++i) {
        status = bus_motor_profile_activate_impl(group->motors[i], profile);
        if(status != MOTOR_STATUS_OK) {
            return status;
        }
    }

    return MOTOR_STATUS_OK;
}

/**
 * @brief 发送逻辑电机组控制命令
 */
static BusMotorStatus bus_motor_group_cmd_impl(BusMotorGroupId group_id, const BusMotorCommand* commands,
                                               uint8_t count, BusMotorGroupPolicy policy) {
    uint16_t instances[BUS_MOTOR_GROUP_MEMBER_MAX];
    BusMotorGroupEntry* group = bus_motor_group_find(group_id);
    BusMotorEntry* entry;
    const BusMotorDriver* driver = 0;
    BusMotorProfile profile;
    BusMotorStatus status;
    bool same_driver = true;
    uint8_t i;

    if(group == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }
    if(commands == 0 || count != group->count) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    for(i = 0u; i < group->count; ++i) {
        entry = bus_motor_find(group->motors[i]);
        if(entry == 0) {
            return MOTOR_STATUS_NOT_FOUND;
        }

        profile = bus_motor_command_profile(commands[i].type);
        if(profile == BUS_MOTOR_PROFILE_NONE) {
            return MOTOR_STATUS_INVALID_PARAM;
        }
        if((entry->profiles & (BusMotorProfileMask)profile) == 0u) {
            return MOTOR_STATUS_UNSUPPORTED;
        }
        if(entry->current_profile != profile) {
            return MOTOR_STATUS_PROFILE_MISMATCH;
        }

        if(i == 0u) {
            driver = entry->driver;
        }
        else if(entry->driver != driver) {
            same_driver = false;
        }
        instances[i] = entry->instance;
    }

    if(same_driver && driver != 0 && driver->group_command != 0) {
        status = driver->group_command(instances, commands, group->count, policy);
        if(status != MOTOR_STATUS_UNSUPPORTED || policy != BUS_MOTOR_GROUP_POLICY_DEFAULT) {
            return status;
        }
    }

    if(policy != BUS_MOTOR_GROUP_POLICY_DEFAULT) {
        return MOTOR_STATUS_UNSUPPORTED;
    }

    for(i = 0u; i < group->count; ++i) {
        status = bus_motor_cmd_impl(group->motors[i], commands[i]);
        if(status != MOTOR_STATUS_OK) {
            return status;
        }
    }

    return MOTOR_STATUS_OK;
}

/**
 * @brief 获取逻辑电机组反馈
 */
static BusMotorStatus bus_motor_group_feedback_impl(BusMotorGroupId group_id, BusMotorFeedback* feedback, uint8_t count) {
    BusMotorGroupEntry* group = bus_motor_group_find(group_id);
    BusMotorStatus status;
    uint8_t i;

    if(group == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }
    if(feedback == 0 || count < group->count) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    for(i = 0u; i < group->count; ++i) {
        status = bus_motor_feedback_all_impl(group->motors[i], &feedback[i]);
        if(status != MOTOR_STATUS_OK) {
            return status;
        }
    }

    return MOTOR_STATUS_OK;
}

/**
 * @brief 高频位置控制
 */
static BusMotorStatus bus_motor_pos_impl(BusMotorId id, float position) {
    return bus_motor_cmd_impl(id, BUS_CMD_POSITION(position));
}

/**
 * @brief 高频速度控制
 */
static BusMotorStatus bus_motor_vel_impl(BusMotorId id, float velocity) {
    return bus_motor_cmd_impl(id, BUS_CMD_VELOCITY(velocity));
}

/**
 * @brief 高频扭矩控制
 */
static BusMotorStatus bus_motor_tor_impl(BusMotorId id, float torque) {
    return bus_motor_cmd_impl(id, BUS_CMD_TORQUE(torque));
}

/**
 * @brief 高频阻抗控制
 */
static BusMotorStatus bus_motor_imp_impl(BusMotorId id, const BusMotorImpedanceCommand* command) {
    BusMotorCommand value;

    if(command == 0) {
        return MOTOR_STATUS_INVALID_PARAM;
    }

    value.type = BUS_MOTOR_CMD_IMPEDANCE;
    value.data.imp = *command;
    return bus_motor_cmd_impl(id, value);
}

/**
 * @brief 发送统一控制命令
 */
static BusMotorStatus bus_motor_cmd_impl(BusMotorId id, BusMotorCommand command) {
    BusMotorEntry* entry = bus_motor_find(id);
    BusMotorProfile profile = bus_motor_command_profile(command.type);

    if(entry == 0) {
        return MOTOR_STATUS_NOT_FOUND;
    }
    if(profile == BUS_MOTOR_PROFILE_NONE) {
        return MOTOR_STATUS_INVALID_PARAM;
    }
    if((entry->profiles & (BusMotorProfileMask)profile) == 0u) {
        return MOTOR_STATUS_UNSUPPORTED;
    }
    if(entry->current_profile != profile) {
        return MOTOR_STATUS_PROFILE_MISMATCH;
    }

    return entry->driver->command(entry->instance, command);
}
