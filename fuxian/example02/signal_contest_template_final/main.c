/* example02：DDS/DAC 输出扫频，ADC 采样并绘制幅频点/相位结果。
 * DDS、DAC、ADC、phase、ST7789 为 README 模块调用；App_RunSweepPoint、按键、
 * 扫频步进和坐标刷新是 main 的组合逻辑。起止频率和步进改 APP_* 参数。 */
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_config.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_frequency_sweep.h"
#include "signal_matrix_keypad_4x4.h"
#include "signal_dual_adc_phase.h"
#include "signal_dac_dma_mspm0g3507.h"
#include "signal_wave_output_mspm0g3507.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_mspm0g3507.h"
#include "signal_tft_st7789_font.h"

#define SWEEP_POINTS (16U)
#define PLOT_X (8)
#define PLOT_Y (42)
#define PLOT_W (300)
#define PLOT_H (170)
#define ADC_WAIT_LIMIT (2000000UL)

/* g_raw_in/out：输入和输出 ADC 原始帧；g_frequency_hz/g_gain/g_phase：每个扫频点的
 * 结果数组；g_curve/g_point：当前显示曲线和点号；g_previous_*：旧曲线像素；
 * pending_key：SysTick 扫描到、等待主循环处理的按键。 */
/* 变量逐项：g_raw_in 是被测输入，g_raw_out 是 DAC 参考；g_frequency_hz 是频点表；
 * g_gain/g_phase 是每点幅值比和相位；g_dds_table 是基础正弦表，g_dds_output 是
 * DDS 展开后的 DMA 缓冲；g_curve 选择幅值/相位图，g_point 是当前频点；
 * g_sample_rate 是 ADC 采样率；g_previous_x/y/valid 保存上一曲线点；
 * g_pending_key/valid 在 SysTick 与 main 间传递按键。 */
static uint16_t g_raw_in[SIGNAL_SAMPLE_COUNT];
static uint16_t g_raw_out[SIGNAL_SAMPLE_COUNT];
static float g_frequency_hz[SWEEP_POINTS];
static float g_gain[SWEEP_POINTS];
static float g_phase[SWEEP_POINTS];
#define DDS_TABLE_COUNT (256U)
#define DDS_OUTPUT_COUNT (2048U)
static uint16_t g_dds_table[DDS_TABLE_COUNT];
static uint16_t g_dds_output[DDS_OUTPUT_COUNT];
static tft_st7789_t g_tft;
static uint8_t g_curve;
static uint8_t g_point;
static uint32_t g_sample_rate = SIGNAL_SAMPLE_RATE_HZ;
static int32_t g_previous_x;
static int32_t g_previous_y;
static uint8_t g_previous_valid;
static volatile char g_pending_key;
static volatile uint8_t g_pending_key_valid;

#define APP_KEYPAD_SCAN_PERIOD_MS (5U)

/* 函数索引：App_DrawStatus/DrawSampleRate 更新局部状态；App_PeakToPeak 求一帧峰峰值；
 * App_DrawPoint 把一个扫频点映射到幅频图；App_DrawStaticUi 画坐标轴；App_ClearCurve
 * 擦除旧曲线；App_RunSweepPoint 执行“设频->等待->采样->计算->画点”；App_ProcessKey
 * 修改扫频参数；SysTick 扫描键盘；main 完成外设初始化和扫频循环。frequency_index
 * 是当前点序号，start/stop/step 是扫频参数，amplitude/phase 是测量结果。 */
/* 自写：刷新扫频状态文字和当前点号；color 只决定文字颜色。 */
static void App_DrawStatus(uint16_t color)
{
    /* Small diagnostic field; the plot itself is not cleared. */
    (void)TFT_ST7789_FillRect(&g_tft, 292, 8, 16, 16, color);
}

