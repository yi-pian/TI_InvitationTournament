/* 工程：61_lock_in。教学流程：采集 → 电压换算 → 已知频率 I/Q 同步检测。 */
#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_config.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_lock_in.h"

/* ADC DMA 原始码与第二路驱动接收数组。 */
static uint16_t adc_samples[SIGNAL_SAMPLE_COUNT];
static uint16_t adc_unused_samples[SIGNAL_SAMPLE_COUNT];
/* 物理输入电压，float/V；ConvertADCToVoltage() 写入。 */
static float voltage_samples[SIGNAL_SAMPLE_COUNT];
/* 实际采样率 Hz、外部参考频率 Hz、锁相输出幅值 V peak 与相位 deg。 */
static float sample_rate_hz;
static float reference_frequency_hz = 1000.0f;
static float amplitude_v;
static float phase_deg;
static signal_lock_in_result_t lock_in_result;

static const signal_dual_adc_config_t s_adc_config = {
    SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U
};

/* ============================================================
 * 函数：AcquireADCFrame
 * [功能] 获得一帧 adc_samples[]，同时更新实际 sample_rate_hz。
 * [输出] true 后 adc_samples[]（uint16_t code）和 sample_rate_hz（Hz）有效。
 * [复用] 依赖 signal_dual_adc_mspm0g3507 及对应 ADC/DMA SysConfig。
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
 * [COPY START: LOCK_IN_CONVERT]
 * 函数：ConvertADCToVoltage
 * [功能] 将 uint16_t ADC code 转为 voltage_samples[]（float/V）。
 * [输入] adc_samples[]；[输出] voltage_samples[]。
 * [复用] 需要 signal_config；若有校准须在本函数按原题物理含义处理。
 * ============================================================ */
static void ConvertADCToVoltage(void)
{
    uint32_t index;

    for (index = 0U; index < SIGNAL_SAMPLE_COUNT; ++index) {
        voltage_samples[index] = (float)adc_samples[index] *
            SIGNAL_ADC_VREF_V / 4095.0f;
    }
}
/* [COPY END: LOCK_IN_CONVERT] */

/* ============================================================
 * [COPY START: LOCK_IN]
 * 函数：RunLockIn
 * [功能] 以已知 reference_frequency_hz 构造正交参考，对 voltage_samples[] 做 I/Q
 * 同步检测，抑制非参考频率成分。
 * [输入] voltage_samples[]（V）、reference_frequency_hz（Hz）、sample_rate_hz（Hz）。
 * [输出] amplitude_v（V peak）、phase_deg（deg）；lock_in_result 含 I/Q 等细节。
 * [为什么需要参考频率] Lock-In 不负责盲测频，参考必须来自 DDS、题目已知值或 FFT。
 * [返回值] true：幅值/相位有效；false：模块计算失败。
 * [复用] 需要 ConvertADCToVoltage()、signal_lock_in.c/.h 和正确采样率。
 * ============================================================ */
static bool RunLockIn(void)
{
    const signal_lock_in_config_t config = {
        reference_frequency_hz, sample_rate_hz, 0.0f, 1U
    };

    if (SignalLockIn_Process(voltage_samples, SIGNAL_SAMPLE_COUNT, &config,
            &lock_in_result) != SIGNAL_ALGORITHM_OK) {
        return false;
    }
    amplitude_v = lock_in_result.amplitude_peak_v;
    phase_deg = lock_in_result.phase_deg;
    return true;
}
/* [COPY END: LOCK_IN] */

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
        (void)RunLockIn();
    }
}
