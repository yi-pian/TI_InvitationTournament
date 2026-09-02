/*
 * example06 总流程：单 ADC/DMA 采样 -> FFT 自动搜索目标频率 ->
 * I/Q 计算并重构目标波形 -> ST7789 显示频率、峰峰值和目标波形 -> 矩阵键盘选择 1~5 个周期。
 *
 * 本文件只做比赛题目的“应用层编排”：参数、数据处理、坐标映射和显示布局由本文件
 * 完成；ADC、DMA、ST7789、8x16 字库和矩阵键盘的底层实现全部来自 modules/ 中按 README
 * 原样复制的模块。硬件引脚、DMA 通道、Timer/Event 路由只能回到 SysConfig 修改。
 */
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_config.h"
#include "signal_adc_dma.h"
#include "signal_zero_cross.h"
#include "signal_zero_cross_interpolation.h"
#include "signal_tft_st7789_mspm0g3507.h"
#include "signal_tft_st7789_font.h"
#include "signal_matrix_keypad_4x4.h"
#include "arm_math.h"

/* ADC 为 12 位，原始码 0~4095；电压换算使用 signal_config.h 中的 3.3 V 参考。 */
#define APP_ADC_MAX_CODE       (4095.0f)
/* 目标频带对应的 FFT bin：k=f*N/Fs；只在此范围内找峰，排除带外单频干扰。 */
#define APP_FFT_MIN_BIN        ((uint16_t)(SIGNAL_EXPECTED_MIN_HZ * SIGNAL_SAMPLE_COUNT / SIGNAL_SAMPLE_RATE_HZ))
#define APP_FFT_MAX_BIN        ((uint16_t)(SIGNAL_EXPECTED_MAX_HZ * SIGNAL_SAMPLE_COUNT / SIGNAL_SAMPLE_RATE_HZ))
/* ST7789 横屏旋转后为 320x240；波形框留出上方文字区域。 */
#define APP_WAVE_X              (4)
#define APP_WAVE_Y              (76)
#define APP_WAVE_W              (312)
#define APP_WAVE_H              (150)
/* 500 kS/s、100 kHz、1024 点时最多约 205 个上升过零；256 个容量留出余量。 */
#define APP_ZERO_CROSS_CAPACITY  (256U)

/* 【单 ADC 模块】DMA 完成前禁止读取本数组；g_raw 保存一路混合输入波形。 */
static uint16_t g_raw[SIGNAL_SAMPLE_COUNT];
/*
 * I/Q 同步检波后重构出的目标窄带波形，单位 V。
 * 它只保留 FFT 已锁定频率处的正弦分量；原始混合输入中的带外干扰和噪声不用于显示。
 */
static float g_target_v[SIGNAL_SAMPLE_COUNT];
/* 【CMSIS-DSP】FFT 输入/输出复用同一静态数组，避免动态内存。 */
static float g_fft_data[SIGNAL_SAMPLE_COUNT];
/* 【过零模块】事件夹点及线性插值后的小数过零位置；均由正式模块写入。 */
static signal_zero_cross_event_t g_zero_cross_events[APP_ZERO_CROSS_CAPACITY];
static float g_zero_cross_positions[APP_ZERO_CROSS_CAPACITY];
/* 显示区每个横向像素对应一个已经抽取的电压采样点。 */
static float g_wave_data[APP_WAVE_W];
/* CMSIS RFFT 实例；初始化一次，后续每帧只调用处理函数。 */
static arm_rfft_fast_instance_f32 g_fft;
/* 【ST7789 模块】保存屏幕句柄，所有 Draw* API 都使用它。 */
static tft_st7789_t g_tft;
/* 键盘最终只允许改变 1~5 个周期；由 SysTick 中断更新，故声明为 volatile。 */
static volatile uint8_t g_display_periods = 1U;
/* 模块返回状态留在变量中，便于比赛调试时观察失败位置。 */
static volatile signal_result_t g_key_status;
/* 与 22_X 一致：SysTick 每 1 ms 进入一次，累计 5 次后扫描矩阵键盘。 */
#define APP_KEYPAD_SCAN_PERIOD_MS  (5U)

