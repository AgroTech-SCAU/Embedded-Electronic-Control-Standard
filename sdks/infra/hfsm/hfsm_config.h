#ifndef _hfsm_core_config_h_
#define _hfsm_core_config_h_

#include <stdint.h>

/**
 * @brief HFSM 最大状态层级深度
 */
#ifndef HFSM_DEPTH
#define HFSM_DEPTH 8
#endif

/**
 * @brief 状态数量
 */
#ifndef HFSM_MAX_STATES
#define HFSM_MAX_STATES 16
#endif

/**
 * @brief 事件数据类型宏，用户可以按需求自定义并覆盖
 * @note e.g.
 * @note typedef union {
 * @note    void* ptr;
 * @note    const WeedData* weed;
 * @note } MissionEventData;
 * @note #define HFSM_EVENT_DATA_TYPE MissionEventData
 * @note #include "infra/hfsm/hfsm.h"
 */
#ifndef HFSM_EVENT_DATA_TYPE
typedef union {
    void* ptr;
    int32_t i32;
    uint32_t u32;
    float f;
} HfsmEventData;
#define HFSM_EVENT_DATA_TYPE HfsmEventData
#endif

/**
 * @brief 状态机进程是否执行所有祖先状态的动作，1 表示执行，0 表示只执行当前状态的动作
 */
#ifndef HFSM_RUN_PARENT_ACTIONS
#define HFSM_RUN_PARENT_ACTIONS 1
#endif

/**
 * @brief 是否启用断言，1 表示启用，0 表示禁用
 */
#ifndef HFSM_ENABLE_ASSERT
#define HFSM_ENABLE_ASSERT 1
#endif

/**
 * @brief 状态机待处理事件队列的最大长度
 */
#ifndef HFSM_PENDING_QUEUE_MAX
#define HFSM_PENDING_QUEUE_MAX 8
#endif

/**
 * @brief 状态机事件链条最大长度，必须大于等于 HFSM_PENDING_QUEUE_MAX
 */
#ifndef HFSM_MAX_CHAIN_LENGTH
#define HFSM_MAX_CHAIN_LENGTH 2 * HFSM_PENDING_QUEUE_MAX
#endif

#endif
