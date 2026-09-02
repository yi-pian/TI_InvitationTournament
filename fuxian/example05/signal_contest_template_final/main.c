/*
 * example05：阻抗参数测量装置。
 * ADC0 采集待测网络电流，ADC1 采集待测网络两端电压；两路由同一个
 * TIMER 触发，因此可以在软件中得到复阻抗 Z=V/I。外部 AD9833 按 SPI 指令依次输出
 * 1 kHz～100 kHz 对数扫频，屏幕使用 ST7789 的 8x16 字库显示测量结果。
 * signal_config.h 中的两个 SCALE 宏用于匹配题目给出的模拟前端增益。
 */
/* 大步骤 1：复制模块 README 的头文件和题目参数。 */
#include <math.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_config.h"
#include "ad9833.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_frequency_sweep.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_font.h"
#include "signal_tft_st7789_mspm0g3507.h"

/* ============================================================
 * 大步骤 2：应用层参数和工作缓冲区
 *
 * APP_SWEEP_POINTS：扫频表中的频率个数；频率具体数值由扫频模块生成。
 * APP_ADC_WAIT_LIMIT：主循环等待双 ADC DMA 完成的最大轮询次数，防止 DMA/IRQ
 *                      异常时程序永久卡在一个频点。
 * APP_HOLD_TIME_MS：扫频结束后在屏幕上保持最终结果的时间。
 * ============================================================ */
#define APP_SWEEP_POINTS       (32U)
#define APP_ADC_WAIT_LIMIT     (6000000UL)
#define APP_TWO_PI             (6.2831853071795864769f)
#define APP_HOLD_TIME_MS       (1500U)

/* g_raw_current/g_raw_voltage：双 ADC DMA 写入的原始 12 位采样码。
 * 数组下标相同表示同一个 Timer 触发时刻；DMA 完成前禁止读取。 */
static uint16_t g_raw_current[SIGNAL_SAMPLE_COUNT];
static uint16_t g_raw_voltage[SIGNAL_SAMPLE_COUNT];
/* 每个扫频点的频率、有效值、阻抗实虚部和相位结果。 */
static float g_frequency_hz[APP_SWEEP_POINTS];
static float g_voltage_rms[APP_SWEEP_POINTS];
static float g_current_rms[APP_SWEEP_POINTS];
static float g_z_abs[APP_SWEEP_POINTS];
static float g_z_re[APP_SWEEP_POINTS];
static float g_z_im[APP_SWEEP_POINTS];
static float g_phase_deg[APP_SWEEP_POINTS];
/* g_tft 是 ST7789 平台句柄；g_point 是当前扫频点序号。 */
static tft_st7789_t g_tft;
static uint8_t g_point;
/* 这些变量在 SysTick 中断和 main 主循环之间共享，必须使用 volatile。 */
static volatile uint8_t g_hold_active;
static volatile uint32_t g_hold_ms;
/* 只有 ADC 成功完成的帧才计入有效点数。 */
static uint8_t g_valid_point_count;

/* 函数索引：App_Text/App_ClearRow/App_LabelFloat 是显示薄封装；
 * App_DrawStaticUi/App_DrawProgress/App_DrawLiveResult 负责屏幕页面；
 * App_PhaseDegrees/App_WrapPhase 负责两路基波相位；
 * App_MeasureFrame 负责一帧 DMA、RMS 和复阻抗；
 * App_FitSeriesModel/App_InterpolateCrossing/App_DrawAnalysis 负责 RLC、F0、BW、Q；
 * App_AD9833_Init/App_AD9833_SetSine 负责通过独立 SPI0 写入 AD9833 的 Mode 2；
 * App_RunSweepPoint 负责“AD9833 设频->采样->显示”的单点状态机；
 * SysTick_Handler 和 main 负责保持计时、初始化和自动重扫。 */

static void App_Text(int32_t x, int32_t y, const char *text, uint16_t color)
{
    /* 所有文字统一走 ST7789 README 的 8×16 字库调用，避免混用字体。 */
    (void)TFT_ST7789_DrawString(&g_tft, x, y, text, TFT_ST7789_FONT_8X16,
        color, TFT_ST7789_BLACK, false, false);
}

