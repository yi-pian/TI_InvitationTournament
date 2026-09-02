/* 工程：60_precision_measurement。教学流程：采集 → 电压 → 3P/4P 正弦拟合。 */
#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_config.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_sine_fit_3param.h"
#include "signal_sine_fit_4param.h"

/* DMA ADC code、双 ADC 驱动占位接收数组与转换后的电压样本。 */
static uint16_t adc_samples[SIGNAL_SAMPLE_COUNT];
static uint16_t adc_unused_samples[SIGNAL_SAMPLE_COUNT];
static float voltage_samples[SIGNAL_SAMPLE_COUNT];
/* 实际采样率和拟合初值，单位 Hz。initial_frequency_hz 可由 20_fft_analysis 的
 * frequency_hz 直接提供；本教学工程维持原来的 1000 Hz 已知初值。 */
static float sample_rate_hz;
static float initial_frequency_hz = 1000.0f;
/* 正弦拟合输出：frequency_hz/Hz、amplitude_v/V、phase_deg/deg、mean_v/V。 */
static float frequency_hz;
static float amplitude_v;
static float phase_deg;
static float mean_v;
static signal_sine_fit_3param_result_t fit3_result;
static signal_sine_fit_4param_result_t fit4_result;

static const signal_dual_adc_config_t s_adc_config = {
    SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U
};

/* ============================================================
 * 函数：AcquireADCFrame
 * [功能] 采集一帧 ADC code 并读取实际 sample_rate_hz。
 * [输出] adc_samples[]（uint16_t）、sample_rate_hz（Hz）；true 后有效。
 * [复用] 需要 signal_dual_adc_mspm0g3507 与同一同步 ADC SysConfig。
 * ============================================================ */
static bool AcquireADCFrame(void)
{
    if (SignalDualADC_Start(adc_samples, adc_unused_samples,
            SIGNAL_SAMPLE_COUNT) != SIGNAL_RESULT_OK) {
        return false;
    }
    while (!SignalDualADC_IsFinished()) {
        __WFI();
    }
    sample_rate_hz = (float)SignalDualADC_GetConfiguredRate();
    return true;
}

/* ============================================================
 * [COPY START: SINE_FIT_CONVERT]
 * 函数：ConvertADCToVoltage
 * [功能] 把 adc_samples[] 从 uint16_t code 换算为 voltage_samples[]（float/V）。
 * [输入] adc_samples[]；[输出] voltage_samples[]。
 * [复用] 有前端增益、offset 或两点校准时应保留原工程的物理校准步骤。
 * ============================================================ */
static void ConvertADCToVoltage(void)
{
    uint32_t index;

    for (index = 0U; index < SIGNAL_SAMPLE_COUNT; ++index) {
        voltage_samples[index] = (float)adc_samples[index] *
            SIGNAL_ADC_VREF_V / 4095.0f;
    }
}
/* [COPY END: SINE_FIT_CONVERT] */

/* ============================================================
 * [COPY START: SINE_FIT_3PARAM]
 * 函数：RunSineFit3Param
 * [功能] 在已知 initial_frequency_hz 下拟合正弦的幅值、相位和 DC。
 * [输入] voltage_samples[]（V）、initial_frequency_hz（Hz）、sample_rate_hz（Hz）。
 * [输出] amplitude_v（V peak）、phase_deg（deg）、mean_v（V）。
 * [为什么需要初值] 3 参数拟合把频率视为已知；频率可由题目给定或 FFT 粗测获得。
 * [返回值] true：fit3_result 有效；false：拟合失败。
 * [复用] 需要 signal_sine_fit_3param。
 * ============================================================ */
static bool RunSineFit3Param(void)
{
    const signal_sine_fit_3param_config_t config = {
        initial_frequency_hz, sample_rate_hz
    };

    if (SignalSineFit3Param_Process(voltage_samples, SIGNAL_SAMPLE_COUNT,
            &config, &fit3_result) != SIGNAL_ALGORITHM_OK) {
        return false;
    }
    amplitude_v = fit3_result.amplitude_peak_v;
    phase_deg = fit3_result.phase_deg;
    mean_v = fit3_result.dc_offset_v;
    return true;
}
/* [COPY END: SINE_FIT_3PARAM] */

/* ============================================================
 * [COPY START: SINE_FIT_4PARAM]
 * 函数：RunSineFit4Param
 * [功能] 在 initial_frequency_hz 附近搜索并同时估计 frequency_hz。
 * [输入] voltage_samples[]（V）、initial_frequency_hz（Hz）、sample_rate_hz（Hz）。
 * [输出] frequency_hz（Hz）及 fit4_result；3P 的幅值/相位/DC 保持原行为。
 * [初值来源] 可直接将 20_fft_analysis 的 frequency_hz 赋给 initial_frequency_hz，
 * 再运行本函数作精修；本工程默认 1000 Hz 是既有教学条件。
 * [返回值] true：4P 拟合成功；false：搜索失败。
 * [复用] 需要 signal_sine_fit_4param；搜索带宽/迭代数要按信号稳定度设置。
 * ============================================================ */
static bool RunSineFit4Param(void)
{
    const signal_sine_fit_4param_config_t config = {
        initial_frequency_hz, 10.0f, sample_rate_hz, 12U
    };

    if (SignalSineFit4Param_Process(voltage_samples, SIGNAL_SAMPLE_COUNT,
            &config, &fit4_result) != SIGNAL_ALGORITHM_OK) {
        return false;
    }
    frequency_hz = fit4_result.frequency_hz;
    return true;
}
/* [COPY END: SINE_FIT_4PARAM] */

int main(void)
{
    SYSCFG_DL_init();
    if (SignalDualADC_Init(&s_adc_config) != SIGNAL_RESULT_OK) {
        while (true) {
        }
    }

    while (true) {
        if (!AcquireADCFrame()) {
            continue;
        }
        ConvertADCToVoltage();
        (void)RunSineFit3Param();
        (void)RunSineFit4Param();
    }
}