typedef struct {
    float frequency_hz; /* FFT 峰值换算得到的目标频率，单位 Hz。 */
    float peak_to_peak_v; /* I/Q 模长换算得到的目标正弦峰峰值，单位 Vpp。 */
    float half_range_v; /* 当前波形显示的半量程，自动设置为 1.25*max_abs。 */
    bool valid;         /* FFT 频带内找到有效峰时为 true。 */
} app_measurement_t;

/* 所有初始化/硬件错误统一停在 WFI，避免错误状态继续向屏幕和 DMA 传播。 */
static void App_Fail(void)
{
    while (1) { __WFI(); }
}

/*
 * STEP 4：自动搜索、频率和峰峰值测量。
 * ① 求均值去直流；② Hann 加窗后调用 CMSIS RFFT；③ 只搜索 10~100 kHz bin；
 * ④ 用峰值 bin 的正余弦相关得到 I/Q；⑤ 换算频率、峰峰值并重构目标波形。
 * 这里没有在 ADC/DMA 中断里做浮点运算，处理只在主循环、DMA 完成之后执行。
 */
static void App_Measure(const uint16_t *samples, app_measurement_t *m)
{
    uint16_t k;
    uint16_t peak_bin = APP_FFT_MIN_BIN;
    float peak = 0.0f;
    float mean = 0.0f;
    float i_acc = 0.0f;
    float q_acc = 0.0f;
    float fs = (float)SignalADC_GetConfiguredTriggerRate();
    const float n = (float)SIGNAL_SAMPLE_COUNT;

    /* 先将 1024 个 ADC 码相加，求本帧直流偏置。 */
    m->valid = false;
    for (k = 0U; k < SIGNAL_SAMPLE_COUNT; ++k) mean += (float)samples[k];
    mean /= n;
    /* 去直流并乘 Hann 窗，降低采样帧两端突变造成的频谱泄漏。 */
    for (k = 0U; k < SIGNAL_SAMPLE_COUNT; ++k) {
        float x = (float)samples[k] - mean;
        float w = 0.5f - 0.5f * arm_cos_f32(2.0f * PI * (float)k / (n - 1.0f));
        g_fft_data[k] = x * w;
    }
    /* CMSIS-DSP 正向实数 FFT；输入和输出复用 g_fft_data。 */
    arm_rfft_fast_f32(&g_fft, g_fft_data, g_fft_data, 0);
    /* RFFT 的第 k 个复数 bin 位于 [2*k]、[2*k+1]；找频带内最大能量。 */
    for (k = APP_FFT_MIN_BIN; k <= APP_FFT_MAX_BIN; ++k) {
        float re = g_fft_data[2U * k];
        float im = g_fft_data[2U * k + 1U];
        float mag = re * re + im * im;
        if (mag > peak) { peak = mag; peak_bin = k; }
    }
    /* 没有峰值时不刷新测量值，避免屏幕显示无意义的 0。 */
    if ((peak == 0.0f) || (peak_bin == 0U)) return;
    {
        float refined_bin = (float)peak_bin;
        float phase;
        float amplitude_code;
        float iq_magnitude;
        float i_v;
        float q_v;
        /*
         * FFT 整数 bin 间隔为 Fs/N≈488.3 Hz。用峰值左右三个“对数功率”点做抛物线插值，
         * 得到 -0.5~+0.5 的小数 bin 偏移；对 10 kHz 这样的频带边界信号，也允许读取
         * 搜索范围外相邻 bin 作为插值点，不会把它当作候选目标，因此不增加 RAM 或 SysConfig 资源。
         */
        if ((peak_bin > 1U) && (peak_bin < (SIGNAL_SAMPLE_COUNT / 2U - 1U))) {
            float left_re = g_fft_data[2U * (peak_bin - 1U)];
            float left_im = g_fft_data[2U * (peak_bin - 1U) + 1U];
            float right_re = g_fft_data[2U * (peak_bin + 1U)];
            float right_im = g_fft_data[2U * (peak_bin + 1U) + 1U];
            float left_power = left_re * left_re + left_im * left_im;
            float right_power = right_re * right_re + right_im * right_im;
            if ((left_power > 0.0f) && (peak > 0.0f) && (right_power > 0.0f)) {
                float left_log_power = logf(left_power);
                float peak_log_power = logf(peak);
                float right_log_power = logf(right_power);
                float denominator = left_log_power - 2.0f * peak_log_power + right_log_power;
                float offset;
                if (denominator == 0.0f) denominator = 1.0f;
                offset = 0.5f * (left_log_power - right_log_power) / denominator;
                if (offset > 0.5f) offset = 0.5f;
                if (offset < -0.5f) offset = -0.5f;
                refined_bin += offset;
            }
        }
        /* I/Q 同样使用小数 bin，避免频率细化后幅值/相位仍按旧整数 bin 相关。 */
        phase = 2.0f * PI * refined_bin / n;
        /* 在 FFT 峰值频率上做同步检波：I 乘 cos，Q 乘 sin。 */
        for (k = 0U; k < SIGNAL_SAMPLE_COUNT; ++k) {
            float x = (float)samples[k] - mean;
            i_acc += x * arm_cos_f32(phase * (float)k);
            q_acc += x * arm_sin_f32(phase * (float)k);
        }
        /* sqrt(I^2+Q^2) 是相关结果的模；arm_sqrt_f32 输出写入 iq_magnitude。 */
        (void)arm_sqrt_f32(i_acc * i_acc + q_acc * q_acc, &iq_magnitude);
        amplitude_code = 2.0f * iq_magnitude / n;
        /* 频率：实际 Timer 配置采样率乘以“抛物线细化后”的小数 bin/N。 */
        m->frequency_hz = fs * refined_bin / n;
        /* I/Q 模长先换算正弦峰值，再乘 2 后作为用户需要的峰峰值 Vpp。 */
        m->peak_to_peak_v = 2.0f * amplitude_code * SIGNAL_ADC_VREF_V / APP_ADC_MAX_CODE;
        /*
         * 窄带“滤波”与显示：I/Q 是输入在锁定正弦基上的相关投影，乘以 2/N 后分别
         * 成为 cos/sin 两个基的系数。把两者合成，即得到目标频率的纯正弦重构波形。
         * 与该频率不同的干扰在一个采样帧内正负相关会相互抵消，因此不会画到屏幕上。
         */
        i_v = 2.0f * i_acc * SIGNAL_ADC_VREF_V / (n * APP_ADC_MAX_CODE);
        q_v = 2.0f * q_acc * SIGNAL_ADC_VREF_V / (n * APP_ADC_MAX_CODE);
        for (k = 0U; k < SIGNAL_SAMPLE_COUNT; ++k) {
            float angle = phase * (float)k;
            g_target_v[k] = i_v * arm_cos_f32(angle) + q_v * arm_sin_f32(angle);
        }
        /* 初始量程，随后由显示窗口内的实际波形峰值再次自动调整。 */
        m->half_range_v = 0.625f * m->peak_to_peak_v;
        if (m->half_range_v < 0.01f) m->half_range_v = 0.01f;
        m->valid = true;
    }
}