static void App_ClearRow(int32_t y)
{
    /* 数值刷新前只擦除当前 16 像素行，保留标题和其他测量行。 */
    (void)TFT_ST7789_FillRect(&g_tft, 0, y, 320, 16, TFT_ST7789_BLACK);
}

static void App_LabelFloat(int32_t y, const char *label, float value,
    uint8_t decimals, const char *unit, uint16_t color)
{
    /* 先清行，再画标签、浮点值和单位，防止位数变短时残留旧字符。 */
    App_ClearRow(y);
    App_Text(4, y, label, TFT_ST7789_WHITE);
    (void)TFT_ST7789_DrawFloat(&g_tft, 76, y, value, decimals,
        TFT_ST7789_FONT_8X16, color, TFT_ST7789_BLACK, false);
    App_Text(220, y, unit, TFT_ST7789_WHITE);
}

static void App_DrawStaticUi(void)
{
    /* 仅在上电和自动重扫开始时整屏清除；扫频过程中使用局部刷新。 */
    (void)TFT_ST7789_FillScreen(&g_tft, TFT_ST7789_BLACK);
    App_Text(4, 2, "IMPEDANCE METER", TFT_ST7789_CYAN);
    App_Text(220, 2, "RLC", TFT_ST7789_YELLOW);
    App_Text(4, 220, "AUTO 1K-100KHz", TFT_ST7789_GREEN);
    App_Text(220, 220, "A:RESTART", TFT_ST7789_WHITE);
}

static void App_DrawProgress(void)
{
    /* 用当前点号/总点数把进度换算到 300 像素宽的进度条。 */
    uint32_t width = ((uint32_t)(g_point + 1U) * 300U) / APP_SWEEP_POINTS;
    (void)TFT_ST7789_FillRect(&g_tft, 8, 202, 304, 12,
        TFT_ST7789_RGB565(30, 30, 30));
    (void)TFT_ST7789_FillRect(&g_tft, 10, 204, (int32_t)width, 8,
        TFT_ST7789_GREEN);
}

/* ============================================================
 * 大步骤 3：复制 AD9833 README 的 Init/SetOutput，使用独立 SPI0。
 * ST7789 固定使用 SPI1（PB9/PB8、Mode 0），而 AD9833 使用 SPI0（PA12/PA9、Mode 2）
 * 和 PA8 的 FSYNC 分帧；两者不共用任何 SPI 信号线，所以无需在运行中切换模式或片选。 */
static uint8_t App_AD9833_Init(void)
{
    bool ok;
    /* README：FSYNC 空闲必须为高；AD9833 的 MCLK 来自模块晶振，不由 MCU 配置。 */
    DL_GPIO_setPins(GPIO_AD9833_PORT, GPIO_AD9833_AD9833_FSYNC_PIN);
    /* 以下 AD9833_Init 调用直接来自 AD9833 README；SPI_AD9833 已由 SysConfig 固定为 Mode 2。 */
    ok = AD9833_Init(SPI_AD9833_INST, GPIO_AD9833_PORT,
        GPIO_AD9833_AD9833_FSYNC_PIN);
    return ok ? 1U : 0U;
}

static uint8_t App_AD9833_SetSine(uint32_t output_hz)
{
    bool ok;
    /* 以下 SetOutput 调用直接来自 README；频率、相位 0 和正弦类型仅为本题参数。 */
    ok = AD9833_SetOutput(SPI_AD9833_INST, GPIO_AD9833_PORT,
        GPIO_AD9833_AD9833_FSYNC_PIN, SIGNAL_AD9833_MCLK_HZ,
        output_hz, 0U, AD9833_WAVE_SINE);
    return ok ? 1U : 0U;
}

