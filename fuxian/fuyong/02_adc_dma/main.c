/* 工程：02_adc_dma。教学流程：初始化 ADC/DMA → AcquireADCFrame() → 使用 adc_samples。 */
#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_config.h"
#include "signal_adc_dma.h"

/* DMA 的唯一目的缓冲：uint16_t ADC code，adc_frame_ready 为 true 时才可读取。 */
static uint16_t adc_samples[SAMPLE_COUNT];
/* Timer 整数分频后的实际采样率，float/Hz；AcquireADCFrame() 每帧更新。 */
static float sample_rate_hz = (float)SIGNAL_SAMPLE_RATE_HZ;
/* 一帧完整性标志：AcquireADCFrame() 写入，后续测量/显示读取。 */
static bool adc_frame_ready;
static volatile signal_result_t module_status;
static const signal_adc_dma_config_t s_adc_config = {
    SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U
};

/* ============================================================
 * 函数：InitADC
 * [功能] 按既有 Timer/ADC/DMA 参数初始化采样模块。
 * [输出] true 后可调用 AcquireADCFrame()；false 表示初始化失败。
 * [复用] 需要 signal_adc_dma.c/.h 和匹配 ADC/DMA/Timer SysConfig。
 * ============================================================ */
static bool InitADC(void)
{
    module_status = SignalADC_Init(&s_adc_config);
    return module_status == SIGNAL_RESULT_OK;
}

/* ============================================================
 * [COPY START: ADC_DMA]
 * 函数：AcquireADCFrame
 * [功能] 启动 ADC DMA，等待一帧 adc_samples[]，并读回真实采样率。
 * [输入] adc_samples[]：uint16_t DMA 目的数组；SAMPLE_COUNT：本帧点数。
 * [输出] adc_frame_ready、adc_samples[]、sample_rate_hz（Hz）。
 * [内部步骤] ready=false → SignalADC_Start → WFI 等待 → GetConfiguredTriggerRate。
 * [返回值] true：一帧完整；false：启动失败，数组不可用于算法。
 * [复用] 复制 InitADC()+本函数及 signal_adc_dma；后续 Basic/FFT/绘图都直接使用
 * 同一个 adc_samples[]，无需改变量名。
 * ============================================================ */
static bool AcquireADCFrame(void)
{
    adc_frame_ready = false;
    module_status = SignalADC_Start(adc_samples, SAMPLE_COUNT);
    if (module_status != SIGNAL_RESULT_OK) {
        return false;
    }
    while (!SignalADC_IsFinished()) {
        __WFI();
    }
    sample_rate_hz = (float)SignalADC_GetConfiguredTriggerRate();
    adc_frame_ready = true;
    return true;
}
/* [COPY END: ADC_DMA] */

int main(void)
{
    SYSCFG_DL_init();
    if (!InitADC()) {
        while (true) {
        }
    }
    while (true) {
        if (!AcquireADCFrame()) {
            continue;
        }
        /* adc_samples、sample_rate_hz 现在可接 Basic、FFT 或时域绘图。 */
    }
}
