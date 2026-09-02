#include "signal_oscilloscope.h"

#include <limits.h>

#include "signal_ac_rms.h"
#include "signal_mean.h"
#include "signal_minmax.h"
#include "signal_rms.h"
#include "signal_status_adapter.h"

signal_result_t SignalOscilloscope_Analyze(const float *voltage_v,
    size_t count, signal_oscilloscope_measurements_t *measurements)
{
    signal_mean_result_t mean;
    signal_minmax_result_t minmax;
    signal_rms_result_t rms;
    signal_ac_rms_result_t ac_rms;
    signal_algorithm_status_t status;

    if ((voltage_v == NULL) || (measurements == NULL) ||
        (count == 0U) || (count > UINT32_MAX)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    status = SignalMean_Process(voltage_v, (uint32_t) count, &mean);
    if (status != SIGNAL_ALGORITHM_OK) return SignalStatus_FromAlgorithm(status);
    status = SignalMinMax_Process(voltage_v, (uint32_t) count, &minmax);
    if (status != SIGNAL_ALGORITHM_OK) return SignalStatus_FromAlgorithm(status);
    status = SignalRMS_Process(voltage_v, (uint32_t) count, &rms);
    if (status != SIGNAL_ALGORITHM_OK) return SignalStatus_FromAlgorithm(status);
    status = SignalACRMS_Process(voltage_v, (uint32_t) count, &ac_rms);
    if (status != SIGNAL_ALGORITHM_OK) return SignalStatus_FromAlgorithm(status);

    measurements->minimum_v = minmax.min_value;
    measurements->maximum_v = minmax.max_value;
    measurements->mean_v = mean.mean_value;
    measurements->vpp_v = minmax.max_value - minmax.min_value;
    measurements->total_rms_v = rms.rms_v;
    measurements->ac_rms_v = ac_rms.ac_rms_v;
    return SIGNAL_RESULT_OK;
}

signal_module_status_t SignalOscilloscope_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