/* ============================================================
 * 大步骤 4：双路 ADC 完成后，用正交相关求两路基波相位。
 * 这是在 modules/README.md“阻抗测量组合补充”中给出的应用代码，
 * 不改动 signal_dual_adc_mspm0g3507.c/.h。
 * 输入模型为 x[n]=A*cos(w*n/Fs+phi)。与 cos、sin 分别相关后，
 * atan2(-imag, real) 得到 phi；两通道相减后得到电压相对电流的相位差。 */
static float App_PhaseDegrees(const uint16_t *samples, float frequency_hz,
    float sample_rate_hz, float mean)
{
    uint32_t i;
    /* step 是相邻 ADC 样本之间的参考角增量。 */
    float real_part = 0.0f;
    float imag_part = 0.0f;
    float step = APP_TWO_PI * frequency_hz / sample_rate_hz;
    for (i = 0U; i < SIGNAL_SAMPLE_COUNT; ++i) {
        /* 去掉 1.65 V 偏置后，再累加同相和正交分量。 */
        float angle = step * (float)i;
        float sample = (float)samples[i] - mean;
        real_part += sample * cosf(angle);
        imag_part += sample * sinf(angle);
    }
    /* ADC 波形按 cos(wt+phi) 相关，-imag/real 才是 phi。 */
    return atan2f(-imag_part, real_part) * 57.2957795131f;
}

static float App_WrapPhase(float phase)
{
    /* atan2 的结果已经在 ±180°附近；此处处理两通道相减后的越界。 */
    while (phase > 180.0f) phase -= 360.0f;
    while (phase < -180.0f) phase += 360.0f;
    return phase;
}

/* ============================================================
 * 大步骤 5：复制双 ADC README 的 Start/等待闭环，再做本题的
 * Vrms、Irms、|Z|、相位和 Z 实虚部换算。
 * 本函数在 main 中运行，绝不在 DMA/ADC 中断里做浮点和屏幕操作。 */
static uint8_t App_MeasureFrame(float frequency_hz)
{
    uint32_t i;
    uint32_t sample_rate_hz;
    float mean_i = 0.0f;
    float mean_v = 0.0f;
    float sum_i2 = 0.0f;
    float sum_v2 = 0.0f;
    float phase_i;
    float phase_v;
    /* 失败帧也必须清零，避免自动重扫时沿用上一轮旧结果。 */
    g_voltage_rms[g_point] = 0.0f;
    g_current_rms[g_point] = 0.0f;
    g_z_abs[g_point] = 0.0f;
    g_z_re[g_point] = 0.0f;
    g_z_im[g_point] = 0.0f;
    g_phase_deg[g_point] = 0.0f;
    /* Start 只设置 DMA 目的地址并启动 Timer/ADC；返回后缓冲区仍不可读。 */
    if (SignalDualADC_Start(g_raw_current, g_raw_voltage,
            SIGNAL_SAMPLE_COUNT) != SIGNAL_RESULT_OK) return 0U;
    /* 使用有上限的忙等待；正常情况下由 DMA 完成标志提前退出。 */
    for (i = 0U; i < APP_ADC_WAIT_LIMIT; ++i) {
        if (SignalDualADC_IsFinished()) break;
        __NOP();
    }
    if (!SignalDualADC_IsFinished()) {
        SignalDualADC_Stop();
        return 0U;
    }
    /* Timer 计数是整数分频，优先使用模块报告的实际 Fs 计算相位。 */
    sample_rate_hz = SignalDualADC_GetConfiguredRate();
    if (sample_rate_hz == 0U) sample_rate_hz = SIGNAL_SAMPLE_RATE_HZ;
    /* 第一次遍历求 ADC 码均值，用于消除模拟前端的直流偏置。 */
    for (i = 0U; i < SIGNAL_SAMPLE_COUNT; ++i) {
        mean_i += (float)g_raw_current[i];
        mean_v += (float)g_raw_voltage[i];
    }
    mean_i /= (float)SIGNAL_SAMPLE_COUNT;
    mean_v /= (float)SIGNAL_SAMPLE_COUNT;
    /* 第二次遍历按前端校准系数换算成 A/V，并累加平方和。 */
    for (i = 0U; i < SIGNAL_SAMPLE_COUNT; ++i) {
        float current = ((float)g_raw_current[i] - mean_i) *
            SIGNAL_CURRENT_SCALE_A_PER_CODE;
        float voltage = ((float)g_raw_voltage[i] - mean_v) *
            SIGNAL_VOLTAGE_SCALE_V_PER_CODE;
        sum_i2 += current * current;
        sum_v2 += voltage * voltage;
    }
    /* RMS=sqrt(mean(x²))；电流过小时不计算 V/I，避免除零。 */
    g_current_rms[g_point] = sqrtf(sum_i2 / (float)SIGNAL_SAMPLE_COUNT);
    g_voltage_rms[g_point] = sqrtf(sum_v2 / (float)SIGNAL_SAMPLE_COUNT);
    g_z_abs[g_point] = (g_current_rms[g_point] > 1.0e-9f) ?
        g_voltage_rms[g_point] / g_current_rms[g_point] : 0.0f;
    /* 电压相位减电流相位，得到阻抗角；再由极坐标转为 Zre/Zim。 */
    phase_i = App_PhaseDegrees(g_raw_current, frequency_hz,
        (float)sample_rate_hz, mean_i);
    phase_v = App_PhaseDegrees(g_raw_voltage, frequency_hz,
        (float)sample_rate_hz, mean_v);
    g_phase_deg[g_point] = App_WrapPhase(phase_v - phase_i);
    g_z_re[g_point] = g_z_abs[g_point] *
        cosf(g_phase_deg[g_point] * 0.01745329252f);
    g_z_im[g_point] = g_z_abs[g_point] *
        sinf(g_phase_deg[g_point] * 0.01745329252f);
    return 1U;
}

