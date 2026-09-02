/* 双路同步 ADC 相位测量完整示例：展示参数、调用、状态和结果读取。 */
#include <stdint.h>

#include "signal_dual_adc_phase.h"

void dual_adc_phase_measurement_FullExample(void)
{
    static uint16_t raw_x[1024U] = {0};
    static uint16_t raw_y[1024U] = {0};
    static const signal_dual_adc_phase_config_t config = {
        .hysteresis_code = 16U,
        .min_amplitude_code = 64U,
        .frequency_ratio = 1U,
        .max_x_crossings = 16U,
        .max_y_crossings = 64U
    };
    signal_dual_adc_phase_result_t result;
    signal_algorithm_status_t status;

    /* 先启动上游双 ADC 同步采集；这里仅表示采集完成后的处理位置。 */
    status = SignalDualADCPhase_Process(
        raw_x, raw_y, 1024U, 500000U, &config, &result);
    if (status != SIGNAL_ALGORITHM_OK)
    {
        return;
    }
    if (result.valid == 0U)
    {
        return;
    }

    /* 可把 phase_degrees 交给 TFT；crossing_count 可用于诊断采样质量。 */
    (void)result.phase_degrees;
    (void)result.valid_phase_count;
    (void)result.x_crossing_count;
    (void)result.y_crossing_count;
}