/*
 * STEP 5：用正式 ZeroCross + Interpolation 模块定位显示窗口。
 * FFT/IQ 先把目标频率锁定并重构为纯目标波形；随后在该重构波形上找相邻的上升过零点，
 * 选择周期最接近 FFT 预估值、且能容纳当前 1~5 个周期的那一对。返回的起点和周期
 * 都是小数 sample，因此波形横轴从真实过零处开始，而不是从 DMA 数组的任意第 0 点开始。
 */
static bool App_FindDisplayWindow(const app_measurement_t *m,
    float *window_start_sample, float *period_samples)
{
    signal_zero_cross_config_t zero_cross_config;
    signal_zero_cross_result_t zero_cross_result;
    signal_zero_cross_interpolation_result_t interpolation_result;
    signal_algorithm_status_t algorithm_status;
    float expected_period_samples;
    float hysteresis_v;
    float best_error = 0.0f;
    uint32_t index;
    bool found = false;

    if ((m == NULL) || (window_start_sample == NULL) ||
        (period_samples == NULL) || (m->frequency_hz <= 1.0f)) return false;

    /* 滞回取目标峰峰值的 2.5%（即峰值的 5%），但不小于 1 mV，抑制过零附近的噪声重复触发。 */
    hysteresis_v = m->peak_to_peak_v * 0.025f;
    if (hysteresis_v < 0.001f) hysteresis_v = 0.001f;
    zero_cross_config.threshold_v = 0.0f;
    zero_cross_config.hysteresis_v = hysteresis_v;
    zero_cross_config.direction = SIGNAL_ZERO_CROSS_RISING;

    /* 【ZeroCross README 调用】先获得每个上升过零所在的两个整数样本夹点。 */
    algorithm_status = SignalZeroCross_Process(g_target_v, SIGNAL_SAMPLE_COUNT,
        &zero_cross_config, g_zero_cross_events, APP_ZERO_CROSS_CAPACITY,
        &zero_cross_result);
    if (algorithm_status != SIGNAL_ALGORITHM_OK) return false;

    /* 【Interpolation README 调用】把夹点转换为小数 sample 位置。 */
    algorithm_status = SignalZeroCrossInterpolation_Process(g_target_v,
        SIGNAL_SAMPLE_COUNT, zero_cross_config.threshold_v, g_zero_cross_events,
        zero_cross_result.event_count, g_zero_cross_positions,
        APP_ZERO_CROSS_CAPACITY, &interpolation_result);
    if (algorithm_status != SIGNAL_ALGORITHM_OK) return false;

    expected_period_samples = (float)SignalADC_GetConfiguredTriggerRate() /
        m->frequency_hz;
    for (index = 0U; index + 1U < interpolation_result.position_count; ++index) {
        float candidate_period = g_zero_cross_positions[index + 1U] -
            g_zero_cross_positions[index];
        float candidate_end = g_zero_cross_positions[index] + candidate_period *
            (float)g_display_periods;
        float error = fabsf(candidate_period - expected_period_samples);

        /* 只接受完整位于当前 DMA 帧内的 1~5 周窗口。 */
        if ((candidate_period <= 0.0f) ||
            (candidate_end > (float)(SIGNAL_SAMPLE_COUNT - 1U))) continue;
        if ((!found) || (error < best_error)) {
            *window_start_sample = g_zero_cross_positions[index];
            *period_samples = candidate_period;
            best_error = error;
            found = true;
        }
    }
    return found;
}

