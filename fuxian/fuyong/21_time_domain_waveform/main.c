/* 工程：21_time_domain_waveform。教学流程：采集 ADC 一帧 → 换算电压 → TFT 时域折线。 */
#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_config.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_mspm0g3507.h"

#define GRAPH_X (8)
#define GRAPH_Y (48)
#define GRAPH_W (304)
#define GRAPH_H (160)

/* ADC0 DMA 的 uint16_t 原始码；AcquireADCFrame() 成功后可供绘图读取。 */
static uint16_t adc_samples[SIGNAL_SAMPLE_COUNT];
/* 满足已验证双 ADC 驱动接口的第二路 DMA 缓冲，不参与本工程绘图。 */
static uint16_t adc_unused_samples[SIGNAL_SAMPLE_COUNT];
/* 与 adc_samples 同下标的物理电压，float/V；PrepareDisplaySamples() 写入。 */
static float voltage_samples[SIGNAL_SAMPLE_COUNT];
/* 真实采样率 Hz；用于向后续叠加坐标或频率标注扩展，不改变绘图算法。 */
static float sample_rate_hz;
/* ST7789 显示实例；初始化后由 DrawTimeDomainWaveform() 读取。 */
static tft_st7789_t tft;

static const signal_dual_adc_config_t s_adc_config = {
    SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U
};

/* ============================================================
 * 函数：AcquireADCFrame
 * [功能] 等待 DMA 完整写入一帧 ADC 原始码，并更新 sample_rate_hz。
 * [输入] 已初始化 SignalDualADC 硬件。
 * [输出] adc_samples[]（uint16_t code）、sample_rate_hz（Hz）。
 * [返回值] true：一帧有效；false：DMA 启动失败。
 * [复用] 需 signal_dual_adc_mspm0g3507 与对应双 ADC/DMA/Timer SysConfig。
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
 * [COPY START: TIME_DOMAIN_PREPARE]
 * 函数：PrepareDisplaySamples
 * [功能] 把 uint16_t ADC code 换算为 float 物理电压，保留统一数据接口。
 * [输入] adc_samples[]，ADC code，0..4095。
 * [输出] voltage_samples[]，float，V。
 * [为什么] 当前图形 Y 坐标仍沿用原始 code 的满量程映射以保持显示行为；但同步
 * 保存电压数组，使该函数复制到测量/标注工程时不需要再次换算。
 * [复用] 需要 signal_config.h；有校准时应在此处按原工程加入校准。
 * ============================================================ */
static void PrepareDisplaySamples(void)
{
    uint32_t index;

    for (index = 0U; index < SIGNAL_SAMPLE_COUNT; ++index) {
        voltage_samples[index] = (float)adc_samples[index] *
            SIGNAL_ADC_VREF_V / 4095.0f;
    }
}
/* [COPY END: TIME_DOMAIN_PREPARE] */

/* ============================================================
 * [COPY START: TIME_DOMAIN_PLOT]
 * 函数：DrawTimeDomainWaveform
 * [功能] 将一帧 adc_samples[] 映射为 TFT 上的连续折线；不做 FFT 或滤波。
 * [输入] adc_samples[]（uint16_t code）、SIGNAL_SAMPLE_COUNT；tft 已初始化。
 * [输出] GRAPH_X/Y/W/H 范围内的黄色时域折线。
 * [坐标规则]
 * X：第 i 个屏幕列取 i×N/GRAPH_W 对应的样本；N 大于屏宽时完成降采样显示。
 * Y：0 code 映射底部，4095 code 映射顶部，保留原工程满量程显示行为。
 * [为什么不逐点画 N 次] 屏宽只有 GRAPH_W，按列选点可限制刷新时间且不需新缓冲。
 * [复用] 需 adc_samples、tft、GRAPH 宏和 signal_tft_st7789 模块；若输入已是
 * voltage_samples，必须同步重写 Y 映射，不能把 V 当 ADC code。
 * ============================================================ */
static void DrawTimeDomainWaveform(void)
{
    uint32_t column;

    (void)TFT_ST7789_FillRect(&tft, GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H,
        TFT_ST7789_BLACK);
    for (column = 1U; column < (uint32_t)GRAPH_W; ++column) {
        const uint32_t sample0 = (column - 1U) * SIGNAL_SAMPLE_COUNT /
            (uint32_t)GRAPH_W;
        const uint32_t sample1 = column * SIGNAL_SAMPLE_COUNT /
            (uint32_t)GRAPH_W;
        const int32_t y0 = GRAPH_Y + GRAPH_H - 1 -
            (int32_t)((uint32_t)adc_samples[sample0] * (GRAPH_H - 1) / 4095U);
        const int32_t y1 = GRAPH_Y + GRAPH_H - 1 -
            (int32_t)((uint32_t)adc_samples[sample1] * (GRAPH_H - 1) / 4095U);
        (void)TFT_ST7789_DrawLine(&tft, GRAPH_X + (int32_t)column - 1,
            y0, GRAPH_X + (int32_t)column, y1, TFT_ST7789_YELLOW);
    }
}
/* [COPY END: TIME_DOMAIN_PLOT] */

int main(void)
{
    SYSCFG_DL_init();
    if (SignalDualADC_Init(&s_adc_config) != SIGNAL_RESULT_OK ||
        SignalTFTST7789_MSPM0_Init(&tft, TFT_ST7789_ROTATION_270,
            0U, 0U) != TFT_ST7789_OK) {
        while (true) {
        }
    }

    while (true) {
        if (!AcquireADCFrame()) {
            continue;
        }
        PrepareDisplaySamples();
        DrawTimeDomainWaveform();
    }
}
