#ifndef SIGNAL_STATUS_ADAPTER_H
#define SIGNAL_STATUS_ADAPTER_H

#include "signal_algorithm_status.h"
#include "signal_status.h"

/* Lightweight integration-only adapter between the pure-algorithm status
 * domain and the historical application/peripheral result domain. */
static inline signal_result_t SignalStatus_FromAlgorithm(
    signal_algorithm_status_t status)
{
    switch (status) {
    case SIGNAL_ALGORITHM_OK: return SIGNAL_RESULT_OK;
    case SIGNAL_ALGORITHM_INVALID_ARGUMENT:
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    case SIGNAL_ALGORITHM_INSUFFICIENT_DATA:
    case SIGNAL_ALGORITHM_NO_FEATURE:
        return SIGNAL_RESULT_NO_DATA;
    case SIGNAL_ALGORITHM_OUT_OF_RANGE:
        return SIGNAL_RESULT_OUT_OF_RANGE;
    case SIGNAL_ALGORITHM_BUFFER_TOO_SMALL:
        return SIGNAL_RESULT_INSUFFICIENT_BUFFER;
    case SIGNAL_ALGORITHM_NUMERIC_ERROR:
        return SIGNAL_RESULT_NUMERIC_ERROR;
    case SIGNAL_ALGORITHM_NOT_SUPPORTED:
        return SIGNAL_RESULT_NOT_SUPPORTED;
    default:
        return SIGNAL_RESULT_HARDWARE_ERROR;
    }
}

#endif /* SIGNAL_STATUS_ADAPTER_H */