/*
 * STEP 5：根据键盘选择的周期数截取“重构目标波形”的显示窗口。
 * 优先用相邻上升过零点给出的真实周期；若本帧找不到可靠过零，才回退到 FFT 频率估计。
 * 量程取窗口内最大绝对电压的 1.25 倍，波形上下各使用绘图区 40%，约占 80% 高度。
 */
static void App_PrepareWave(app_measurement_t *m)
{
    uint16_t i;
    float max_abs = 0.0f;
    float sample_position;
    float fraction;
    float window_start_sample;
    float window_span_samples;
    uint32_t index;
    uint32_t next_index;
    if (!App_FindDisplayWindow(m, &window_start_sample, &window_span_samples)) {
        /* 过零不足时保留可显示的 FFT 回退路径，避免本帧空白。 */
        window_start_sample = 0.0f;
        window_span_samples = (float)SignalADC_GetConfiguredTriggerRate() *
            (float)g_display_periods /
            (m->frequency_hz > 1.0f ? m->frequency_hz : 1.0f);
        if (window_span_samples < 1.0f) window_span_samples = 1.0f;
        if (window_span_samples > (float)(SIGNAL_SAMPLE_COUNT - 1U)) {
            window_span_samples = (float)(SIGNAL_SAMPLE_COUNT - 1U);
        }
    } else {
        window_span_samples *= (float)g_display_periods;
    }
    for (i = 0U; i < APP_WAVE_W; ++i) {
        /*
         * 像素 0 对应窗口起点、最后一个像素对应终点；对少于 312 点的高频短窗口做
         * 线性插值，既能铺满绘图区，也能在按 1~5 时真实改变横向显示周期数。
         */
        sample_position = window_start_sample + (float)i * window_span_samples /
            (float)(APP_WAVE_W - 1U);
        index = (uint32_t)sample_position;
        if (index >= SIGNAL_SAMPLE_COUNT) index = SIGNAL_SAMPLE_COUNT - 1U;
        next_index = (index + 1U < SIGNAL_SAMPLE_COUNT) ? index + 1U : index;
        fraction = sample_position - (float)index;
        {
            /* g_target_v 已是伏特单位的滤波后目标分量，无需再减直流或码值换算。 */
            g_wave_data[i] = g_target_v[index] * (1.0f - fraction) +
                g_target_v[next_index] * fraction;
        }
        if (fabsf(g_wave_data[i]) > max_abs) max_abs = fabsf(g_wave_data[i]);
    }
    m->half_range_v = 1.25f * max_abs;
    if (m->half_range_v < 0.01f) m->half_range_v = 0.01f;
}

