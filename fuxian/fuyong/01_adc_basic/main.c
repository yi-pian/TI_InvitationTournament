/* 工程：01_adc_basic。最小教学流程：初始化 → 读取一次 ADC code。 */
#include <stdint.h>

#include "ti_msp_dl_config.h"

#define SAMPLE_COUNT (1U)

/* 单次 ADC 结果；uint16_t code，不是物理电压，由 ReadADCOnce() 写入。 */
static volatile uint16_t adc_samples[SAMPLE_COUNT];

/* ============================================================
 * [COPY START: ADC_BASIC]
 * 函数：ReadADCOnce
 * [功能] 软件触发 ADC0 的一次转换，并读出 MEM0 的 uint16_t 原始 ADC code。
 * [输入] PA25/ADC0 的模拟电压；SysConfig 生成的 ADC 实例与 MEM0 宏。
 * [输出] adc_samples[0]：uint16_t ADC code；物理电压需按 VREF/满量程另行换算。
 * [内部步骤] 清 result-loaded 标志 → startConversion → 等待标志 → getMemResult。
 * [为什么清标志] 防止上一次转换的中断状态被误认为当前转换完成。
 * [复用] 需要本工程对应的 SysConfig 宏；换 ADC 实例/通道时不应猜测宏名。
 * ============================================================ */
static void ReadADCOnce(void)
{
    DL_ADC12_clearInterruptStatus(SIGNAL_BASIC_ADC_INST,
        DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
    DL_ADC12_startConversion(SIGNAL_BASIC_ADC_INST);
    while (DL_ADC12_getRawInterruptStatus(SIGNAL_BASIC_ADC_INST,
            DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED) == 0U) {
    }
    adc_samples[0U] = DL_ADC12_getMemResult(SIGNAL_BASIC_ADC_INST,
        SIGNAL_BASIC_ADC_ADCMEM_0);
}
/* [COPY END: ADC_BASIC] */

int main(void)
{
    SYSCFG_DL_init();
    while (true) {
        ReadADCOnce();
        __WFI();
    }
}