/* ============================================================
 * 大步骤 6：把每个扫频点组成的复阻抗拟合成串联 RLC。
 * 串联模型：Zre=R，Zim=wL+(1/w)*(-1/C)，其中 w=2*pi*f。
 * 拟合、带宽和 Q 公式均在 modules/README.md 中先说明，再复制到此处。 */
static void App_FitSeriesModel(float *resistance, float *inductance,
    float *capacitance)
{
    uint32_t i;
    uint32_t valid = 0U;
    float sum_r = 0.0f;
    float sww = 0.0f;
    float swu = 0.0f;
    float suu = 0.0f;
    float swx = 0.0f;
    float sux = 0.0f;
    float determinant;
    *resistance = 0.0f;
    *inductance = 0.0f;
    *capacitance = 0.0f;
    /* 只使用阻抗有效的频点，建立两参数线性最小二乘方程。 */
    for (i = 0U; i < APP_SWEEP_POINTS; ++i) {
        float w = APP_TWO_PI * g_frequency_hz[i];
        float u;
        if (g_z_abs[i] <= 0.0f) continue;
        u = 1.0f / w;
        sum_r += g_z_re[i];
        sww += w * w;
        swu += w * u;
        suu += u * u;
        swx += w * g_z_im[i];
        sux += u * g_z_im[i];
        ++valid;
    }
    /* 没有任何有效点时保持输出为 0，由上层显示 NO VALID DATA。 */
    if (valid == 0U) return;
    *resistance = sum_r / (float)valid;
    /* 行列式过小表示频率点不足或病态，不能可信地求 L/C。 */
    determinant = sww * suu - swu * swu;
    if (fabsf(determinant) > 1.0e-12f) {
        float l = (swx * suu - sux * swu) / determinant;
        float minus_inv_c = (sux * sww - swx * swu) / determinant;
        if (l > 0.0f) *inductance = l;
        if (minus_inv_c < 0.0f) *capacitance = -1.0f / minus_inv_c;
    }
    if (*resistance < 0.0f) *resistance = -*resistance;
}

static float App_InterpolateCrossing(float f0, float z0, float f1, float z1,
    float threshold)
{
    /* 用相邻两个 |Z| 点线性插值半功率阈值的交点。 */
    float denominator = z1 - z0;
    if (fabsf(denominator) < 1.0e-9f) return f0;
    return f0 + (threshold - z0) * (f1 - f0) / denominator;
}

