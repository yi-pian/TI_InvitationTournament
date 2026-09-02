/* 工程：04_dual_adc_dma。教学流程：InitDualADC() → AcquireDualADCFrame() → 同步两路数组。 */
#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_config.h"
#include "signal_dual_adc_mspm0g3507.h"

/* 两路 DMA 目的数组，uint16_t ADC code；同下标对应公共 Timer 的同一触发时刻。 */
static uint16_t adc_ch1_samples[SAMPLE_COUNT];
static uint16_t adc_ch2_samples[SAMPLE_COUNT];
/* 实际 Timer 采样率，float/Hz；后续 FFT 或 phase 必须使用此值。 */
static float sample_rate_hz = (float)SIGNAL_SAMPLE_RATE_HZ;
static bool adc_frame_ready;
static volatile signal_result_t module_status;
static const signal_dual_adc_config_t s_adc_config = {
    SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U
};

/* ============================================================
 * 函数：InitDualADC
 * [功能] 初始化两路 ADC、两路 DMA 和公共 Timer 触发链。
 * [输出] true 后可同步采集；false 表示模块初始化失败。
 * [复用] 需要 signal_dual_adc_mspm0g3507 与 04 工程已验证 SysConfig。
 * ============================================================ */
static bool InitDualADC(void)
{
    module_status = SignalDualADC_Init(&s_adc_config);
    return module_status == SIGNAL_RESULT_OK;
}

/* ============================================================
 * [COPY START: DUAL_ADC_DMA]
 * 函数：AcquireDualADCFrame
 * [功能] 启动同步双 ADC DMA，等待两路采样帧完成。
 * [输入] adc_ch1_samples/adc_ch2_samples（uint16_t DMA 数组）、SAMPLE_COUNT。
 * [输出] adc_frame_ready、两路数组、sample_rate_hz（Hz）。
 * [为什么同步] Phase/双通道比较要求同下标没有软件调度引入的未知时间偏差。
 * [返回值] true：两路完整；false：本次启动失败。
 * [复用] 需连同 InitDualADC() 复制；新工程应保持两个 ADC、两路 DMA、公共 Timer Event。
 * ============================================================ */
static bool AcquireDualADCFrame(void)
{
    adc_frame_ready = false;
    module_status = SignalDualADC_Start(adc_ch1_samples, adc_ch2_samples,
        SAMPLE_COUNT);
    if (module_status != SIGNAL_RESULT_OK) {
        return false;
    }
    while (!SignalDualADC_IsFinished()) {
        __WFI();
    }
    sample_rate_hz = (float)SignalDualADC_GetConfiguredRate();
    adc_frame_ready = true;
    return true;
}
/* [COPY END: DUAL_ADC_DMA] */

int main(void)
{
    SYSCFG_DL_init();
    if (!InitDualADC()) {
        while (true) {
        }
    }
    while (true) {
        if (!AcquireDualADCFrame()) {
            continue;
        }
        /* adc_ch1_samples、adc_ch2_samples 可接 Phase、FFT 或幅值比较。 */
    }
}