/*
 * STEP 5 continued：局部清除旧波形后仅画黄色波形。
 * 蓝色边框属于静态界面，已在初始化时画好；擦除范围严格限制在边框内部，
 * 因此本函数不会重绘或覆盖蓝色边框、顶部标题和字段标签。
 */
static void App_DrawWave(const app_measurement_t *m)
{
    uint16_t x;
    float range = m->half_range_v;
    /* 仅擦除蓝色边框内部，保证本帧波形不会与上帧残留线段叠加。 */
    (void)TFT_ST7789_FillRect(&g_tft, APP_WAVE_X, APP_WAVE_Y,
        APP_WAVE_W, APP_WAVE_H, TFT_ST7789_BLACK);
    /* 每两个相邻采样点调用一次模块 DrawLine，形成连续折线。 */
    for (x = 1U; x < APP_WAVE_W; ++x) {
        /* 以绘图区半高为满量程：max_abs/(1.25*max_abs)=0.8，峰峰值约占 80%。 */
        int32_t y0 = APP_WAVE_Y + APP_WAVE_H / 2 -
            (int32_t)(g_wave_data[x - 1U] * (float)(APP_WAVE_H * 0.5f) / range);
        int32_t y1 = APP_WAVE_Y + APP_WAVE_H / 2 -
            (int32_t)(g_wave_data[x] * (float)(APP_WAVE_H * 0.5f) / range);
        (void)TFT_ST7789_DrawLine(&g_tft, APP_WAVE_X + x - 1U, y0,
            APP_WAVE_X + x, y1, TFT_ST7789_YELLOW);
    }
}

/*
 * STEP 3：矩阵键盘按键分发。
 * 本题应用只接收数字 1~5；实际行列扫描、消抖和鬼键过滤仍由 README 的模块完成。
 */
static void App_ProcessKey(char symbol)
{
    /* ASCII '1'~'5' 减去 '0' 后正好得到 1~5。 */
    if ((symbol >= '1') && (symbol <= '5')) {
        g_display_periods = (uint8_t)(symbol - '0');
    }
}

/*
 * 与 22_X 相同的键盘检测方法：SysTick 每 1 ms 调用，固定每 5 ms 扫描一次。
 * 这样即使主循环正在等待 ADC、执行 FFT 或通过 SPI 绘制波形，按键仍可连续获得 3 次
 * 稳定扫描以通过模块的消抖判断；中断中只更新一个字节状态，不做 FFT 或屏幕操作。
 */
