#ifndef SIGNAL_STATUS_H
#define SIGNAL_STATUS_H

/** @file signal_status.h @brief 全库统一的运行状态、成熟度和返回码。 */

/** 运行时状态：描述一次调用当前进行到哪里。 */
typedef enum {
    MODULE_IDLE = 0,
    MODULE_RUNNING,
    MODULE_DONE,
    MODULE_ERROR
} signal_status_t;

/**
 * 模块成熟度：描述证据等级，不描述运行时状态。
 * 只有真实完成对应验证后才能升级，禁止用“预计可用”代替证据。
 */
typedef enum {
    MODULE_STATUS_DRAFT = 0,
    MODULE_STATUS_BUILD_VERIFIED,
    MODULE_STATUS_BOARD_VERIFIED,
    MODULE_STATUS_CONTEST_VERIFIED
} signal_module_status_t;

/** 所有同步 API 共用的返回码。 */
typedef enum {
    SIGNAL_RESULT_OK = 0,
    SIGNAL_RESULT_INVALID_ARGUMENT,
    SIGNAL_RESULT_BUSY,
    SIGNAL_RESULT_OUT_OF_RANGE,
    SIGNAL_RESULT_NOT_INITIALIZED,
    SIGNAL_RESULT_INSUFFICIENT_BUFFER,
    SIGNAL_RESULT_NO_DATA,
    SIGNAL_RESULT_NUMERIC_ERROR,
    SIGNAL_RESULT_NOT_SUPPORTED,
    SIGNAL_RESULT_HARDWARE_ERROR
} signal_result_t;

#endif /* SIGNAL_STATUS_H */
