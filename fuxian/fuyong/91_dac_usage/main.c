/* 工程：91_dac_usage。固定 DC DAC 输出；连续波形请使用 90_dds_usage。 */
#include <stdint.h>

#include "ti_msp_dl_config.h"

/* 要输出的 12 bit DAC code，范围 0..4095；SetDACDC() 写入硬件。 */
static uint16_t dac_code = 2048U;

/* ============================================================
 * [COPY START: DAC_DC]
 * 函数：SetDACDC
 * [功能] 把 dac_code 写到 DAC0，得到 PA15 的固定直流电压。
 * [输入] dac_code：uint16_t，0..4095；不是 V，电压比例由 DAC VREF 决定。
 * [输出] DAC0/PA15 的直流模拟电压。
 * [为什么单独函数] 直流输出是一次寄存器更新；连续波形需要 DMA，不能在此循环
 * 里手工反复写一个固定 code 来伪造波形。
 * [复用] 需要本工程 DAC0/PA15 SysConfig；不同 DAC 实例必须使用其生成宏。
 * ============================================================ */
static void SetDACDC(void)
{
    DL_DAC12_output12(DAC0, dac_code);
}
/* [COPY END: DAC_DC] */

/* ============================================================
 * [COPY START: DAC_WAVEFORM]
 * 函数：ExplainContinuousWaveformEntry
 * [功能] 明确连续表波的正确入口，不在 DC 工程中新增 DMA 实现。
 * [输入/输出] 无；阅读此函数即可知道需要迁移到 90_dds_usage 的 DAC DMA 模块
 * 与 P03 SysConfig。P07 的固定 DC 配置不含连续输出 DMA。
 * [复用] 若题目需要正弦/方波等连续波，复制 90_dds_usage 的完整 DDS 函数，
 * 不能只复制 SetDACDC()。
 * ============================================================ */
static void ExplainContinuousWaveformEntry(void)
{
    /* 有意为空：它是可复制教学说明的函数边界，不改变原工程 DC 行为。 */
}
/* [COPY END: DAC_WAVEFORM] */

int main(void)
{
    SYSCFG_DL_init();
    SetDACDC();
    ExplainContinuousWaveformEntry();

    while (true) {
        __WFI();
    }
}