void SysTick_Handler(void)
{
    static uint8_t milliseconds;
    char symbol;

    ++milliseconds;
    if (milliseconds < APP_KEYPAD_SCAN_PERIOD_MS) return;
    milliseconds = 0U;
    /* 【矩阵键盘模块】仅新出现的稳定按键返回 SIGNAL_RESULT_OK。 */
    g_key_status = SignalMatrixKeypad4x4_ReadNewSymbol(&symbol);
    if (g_key_status == SIGNAL_RESULT_OK) App_ProcessKey(symbol);
}

/*
 * STEP 2：ST7789 静态界面。
 * 上电后只完整清屏一次，再画不变的标题、F/VPP/N 标签；之后每帧不再调用 FillScreen。
 */
static void App_DrawStaticUI(void)
{
    /* 仅上电初始化时全屏清黑一次，后续刷新只改数值字段和波形框。 */
    (void)TFT_ST7789_FillScreen(&g_tft, TFT_ST7789_BLACK);
    /* 标题和三个字段标签不随采集帧改变，因此只绘制一次。 */
    (void)TFT_ST7789_DrawString(&g_tft, 4, 4, "WEAK SIGNAL", TFT_ST7789_FONT_8X16,
        TFT_ST7789_WHITE, TFT_ST7789_BLACK, false, false);
    (void)TFT_ST7789_DrawString(&g_tft, 4, 24, "F:", TFT_ST7789_FONT_8X16,
        TFT_ST7789_CYAN, TFT_ST7789_BLACK, false, false);
    (void)TFT_ST7789_DrawString(&g_tft, 132, 24, "VPP:", TFT_ST7789_FONT_8X16,
        TFT_ST7789_CYAN, TFT_ST7789_BLACK, false, false);
    (void)TFT_ST7789_DrawString(&g_tft, 4, 44, "N:", TFT_ST7789_FONT_8X16,
        TFT_ST7789_CYAN, TFT_ST7789_BLACK, false, false);
    /* 波形边框不随采集帧变化，因此只在静态界面初始化时绘制一次。 */
    (void)TFT_ST7789_DrawRect(&g_tft, APP_WAVE_X - 1, APP_WAVE_Y - 1,
        APP_WAVE_W + 2, APP_WAVE_H + 2, TFT_ST7789_BLUE);
}

/*
 * STEP 2 continued：局部刷新测量结果。
 * 每个数字字段先擦除自己的固定矩形，再使用 8x16 字库重画；这样数值由长变短时也不会残留字符，
 * 并且不会重画标题、字段标签或整块屏幕。
 */