/* ============================================================
 * 大步骤 7：少量题目组合逻辑：R/L/C 判断、谐振频率、带宽、Q 和显示。
 * 先扫描有效点，再拟合 RLC；最后统一计算 F0、BW、Q 并刷新结果页。 */
static void App_DrawAnalysis(void)
{
    uint32_t i;
    uint32_t min_index = APP_SWEEP_POINTS;
    float resistance;
    float inductance;
    float capacitance;
    float f0;
    float bandwidth = 0.0f;
    float q;
    float phase_sum = 0.0f;
    uint32_t phase_count = 0U;
    uint8_t has_positive = 0U;
    uint8_t has_negative = 0U;
    const char *type;
    float value;
    const char *unit;
    /* 找到 |Z| 最小点作为谐振候选，同时统计相位正负。 */
    for (i = 0U; i < APP_SWEEP_POINTS; ++i) {
        if (g_z_abs[i] > 0.0f) {
            if ((min_index >= APP_SWEEP_POINTS) ||
                (g_z_abs[i] < g_z_abs[min_index])) min_index = i;
            phase_sum += g_phase_deg[i];
            ++phase_count;
            if (g_phase_deg[i] > 12.0f) has_positive = 1U;
            if (g_phase_deg[i] < -12.0f) has_negative = 1U;
        }
    }
    if ((g_valid_point_count == 0U) ||
        (min_index >= APP_SWEEP_POINTS) || (phase_count == 0U)) {
        App_ClearRow(24);
        App_Text(4, 24, "NO VALID DATA", TFT_ST7789_RED);
        return;
    }
    /* 平均相位用于纯 R/L/C 分类；正负相位同时出现表示跨过串联谐振。 */
    phase_sum /= (float)phase_count;
    App_FitSeriesModel(&resistance, &inductance, &capacitance);
    if (has_positive && has_negative) {
        /* 相位从容性区跨到感性区，判为串联 RLC。 */
        type = "TYPE:RLC";
        value = resistance;
        unit = "Ohm";
    } else if (fabsf(phase_sum) <= 12.0f) {
        /* 相位接近 0°，判为电阻性网络。 */
        type = "TYPE:R";
        value = resistance;
        unit = "Ohm";
    } else if (phase_sum > 0.0f) {
        /* 正相位表示感性，X_L=2*pi*f*L。 */
        type = "TYPE:L";
        value = (inductance > 0.0f) ? inductance :
            (g_z_im[min_index] / (APP_TWO_PI * g_frequency_hz[min_index]));
        unit = "H";
    } else {
        /* 负相位表示容性，X_C=-1/(2*pi*f*C)。 */
        type = "TYPE:C";
        value = (capacitance > 0.0f) ? capacitance :
            (-1.0f / (APP_TWO_PI * g_frequency_hz[min_index] *
                g_z_im[min_index]));
        unit = "F";
    }
    /* 拟合出有效 L/C 时使用解析谐振频率，否则使用 |Z| 谷值频点。 */
    f0 = g_frequency_hz[min_index];
    if ((inductance > 0.0f) && (capacitance > 0.0f)) {
        float fitted_f0 = 1.0f / (APP_TWO_PI * sqrtf(inductance * capacitance));
        if ((fitted_f0 >= SIGNAL_EXPECTED_MIN_HZ) &&
            (fitted_f0 <= SIGNAL_EXPECTED_MAX_HZ)) f0 = fitted_f0;
    }
    if (resistance <= 0.0f) resistance = g_z_abs[min_index];
    {
        /* 串联谐振半功率点对应 |Z|=sqrt(2)*R。 */
        float threshold = resistance * 1.41421356f;
        float low = 0.0f;
        float high = 0.0f;
        /* 从谷值向低频方向找第一个阈值交点 fL。 */
        for (i = min_index; i > 0U; --i) {
            if ((g_z_abs[i - 1U] >= threshold) &&
                (g_z_abs[i] < threshold)) {
                low = App_InterpolateCrossing(g_frequency_hz[i - 1U],
                    g_z_abs[i - 1U], g_frequency_hz[i], g_z_abs[i], threshold);
                break;
            }
        }
        /* 从谷值向高频方向找第二个阈值交点 fH。 */
        for (i = min_index; i + 1U < APP_SWEEP_POINTS; ++i) {
            if ((g_z_abs[i + 1U] >= threshold) &&
                (g_z_abs[i] < threshold)) {
                high = App_InterpolateCrossing(g_frequency_hz[i], g_z_abs[i],
                    g_frequency_hz[i + 1U], g_z_abs[i + 1U], threshold);
                break;
            }
        }
        if ((low > 0.0f) && (high > low)) bandwidth = high - low;
    }
    /* 两侧交点齐全才有带宽；没有带宽时 Q 显示为 0。 */
    q = (bandwidth > 0.0f) ? f0 / bandwidth : 0.0f;
    App_Text(4, 24, "SWEEP DONE", TFT_ST7789_GREEN);
    App_LabelFloat(42, "Vrms:", g_voltage_rms[min_index], 3, "V", TFT_ST7789_YELLOW);
    App_LabelFloat(60, "Irms:", g_current_rms[min_index], 3, "A", TFT_ST7789_YELLOW);
    App_LabelFloat(78, "|Z|:", g_z_abs[min_index], 2, "Ohm", TFT_ST7789_CYAN);
    App_LabelFloat(96, "Phase:", g_phase_deg[min_index], 1, "deg", TFT_ST7789_CYAN);
    App_ClearRow(114);
    App_Text(4, 114, type, TFT_ST7789_MAGENTA);
    App_LabelFloat(132, "Value:", value, 5, unit, TFT_ST7789_MAGENTA);
    App_LabelFloat(150, "F0:", f0 / 1000.0f, 3, "kHz", TFT_ST7789_GREEN);
    App_LabelFloat(168, "BW:", bandwidth / 1000.0f, 3, "kHz", TFT_ST7789_GREEN);
    App_LabelFloat(186, "Q:", q, 2, "", TFT_ST7789_GREEN);
    if ((has_positive && has_negative) && (inductance > 0.0f) &&
        (capacitance > 0.0f)) {
        App_LabelFloat(204, "L/C:", inductance * 1000.0f, 3, "mH", TFT_ST7789_WHITE);
        App_LabelFloat(222, "C:", capacitance * 1000000000.0f, 2, "nF", TFT_ST7789_WHITE);
    }
}