/* 自写：显示当前 ADC 采样率，修改采样率后只清除该字段。 */
static void App_DrawSampleRate(void)
{
    /* Only the numeric field is cleared, so the title and plot remain stable. */
    (void)TFT_ST7789_FillRect(&g_tft, 208, 26, 88, 12, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawInt32(&g_tft, 208, 26, (int32_t)g_sample_rate,
        TFT_ST7789_FONT_6X12, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
}

/* 自写小算法：遍历 count 个 ADC 码求最大值减最小值；samples 不被修改。 */
static uint16_t App_PeakToPeak(const uint16_t *samples, uint16_t count)
{
    uint16_t i;
    uint16_t min = 4095U;
    uint16_t max = 0U;
    for (i = 0U; i < count; ++i) {
        if (samples[i] < min) min = samples[i];
        if (samples[i] > max) max = samples[i];
    }
    return (uint16_t)(max - min);
}

/* 自写：将第 index 个扫频点的 value 映射为幅频图像素并与上一点连线。 */
static void App_DrawPoint(uint8_t index, float value, uint16_t color)
{
    int32_t x = PLOT_X + (int32_t)(((uint32_t)index * (PLOT_W - 1U)) /
        (SWEEP_POINTS - 1U));
    int32_t y = PLOT_Y + PLOT_H - 1 -
        (int32_t)(value * (float)(PLOT_H - 1));
    if (y < PLOT_Y) y = PLOT_Y;
    if (y >= PLOT_Y + PLOT_H) y = PLOT_Y + PLOT_H - 1;
    if (g_previous_valid != 0U) {
        (void)TFT_ST7789_DrawLine(&g_tft, g_previous_x, g_previous_y,
            x, y, color);
    }
    (void)TFT_ST7789_FillRect(&g_tft, x - 1, y - 1, 3, 3, color);
    g_previous_x = x;
    g_previous_y = y;
    g_previous_valid = 1U;
}

/* 自写：画扫频图边框、坐标轴和固定说明，只在启动/翻页时调用。 */
static void App_DrawStaticUi(void)
{
    g_previous_valid = 0U;
    (void)TFT_ST7789_FillScreen(&g_tft, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawString(&g_tft, 8, 8, "SWEEP TEST",
        TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN, TFT_ST7789_BLACK,
        false, false);
    (void)TFT_ST7789_DrawString(&g_tft, 8, 26, "A:AMP B:PHASE C:RESET D:RATE",
        TFT_ST7789_FONT_6X12, TFT_ST7789_WHITE, TFT_ST7789_BLACK,
        false, false);
    (void)TFT_ST7789_DrawString(&g_tft, 190, 26, "SR:",
        TFT_ST7789_FONT_6X12, TFT_ST7789_WHITE, TFT_ST7789_BLACK,
        false, false);
    App_DrawSampleRate();
    (void)TFT_ST7789_DrawRect(&g_tft, PLOT_X, PLOT_Y, PLOT_W, PLOT_H,
        TFT_ST7789_BLUE);
    (void)TFT_ST7789_DrawLine(&g_tft, PLOT_X, PLOT_Y + PLOT_H / 2,
        PLOT_X + PLOT_W - 1, PLOT_Y + PLOT_H / 2, TFT_ST7789_BLUE);
}

/* 自写：擦除旧幅频曲线并重置 previous_x/previous_y。 */
static void App_ClearCurve(void)
{
    g_previous_valid = 0U;
    (void)TFT_ST7789_FillRect(&g_tft, PLOT_X + 1, PLOT_Y + 1,
        PLOT_W - 2, PLOT_H - 2, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawLine(&g_tft, PLOT_X, PLOT_Y + PLOT_H / 2,
        PLOT_X + PLOT_W - 1, PLOT_Y + PLOT_H / 2, TFT_ST7789_BLUE);
}

/* 模块调用组合：设置 DDS 频率、等待稳定、启动 ADC DMA、调用频率/相位/幅值模块，
 * 最后把结果存入数组并画点；扫频步进修改在这里或对应 APP_* 宏。 */
static void App_RunSweepPoint(void)
{
    uint16_t in_vpp;
    uint16_t out_vpp;
    if (g_point >= SWEEP_POINTS) {
        g_point = 0U;
        App_ClearCurve();
    }
    signal_dual_adc_phase_result_t phase_result;
    static const signal_dual_adc_phase_config_t phase_config = {
        16U, 64U, 1U, 16U, 64U
    };
    /* 【Wave Output 整合模块】内部完成 DDS/DAC DMA 输出和整数周期缓冲。 */
    (void)SignalWaveOutput_SineWithOffset(g_frequency_hz[g_point],
        2.97f, 1.65f);
    /* 【双 ADC 模块】同步采集被测输入和参考输出，保证相位基准一致。 */
    if (SignalDualADC_Start(g_raw_in, g_raw_out, SIGNAL_SAMPLE_COUNT) !=
        SIGNAL_RESULT_OK) {
        App_DrawStatus(TFT_ST7789_RED);
        return;
    }
    {
        uint32_t wait_count = 0U;
        while (!SignalDualADC_IsFinished() &&
            (wait_count < ADC_WAIT_LIMIT)) {
            ++wait_count;
            /* Poll with a bounded timeout so a missing DMA IRQ cannot freeze
             * the user interface forever.  ADC/DMA interrupts still run. */
            __NOP();
        }
        if (!SignalDualADC_IsFinished()) {
            (void)SignalDualADC_Stop();
            App_DrawStatus(TFT_ST7789_YELLOW);
            return;
        }
    }
    App_DrawStatus(TFT_ST7789_GREEN);
    in_vpp = App_PeakToPeak(g_raw_in, SIGNAL_SAMPLE_COUNT);
    out_vpp = App_PeakToPeak(g_raw_out, SIGNAL_SAMPLE_COUNT);
    g_gain[g_point] = (in_vpp == 0U) ? 0.0f :
        (float)out_vpp / (float)in_vpp;
    g_phase[g_point] = 0.0f;
    /* 【双路相位模块】直接使用两路同步原始码计算 phase[g_point]。 */
    if (SignalDualADCPhase_Process(g_raw_in, g_raw_out,
            SIGNAL_SAMPLE_COUNT, g_sample_rate, &phase_config,
            &phase_result) == SIGNAL_ALGORITHM_OK &&
        phase_result.valid != 0U) {
        g_phase[g_point] = ((float)phase_result.phase_degrees + 180.0f) /
            360.0f;
    }
    App_DrawPoint(g_point, g_curve == 0U ? g_gain[g_point] * 0.5f :
        (g_phase[g_point] + 1.0f) * 0.5f,
        g_curve == 0U ? TFT_ST7789_YELLOW : TFT_ST7789_CYAN);
    ++g_point;
}

/* 自写：解释矩阵键盘 symbol，切换曲线、起止频率或重新开始扫频。 */
static void App_ProcessKey(char symbol)
{
    if (symbol == 'A') g_curve = 0U;
    if (symbol == 'B') g_curve = 1U;
    if (symbol == 'C') {
        g_point = 0U;
        App_ClearCurve();
    }
    if (symbol == 'D') {
        g_sample_rate = (g_sample_rate == 100000U) ? 50000U : 100000U;
        (void)SignalDualADC_SetSampleRate(g_sample_rate);
        App_DrawSampleRate();
    }
}

/* 自写中断：每 5 ms 调用键盘模块，产生的字符存入 pending_key 供 main 处理。 */
void SysTick_Handler(void)
{
    static uint8_t milliseconds;
    char symbol;

    ++milliseconds;
    if (milliseconds < APP_KEYPAD_SCAN_PERIOD_MS) return;
    milliseconds = 0U;
    /* 【矩阵键盘模块】完成扫描/消抖后，本中断只保存一个 pending_key。 */
    if (SignalMatrixKeypad4x4_ReadNewSymbol(&symbol) == SIGNAL_RESULT_OK) {
        g_pending_key = symbol;
        g_pending_key_valid = 1U;
    }
}

/* main：初始化 DDS/DAC DMA、双 ADC、扫频模块、键盘和 ST7789；循环按 point 计算一个
 * 频率点，点号达到 SWEEP_POINTS 后重新开始。A/B/C/D 改曲线或扫频参数，SysTick
 * 只负责把新按键交给 pending_key。 */
int main(void)
{
    const signal_dual_adc_config_t adc_config = {
        SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U
    };
    const signal_frequency_sweep_config_t sweep_config = {
        500.0f, 8000.0f, SWEEP_POINTS, false
    };
    const signal_dac_dma_mspm0_config_t dac_config = {
        SIGNAL_DAC_UPDATE_RATE_HZ, CPUCLK_FREQ, 65536U
    };
    char symbol;

    SYSCFG_DL_init();
    if (SignalDualADC_Init(&adc_config) != SIGNAL_RESULT_OK) while (1) { }
    /* The dual-ADC module owns the shared ISR; enable its DMA channel IRQs
     * at the DMA peripheral level so DMA_IRQHandler can observe completion. */
    DL_DMA_enableInterrupt(DMA,
        DL_DMA_INTERRUPT_CHANNEL0 | DL_DMA_INTERRUPT_CHANNEL1);
    {
        const signal_wave_output_config_t wave_config = {
            g_dds_table, DDS_TABLE_COUNT, g_dds_output, DDS_OUTPUT_COUNT,
            dac_config, 12U, SIGNAL_ADC_VREF_V
        };
        if (SignalWaveOutput_Init(&wave_config) != SIGNAL_RESULT_OK) while (1) { }
    }
    /* 【Frequency Sweep 模块】按起止频率和点数生成频率数组，main 不自己计算步进表。 */
    if (SignalFrequencySweep_Generate(&sweep_config, g_frequency_hz,
        SWEEP_POINTS) != SIGNAL_RESULT_OK) while (1) { }
    if (SignalTFTST7789_MSPM0_Init(&g_tft, TFT_ST7789_ROTATION_270,
        0U, 0U) != TFT_ST7789_OK) while (1) { }
    App_DrawStaticUi();
    if (SysTick_Config(CPUCLK_FREQ / 1000U) != 0U) while (1) { }
    while (1) {
        if (g_pending_key_valid != 0U) {
            symbol = g_pending_key;
            g_pending_key_valid = 0U;
            App_ProcessKey(symbol);
        }
        App_RunSweepPoint();
    }
}