static void App_DrawMeasurement(const app_measurement_t *m)
{
    /* F 和 N 数字位于左半屏；相位字段已按题目要求彻底取消。 */
    (void)TFT_ST7789_FillRect(&g_tft, 20, 24, 124, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_FillRect(&g_tft, 36, 44, 30, 16, TFT_ST7789_BLACK);
    /* VPP 数字位于右半屏，清除到屏幕右边界前，保留标签。 */
    (void)TFT_ST7789_FillRect(&g_tft, 172, 24, 144, 16, TFT_ST7789_BLACK);
    /* 数字内容会改变，故每帧只重画频率、峰峰值和周期数三项。 */
    (void)TFT_ST7789_DrawFloat(&g_tft, 20, 24, m->frequency_hz, 1U,
        TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&g_tft, 172, 24, m->peak_to_peak_v, 3U,
        TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawInt32(&g_tft, 36, 44, g_display_periods,
        TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
    /* 仅波形框内部是每帧变化区域；蓝色边框已在静态界面阶段固定。 */
    App_DrawWave(m);
}

int main(void)
{
    /*
     * main 的比赛执行顺序固定为：
     * 1) SysConfig 初始化底层 Timer、ADC、DMA、SPI、GPIO；
     * 2) 按单 ADC README 初始化采集模块，并准备 CMSIS-DSP FFT；
     * 3) 按 ST7789 README 初始化屏幕；
     * 4) 循环启动一帧采集，等待 DMA 完成后做频率/峰峰值测量；
     * 5) 根据键盘选择的 1~5 个周期刷新波形和测量数字。
     * 除测量、显示布局和按键映射外，main 不改动任何模块内部实现。
     */
    signal_adc_dma_config_t adc_config = {
        /* sample_rate_hz：本题要求的实际采样率，必须与 Timer 周期匹配。 */
        SIGNAL_SAMPLE_RATE_HZ,
        /* timer_clock_hz：SysConfig 中 TIMG0 为 BUSCLK/1/1，直接使用生成的 CPUCLK_FREQ。 */
        CPUCLK_FREQ,
        /* timer_max_count：16 位定时器可用的最大计数值，交给模块检查分频/重装值。 */
        65536U
    };
    app_measurement_t measurement;
    signal_result_t result;

    /* 【SysConfig 生成代码】一次性打开系统时钟、引脚复用和所有外设。 */
    SYSCFG_DL_init();

    /*
     * STEP 1：单 ADC + DMA 采集。
     * README 规定的顺序是 Init 只调用一次，随后每帧 SetSampleRate、Start；
     * main 只提供缓冲区和配置，不直接写 ADC/DMA 寄存器。
     */
    result = SignalADC_Init(&adc_config);
    if (result != SIGNAL_RESULT_OK) App_Fail();

    /* 【CMSIS-DSP】为 1024 点实数 FFT 分配/初始化内部参数。 */
    if (arm_rfft_fast_init_f32(&g_fft, SIGNAL_SAMPLE_COUNT) != ARM_MATH_SUCCESS) App_Fail();

    /*
     * STEP 2：ST7789 显示模块。
     * MSPM0 适配层把 README 中的 SPI/GPIO 实例绑定到 g_tft；旋转 90 度后，
     * 屏幕有效坐标为 320×240，正好容纳顶部测量值和下方波形绘图区。
    */
    if (SignalTFTST7789_MSPM0_Init(&g_tft, TFT_ST7789_ROTATION_270, 0U, 0U) != TFT_ST7789_OK) App_Fail();
    App_DrawStaticUI();
    /* 与 22_X 一致，建立 1 ms SysTick；键盘扫描周期由 APP_KEYPAD_SCAN_PERIOD_MS 控制。 */
    if (SysTick_Config(CPUCLK_FREQ / 1000U) != 0U) App_Fail();

    while (1) {
        /*
         * STEP 1 continued：每一帧采集前重新写入采样率。
         * 这样即使后续需要修改采样率，模块仍会按当前配置重新计算 Timer 参数。
         */
        result = SignalADC_SetSampleRate(SIGNAL_SAMPLE_RATE_HZ);
        if (result != SIGNAL_RESULT_OK) App_Fail();

        /* 启动单 ADC DMA；g_raw 是本题唯一混合输入波形和显示波形的来源。 */
        result = SignalADC_Start(g_raw, SIGNAL_SAMPLE_COUNT);
        if (result != SIGNAL_RESULT_OK) App_Fail();

        /*
         * DMA 完成标志由采集模块的中断置位；等待时执行 WFI，避免 CPU 忙等。
         * 单通道 DMA 完成后才离开循环，此时 g_raw 的 1024 点全部有效。
         */
        while (!SignalADC_IsFinished()) __WFI();

        /* STEP 4：在 10~100 kHz 频带内搜索最大 FFT 峰值并计算 F/A/P。 */
        App_Measure(g_raw, &measurement);
        if (measurement.valid) {
            /* STEP 5：按当前周期数准备 312 个像素点和自动电压量程。 */
            App_PrepareWave(&measurement);
            /* STEP 2 continued：用 README 的 ST7789 8×16 字库 API 刷新界面。 */
            App_DrawMeasurement(&measurement);
        }

        /* 键盘已由 SysTick 固定每 5 ms 扫描，主循环立即进入下一帧采集以提高刷新速度。 */
    }
}
