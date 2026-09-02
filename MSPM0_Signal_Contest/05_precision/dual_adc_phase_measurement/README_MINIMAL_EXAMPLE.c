/* 双路同步 ADC 相位测量最小示例：输入一帧已经完成 DMA 的 X/Y 原始码。 */
#include <stdint.h>

#include "signal_dual_adc_phase.h"

void dual_adc_phase_measurement_MinimalExample(void)
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

    /* raw_x/raw_y 必须来自同一次同步 ADC DMA，且此时 DMA 已完成。 */
    status = SignalDualADCPhase_Process(
        raw_x, raw_y, 1024U, 500000U, &config, &result);
    if ((status == SIGNAL_ALGORITHM_OK) && (result.valid != 0U))
    {
        /* result.phase_degrees 是 Y 相对 X 的相位，单位为度。 */
    }
}