static void App_DrawLiveResult(void)
{
    /* 当前点页面只刷新动态行；频率、Vrms、Irms、|Z|、相位和进度条共用 8×16 字库。 */
    App_ClearRow(24);
    App_Text(4, 24, "SCANNING", TFT_ST7789_GREEN);
    (void)TFT_ST7789_DrawFloat(&g_tft, 116, 24, g_frequency_hz[g_point] / 1000.0f,
        2U, TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
    App_Text(220, 24, "kHz", TFT_ST7789_WHITE);
    App_LabelFloat(42, "Vrms:", g_voltage_rms[g_point], 3, "V", TFT_ST7789_YELLOW);
    App_LabelFloat(60, "Irms:", g_current_rms[g_point], 3, "A", TFT_ST7789_YELLOW);
    App_LabelFloat(78, "|Z|:", g_z_abs[g_point], 2, "Ohm", TFT_ST7789_CYAN);
    App_LabelFloat(96, "Phase:", g_phase_deg[g_point], 1, "deg", TFT_ST7789_CYAN);
    App_DrawProgress();
}

static void App_RunSweepPoint(void)
{
    uint32_t output_hz;
    /* 当前点号达到总点数后，进入最终分析和保持状态。 */
    if (g_point >= APP_SWEEP_POINTS) {
        App_DrawAnalysis();
        g_hold_ms = 0U;
        g_hold_active = 1U;
        return;
    }
    /* AD9833 API 使用整数 Hz；对数表的 float 频点四舍五入后同时作为显示和相位参考。 */
    output_hz = (uint32_t)(g_frequency_hz[g_point] + 0.5f);
    g_frequency_hz[g_point] = (float)output_hz;
    if (App_AD9833_SetSine(output_hz) == 0U) {
        App_ClearRow(24);
        App_Text(4, 24, "AD9833 ERROR", TFT_ST7789_RED);
        ++g_point;
        return;
    }
    /* AD9833 无频率回读；28 bit FTW 量化误差极小，本题使用刚写入的整数 Hz 作相位参考。 */
    if (App_MeasureFrame(g_frequency_hz[g_point]) != 0U) {
        ++g_valid_point_count;
        App_DrawLiveResult();
    } else {
        App_ClearRow(24);
        App_Text(4, 24, "ADC ERROR", TFT_ST7789_RED);
    }
    ++g_point;
}

/* SysTick 只负责扫频完成后的计时；不能在中断中做浮点运算或 SPI 刷屏。 */
void SysTick_Handler(void)
{
    if (g_hold_active != 0U && g_hold_ms < APP_HOLD_TIME_MS) ++g_hold_ms;
}

int main(void)
{
    /* ========================================================
     * 大步骤 8：按比赛顺序完成初始化和主循环
     * 1) SysConfig 初始化硬件；
     * 2) 初始化双 ADC；
     * 3) 初始化外部 AD9833 正弦激励；
     * 4) 生成 1 kHz～100 kHz 对数频率表；
     * 5) 初始化 ST7789 及 8×16 字库页面；
     * 6) 进入“逐点扫频 -> 最终分析 -> 保持 -> 自动重扫”。
     * 每个模块的初始化调用均保持 README 形状，失败立即停机。 */
    const signal_dual_adc_config_t adc_config = {
        SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U
    };
    const signal_frequency_sweep_config_t sweep_config = {
        SIGNAL_EXPECTED_MIN_HZ, SIGNAL_EXPECTED_MAX_HZ,
        APP_SWEEP_POINTS, true
    };
    /* 由 SysConfig 生成的函数配置 GPIO、Timer、ADC、DMA 和 SPI。 */
    SYSCFG_DL_init();
    /* 双 ADC 使用同一个 Timer 触发；65536U 是 16 位 Timer 计数上限。 */
    if (SignalDualADC_Init(&adc_config) != SIGNAL_RESULT_OK) while (1) { }
    /* AD9833 README 的 Init：先保持 FSYNC 高，再写 RESET/B28 控制字。 */
    if (App_AD9833_Init() == 0U) while (1) { }
    /* 先生成完整频率表，主循环只按 g_point 取值，不现场计算对数。 */
    if (SignalFrequencySweep_Generate(&sweep_config, g_frequency_hz,
            APP_SWEEP_POINTS) != SIGNAL_RESULT_OK) while (1) { }
    /* 横屏 270°得到 320×240 页面；偏移 0,0 按 ST7789 README 默认配置。 */
    if (SignalTFTST7789_MSPM0_Init(&g_tft, TFT_ST7789_ROTATION_270,
            0U, 0U) != TFT_ST7789_OK) while (1) { }
    /* 先显示固定标题，确认屏幕链路后才开始输出和采样。 */
    App_DrawStaticUi();
    /* 1 ms SysTick 只用于结果保持计时。 */
    if (SysTick_Config(CPUCLK_FREQ / 1000U) != 0U) while (1) { }
    while (1) {
        /* 保持状态下不重复采样，等待结果展示时间到期。 */
        if (g_hold_active != 0U) {
            if (g_hold_ms >= APP_HOLD_TIME_MS) {
                g_hold_active = 0U;
                g_point = 0U;
                g_valid_point_count = 0U;
                App_DrawStaticUi();
            } else {
                __WFI();
            }
        } else {
            /* 扫频状态每次只处理一个频点，下一轮继续处理下一个点。 */
            App_RunSweepPoint();
        }
    }
}
