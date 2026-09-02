/* 工程：40_dual_channel_measurement。教学流程：同步双 ADC 采集 → 相位 → 延迟。 */
#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_config.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_dual_adc_phase.h"

/* 同一硬件触发时刻的两路 ADC code；AcquireDualADCFrame() 成功后才可读取。 */
static uint16_t adc_ch1_samples[SIGNAL_SAMPLE_COUNT];
static uint16_t adc_ch2_samples[SIGNAL_SAMPLE_COUNT];
/* 实际同步采样率，Hz；相位算法的采样率参数使用该值。 */
static float sample_rate_hz;
/* 最终相位，deg；MeasurePhase() 成功后有效。 */
static float phase_deg;
/* 由 phase_deg 和已知 reference_frequency_hz 推导的延迟，s。 */
static float delay_s;
/* 本教学工程采用的已知参考频率；模块本身不负责估计该频率。 */
static const float reference_frequency_hz = 1000.0f;
static signal_dual_adc_phase_result_t phase_result;

static const signal_dual_adc_config_t s_adc_config = {
    SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U
};
static const signal_dual_adc_phase_config_t s_phase_config = {
    32U, 128U, 1U, 32U, 128U
};

/* ============================================================
 * 函数：AcquireDualADCFrame
 * [功能] 用公共 Timer 启动两路 ADC/DMA，获得同下标对应同一时刻的采样。
 * [输入] 已初始化的 SignalDualADC 和 SysConfig 公共触发配置。
 * [输出] adc_ch1_samples[]、adc_ch2_samples[]（uint16_t code）及 sample_rate_hz（Hz）。
 * [返回值] true：两路完整且同步；false：DMA 启动失败。
 * [为什么必须同步] 两次独立采样会引入未知时差，不能替代相位测量的同步帧。
 * [复用] 需要 04_dual_adc_dma 的双 ADC/DMA/Timer 配置。
 * ============================================================ */
static bool AcquireDualADCFrame(void)
{
    if (SignalDualADC_Start(adc_ch1_samples, adc_ch2_samples,
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
 * [COPY START: PHASE_MEASURE]
 * 函数：MeasurePhase
 * [功能] 把同步的两路 ADC code 交给现有相位算法，得到 phase_deg。
 * [输入] adc_ch1_samples[]、adc_ch2_samples[]（uint16_t code）、SIGNAL_SAMPLE_COUNT、
 * sample_rate_hz（Hz）。
 * [输出] phase_deg（float，deg）。
 * [内部步骤] SignalDualADCPhase_Process() 使用既有窗口/搜索配置处理同步数据。
 * [返回值] true：phase_deg 有效；false：信号或算法条件不满足。
 * [复用] 需要 AcquireDualADCFrame()、signal_dual_adc_phase 模块和其已有依赖。
 * ============================================================ */
static bool MeasurePhase(void)
{
    if (SignalDualADCPhase_Process(adc_ch1_samples, adc_ch2_samples,
            SIGNAL_SAMPLE_COUNT, (uint32_t)sample_rate_hz, &s_phase_config,
            &phase_result) != SIGNAL_ALGORITHM_OK) {
        return false;
    }
    phase_deg = (float)phase_result.phase_degrees;
    return true;
}
/* [COPY END: PHASE_MEASURE] */

/* ============================================================
 * [COPY START: PHASE_DELAY]
 * 函数：CalculateDelayFromPhase
 * [功能] 根据已测 phase_deg 和外部已知的 reference_frequency_hz 计算时间延迟。
 * [输入] phase_deg（deg）、reference_frequency_hz（Hz）。
 * [输出] delay_s（float，s）。
 * [公式] delay_s = phase_deg / (360 × reference_frequency_hz)。
 * [重要] 相位模块只输出相位；reference_frequency_hz 必须由题目已知值、Timer
 * 或 FFT 测频提供，不能把 delay_s 伪装成相位模块直接输出。
 * [复用] 需在调用前确认 reference_frequency_hz 非零且物理定义一致。
 * ============================================================ */
static bool CalculateDelayFromPhase(void)
{
    if (reference_frequency_hz <= 0.0f) {
        return false;
    }
    delay_s = phase_deg / (360.0f * reference_frequency_hz);
    return true;
}
/* [COPY END: PHASE_DELAY] */

int main(void)
{
    SYSCFG_DL_init();
    if (SignalDualADC_Init(&s_adc_config) != SIGNAL_RESULT_OK) {
        while (true) {
        }
    }

    while (true) {
        if (!AcquireDualADCFrame()) {
            continue;
        }
        if (!MeasurePhase()) {
            continue;
        }
        (void)CalculateDelayFromPhase();
        /* phase_deg、delay_s 可在此接入 TFT/UART。 */
    }
}
