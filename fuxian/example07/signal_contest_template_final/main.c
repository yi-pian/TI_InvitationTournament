/*
 * example07：未知信号通道自动补偿装置。
 *
 * 硬件数据链：DDS/DAC 输出测试或预补偿波形 -> 未知线性网络 ->
 * ADC0 采集网络输入、ADC1 采集网络输出。ADC 两路由同一 TIMER 触发，
 * 因而可以用同一个 Lock-In 参考计算幅值比和相位差。
 *
 * 比赛现场按以下大步骤逐项完成：
 *   1. 双 ADC 同步采集；
 *   2. ST7789 + 8x16 字库显示；
 *   3. 4x4 矩阵键盘操作；
 *   4. 0.5 kHz～10 kHz 自动扫频，得到幅频/相频；
 *   5. 1 kHz 逆响应预补偿并重新测量误差；
 *   6. 1/2/3 次谐波分析、合成和补偿。
 *
 * 模块 API 按 modules/README.md 复制调用；本文件只放题目组合逻辑，
 * 不修改 modules 目录中的任何 .c/.h/.inc 文件。
 */
/* 大步骤 1：复制各模块 README 的头文件和题目参数。 */
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include "ti_msp_dl_config.h"
#include "signal_config.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_dac_dma_mspm0g3507.h"
#include "signal_dac_wave_table.h"
#include "signal_sine.h"
#include "signal_dds.h"
#include "signal_frequency_sweep.h"
#include "signal_lock_in.h"
#include "signal_frequency_response_correction.h"
#include "signal_matrix_keypad_4x4.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_font.h"
#include "signal_tft_st7789_mspm0g3507.h"

#define APP_PI (3.14159265358979323846f) /* 三谐波合成使用的圆周率。 */
#define APP_BASE_AMPLITUDE (0.22f)        /* DAC 满量程比例，留出上下余量。 */
/* 本题频率参数放在 main：不改 signal_config.h 母版的默认题目参数。 */
#define APP_SWEEP_MIN_HZ (500.0f)
#define APP_SWEEP_MAX_HZ (10000.0f)
#define APP_TARGET_FREQUENCY_HZ (1000.0f)
#define APP_HARMONIC_BASE_HZ (1000.0f)
/* 复制 22_X：SysTick 每 1 ms 进入一次，累计 5 ms 再扫描矩阵键盘。 */
#define APP_KEYPAD_SCAN_PERIOD_MS (5U)
/*
 * 每个输出周期固定 100 个更新点：1 kHz 时为 100 kS/s，阶梯比原 50 点
 * 减半；10 kHz 时正好为片内 DAC 可用的 1 MSPS 上限，仍覆盖题目频段。
 */
#define APP_WAVE_SAMPLES_PER_PERIOD (100U)

/* 每个响应点保存未知网络 H(f)=输出/输入 的幅值和相位。 */
typedef struct { float gain; float phase_deg; uint8_t valid; } app_response_t;

/* 大步骤 1：双 ADC 原始 DMA 缓冲；输入和输出必须一一对应。 */
static uint16_t g_in_raw[SIGNAL_SAMPLE_COUNT], g_out_raw[SIGNAL_SAMPLE_COUNT];
/* 大步骤 4～6：DDS 查表源与 DMA 输出缓冲分开，避免 DMA 运行时改写源表。 */
static uint16_t g_lut[SIGNAL_DAC_TABLE_COUNT];
static uint16_t g_dac[SIGNAL_DAC_TABLE_COUNT];
/* Lock-In 不能直接使用 ADC 码，下面两个数组保存换算后的电压。 */
static float g_in_v[SIGNAL_SAMPLE_COUNT], g_out_v[SIGNAL_SAMPLE_COUNT];
/* 扫频模块生成的频点，以及每个频点测得的响应。 */
static float g_sweep_hz[SIGNAL_SWEEP_POINTS];
static app_response_t g_response[SIGNAL_SWEEP_POINTS];
/* 波表对象由 Sine 模块填写；DDS 再从波表生成 DAC DMA 缓冲。 */
static signal_dac_wave_table_t g_wave = { g_lut, SIGNAL_DAC_TABLE_COUNT, SIGNAL_DAC_BITS };
static signal_dds_t g_dds;
/* ST7789 对象和键盘中断与主循环之间共享的状态。 */
static tft_st7789_t g_tft;
static volatile char g_key;
static volatile uint8_t g_key_pending;
/* 与 22_X 相同：保存最近一次键盘模块返回值，便于上板观察按键状态。 */
static volatile signal_result_t g_keypad_status;
/* 页面：0=扫频特性，1=1 kHz 误差，2=谐波补偿。 */
static uint8_t g_page, g_sweep_done, g_harmonic_done;
static float g_gain_error, g_phase_error;

/*
 * 大步骤 4：按 DDS 补充 README 的三段调用顺序输出一个频点。
 * ① SignalSine_Generate 生成一周期查表；
 * ② SignalDDS_Init 依据输出更新率计算频率调谐字；
 * ③ SignalDDS_Fill 按调谐字重采样 100 点到 g_dac，供 DAC DMA 循环播放。
 *
 * DAC 更新率固定采用“输出频率 × 100”，因此 DMA 缓冲正好是一整周期；
 * 这避免了任意 DDS 相位在 1024 点缓冲边界重复时产生不连续跳变。
 * actual_frequency_hz 非空时返回定时器量化后的实际输出频率。
 */
static uint8_t App_GenerateDDS(float frequency_hz, float amplitude_fraction,
    float phase_cycles, float *actual_frequency_hz)
{
    uint32_t requested_update_rate_hz;
    uint32_t actual_update_rate_hz;
    float actual_hz;
    if (!(frequency_hz > 0.0f)) return 0U;
    requested_update_rate_hz = (uint32_t)(frequency_hz *
        (float)APP_WAVE_SAMPLES_PER_PERIOD + 0.5f);
    if (SignalDACDMA_MSPM0_SetUpdateRate(requested_update_rate_hz) !=
            SIGNAL_RESULT_OK) return 0U;
    actual_update_rate_hz = SignalDACDMA_MSPM0_GetConfiguredRate();
    if (actual_update_rate_hz == 0U) return 0U;
    actual_hz = (float)actual_update_rate_hz /
        (float)APP_WAVE_SAMPLES_PER_PERIOD;
    /* 先写 g_lut；g_dac 此时仍是上一次输出，DMA 尚未启动。 */
    if (SignalSine_Generate(&g_wave, 0.5f, amplitude_fraction, phase_cycles) !=
            SIGNAL_RESULT_OK) return 0U;
    /* DDS 的频率和 DAC Timer 实际更新率严格匹配，刚好一周期/100 点。 */
    if (SignalDDS_Init(&g_dds, g_lut, SIGNAL_DAC_TABLE_COUNT, actual_hz,
            (float)actual_update_rate_hz, 0U) != SIGNAL_RESULT_OK) return 0U;
    if (SignalDDS_Fill(&g_dds, g_dac, APP_WAVE_SAMPLES_PER_PERIOD) !=
            SIGNAL_RESULT_OK) return 0U;
    if (actual_frequency_hz != NULL) *actual_frequency_hz = actual_hz;
    return 1U;
}

/*
 * 大步骤 1：复制 dual_adc README 的 Start/IsFinished 闭环。
 * Start 会同时打开 ADC0/ADC1 的定时触发和 DMA；采满指定点数后，
 * IsFinished 才返回真。等待期间使用 __WFI，避免主循环忙等。
 */
static uint8_t App_Capture(void)
{
    /* g_in_raw 对应网络输入，g_out_raw 对应网络输出。 */
    if (SignalDualADC_Start(g_in_raw, g_out_raw, SIGNAL_SAMPLE_COUNT) != SIGNAL_RESULT_OK) return 0U;
    /* DMA 完成由模块内部中断置位，主循环只等待状态。 */
    while (!SignalDualADC_IsFinished()) { __WFI(); }
    return 1U;
}

/*
 * 大步骤 4：复制 lock_in README，输入/输出使用相同的频率参考做正交积分。
 * 输出的 gain=|H(f)|，phase_deg=∠H(f)，后续扫频和补偿都只使用这个结构体。
 */
static uint8_t App_MeasureResponse(float frequency_hz, app_response_t *response)
{
    /* phase_offset=0，remove_mean=1：去掉 ADC 偏置后再进行正交积分。 */
    signal_lock_in_config_t cfg = { frequency_hz, SIGNAL_SAMPLE_RATE_HZ, 0.0f, 1U };
    signal_lock_in_result_t in_result, out_result;
    uint32_t i;
    if (!App_Capture()) return 0U;
    /* 12 位 ADC 码 -> 0～VREF 电压；Lock-In 输入必须是 float 电压数组。 */
    for (i = 0U; i < SIGNAL_SAMPLE_COUNT; ++i) {
        g_in_v[i] = (float)g_in_raw[i] * SIGNAL_ADC_VREF_V / 4095.0f;
        g_out_v[i] = (float)g_out_raw[i] * SIGNAL_ADC_VREF_V / 4095.0f;
    }
    /* 分别求输入、输出的峰值和相位；同一 cfg 保证参考相位一致。 */
    if (SignalLockIn_Process(g_in_v, SIGNAL_SAMPLE_COUNT, &cfg, &in_result) != SIGNAL_ALGORITHM_OK) return 0U;
    if (SignalLockIn_Process(g_out_v, SIGNAL_SAMPLE_COUNT, &cfg, &out_result) != SIGNAL_ALGORITHM_OK) return 0U;
    /* 输入过小会使幅值比和相位没有意义。 */
    if (in_result.amplitude_peak_v < 1.0e-5f) return 0U;
    /* 未知通道的传递函数：输出幅值/输入幅值，输出相位-输入相位。 */
    response->gain = out_result.amplitude_peak_v / in_result.amplitude_peak_v;
    response->phase_deg = out_result.phase_deg - in_result.phase_deg;
    /* 把相位统一包络到 [-180°,180°]，便于显示和求负相位补偿。 */
    while (response->phase_deg > 180.0f) response->phase_deg -= 360.0f;
    while (response->phase_deg < -180.0f) response->phase_deg += 360.0f;
    response->valid = 1U;
    return 1U;
}

/*
 * 大步骤 4：复制 frequency_sweep README 的 Generate 调用。
 * 先得到 16 个对数频点，再逐点执行“DDS 输出 -> DAC DMA -> 双 ADC -> Lock-In”。
 * 每个点停止 DMA 后才修改下一张波表，避免异步 DMA 读写冲突。
 */
static void App_RunSweep(void)
{
    signal_frequency_sweep_config_t cfg = { APP_SWEEP_MIN_HZ, APP_SWEEP_MAX_HZ,
                                            SIGNAL_SWEEP_POINTS, true };
    uint32_t i;
    /* 若上一项补偿仍在连续输出，先停 DMA，才能重设 DAC Timer 频率。 */
    SignalDACDMA_MSPM0_Stop();
    /* 0.5 kHz～10 kHz 对数分布，适合查看未知网络拐点。 */
    (void)SignalFrequencySweep_Generate(&cfg, g_sweep_hz, SIGNAL_SWEEP_POINTS);
    for (i = 0U; i < SIGNAL_SWEEP_POINTS; ++i) {
        /* 当前频点写入 DAC 缓冲；失败点跳过，不使用旧缓冲重复测量。 */
        if (!App_GenerateDDS(g_sweep_hz[i], APP_BASE_AMPLITUDE, 0.0f,
                &g_sweep_hz[i])) continue;
        /* 每个频点用“频率×100”的定时器更新率播放 100 点完整周期。 */
        (void)SignalDACDMA_MSPM0_Start(g_dac, APP_WAVE_SAMPLES_PER_PERIOD, true);
        /* ADC 采集和 Lock-In 都使用当前频点作为参考。 */
        (void)App_MeasureResponse(g_sweep_hz[i], &g_response[i]);
        /* 当前点采样结束后停止 DMA，再生成下一个频点。 */
        SignalDACDMA_MSPM0_Stop();
    }
    g_sweep_done = 1U;
}

/*
 * 大步骤 5：复制 frequency_response_correction README。
 * 把测量到的 H(f) 转成 1/H(f)，再在目标 1 kHz 处做对数插值，
 * 得到需要施加到输入端的增益和相位预补偿。
 */
static void App_Run1kCompensation(void)
{
    signal_frequency_response_correction_point_t table[SIGNAL_SWEEP_POINTS];
    signal_frequency_response_correction_result_t correction;
    app_response_t measured = {0};
    float actual_frequency_hz = 0.0f;
    uint32_t i;
    /* 清除可能正在播放的谐波补偿波，避免 SetUpdateRate 返回 BUSY。 */
    SignalDACDMA_MSPM0_Stop();
    /* 用户直接按 2 时，先自动完成扫频，保证逆响应表有数据。 */
    if (!g_sweep_done) App_RunSweep();
    for (i = 0U; i < SIGNAL_SWEEP_POINTS; ++i) {
        /* 增益取倒数，相位取负号；无效点使用单位补偿作为保护值。 */
        table[i].frequency_hz = g_sweep_hz[i];
        table[i].gain_correction_linear = (g_response[i].valid && g_response[i].gain > 0.001f) ?
            1.0f / g_response[i].gain : 1.0f;
        table[i].phase_correction_deg = g_response[i].valid ? -g_response[i].phase_deg : 0.0f;
    }
    /* 在 1 kHz 对数插值，超出范围时采用边界点夹紧。 */
    if (SignalFrequencyResponseCorrection_Process(table, SIGNAL_SWEEP_POINTS,
            APP_TARGET_FREQUENCY_HZ, 1.0f, 0.0f, SIGNAL_FRC_INTERPOLATE_LOG_HZ,
            SIGNAL_FRC_RANGE_CLAMP, &correction) != SIGNAL_ALGORITHM_OK) return;
    /* 防止未知网络深衰减时所需增益过大，造成 DAC 饱和。 */
    if (correction.applied_gain_correction_linear < SIGNAL_COMP_GAIN_MIN)
        correction.applied_gain_correction_linear = SIGNAL_COMP_GAIN_MIN;
    if (correction.applied_gain_correction_linear > SIGNAL_COMP_GAIN_MAX)
        correction.applied_gain_correction_linear = SIGNAL_COMP_GAIN_MAX;
    /* 生成带有逆增益和负相位的 1 kHz 预补偿信号。 */
    if (!App_GenerateDDS(APP_TARGET_FREQUENCY_HZ,
            APP_BASE_AMPLITUDE * correction.applied_gain_correction_linear,
            correction.applied_phase_correction_deg / 360.0f,
            &actual_frequency_hz)) return;
    (void)SignalDACDMA_MSPM0_Start(g_dac, APP_WAVE_SAMPLES_PER_PERIOD, true);
    /* 再走一次同样的采集链，得到实际幅值误差和相位误差。 */
    if (App_MeasureResponse(actual_frequency_hz, &measured)) {
        g_gain_error = (measured.gain - 1.0f) * 100.0f;
        g_phase_error = measured.phase_deg;
    }
    /*
     * 复测已完成，但不要停 DAC：题目要求的预补偿 1 kHz 信号应继续
     * 从 PA15 输出，便于示波器观察和接入未知网络。下一次按 1/2/3
     * 会在各自函数开头停止当前 DMA，再切换到新的测试任务。
     */
}

/*
 * 大步骤 6：README 没有“三谐波合成”单独接口，因此只增加少量题目逻辑。
 * 先分别测量 1 kHz、2 kHz、3 kHz 的增益和相位，再按目标权重
 * (1、1/2、1/3) 除以实测增益，并把实测相位取负，合成一周期查表。
 */
static void App_RunHarmonics(void)
{
    const float f[3] = { APP_HARMONIC_BASE_HZ, 2.0f * APP_HARMONIC_BASE_HZ,
                          3.0f * APP_HARMONIC_BASE_HZ };
    float precomp_gain[3] = { 1.0f, 1.0f / 2.0f, 1.0f / 3.0f };
    float actual_f[3] = { 0.0f };
    app_response_t r[3] = {{0}};
    uint32_t i;
    /* 若 1 kHz 补偿仍在输出，先停止，之后才能依次设为 1/2/3 kHz。 */
    SignalDACDMA_MSPM0_Stop();
    /* 第一轮：逐个输出三个频率，得到三组通道响应。 */
    for (i = 0U; i < 3U; ++i) {
        if (!App_GenerateDDS(f[i], APP_BASE_AMPLITUDE, 0.0f,
                &actual_f[i])) continue;
        (void)SignalDACDMA_MSPM0_Start(g_dac, APP_WAVE_SAMPLES_PER_PERIOD, true);
        (void)App_MeasureResponse(actual_f[i], &r[i]);
        SignalDACDMA_MSPM0_Stop();
        /* 目标幅度/通道增益就是预补偿幅度；限制最大补偿避免溢出。 */
        if (r[i].valid && r[i].gain > 0.001f) precomp_gain[i] /= r[i].gain;
        if (precomp_gain[i] > SIGNAL_COMP_GAIN_MAX) precomp_gain[i] = SIGNAL_COMP_GAIN_MAX;
    }
    /* 第二轮：在 1024 点一周期查表中叠加三个已补偿谐波。 */
    for (i = 0U; i < SIGNAL_DAC_TABLE_COUNT; ++i) {
        /* 三项绝对值之和用于归一化，保证 normalized 不超出 [-1,1]。 */
        const float norm = precomp_gain[0] + precomp_gain[1] + precomp_gain[2];
        float t = 2.0f * APP_PI * (float)i / (float)SIGNAL_DAC_TABLE_COUNT;
        /*
         * 目标定义与 SignalSine_Generate 一致：每一项均以 sin(nωt) 为
         * 0° 相位基准。不能使用 cos() 代替：对 n=1/2/3 三项分别加
         * 90° 会改变相对相位，合成形状就会从目标曲线变成尖峰曲线。
         * “-实测相位”仍是未知网络 H(f) 的相位预补偿。
         */
        float n = precomp_gain[0] * sinf(t - (r[0].valid ? r[0].phase_deg : 0.0f) * APP_PI / 180.0f);
        n += precomp_gain[1] * sinf(2.0f * t - (r[1].valid ? r[1].phase_deg : 0.0f) * APP_PI / 180.0f);
        n += precomp_gain[2] * sinf(3.0f * t - (r[2].valid ? r[2].phase_deg : 0.0f) * APP_PI / 180.0f);
        (void)SignalDACWaveTable_NormalizedToRaw(n / norm, SIGNAL_DAC_BITS,
            0.5f, APP_BASE_AMPLITUDE, &g_lut[i]);
    }
    /* 合成查表以基波 1 kHz 播放，谐波频率由查表中的 2t、3t 体现。 */
    if (SignalDACDMA_MSPM0_SetUpdateRate((uint32_t)(APP_HARMONIC_BASE_HZ *
            (float)APP_WAVE_SAMPLES_PER_PERIOD + 0.5f)) != SIGNAL_RESULT_OK) return;
    {
        float actual_base_hz = (float)SignalDACDMA_MSPM0_GetConfiguredRate() /
            (float)APP_WAVE_SAMPLES_PER_PERIOD;
        if (SignalDDS_Init(&g_dds, g_lut, SIGNAL_DAC_TABLE_COUNT,
                actual_base_hz, SignalDACDMA_MSPM0_GetConfiguredRate(), 0U) !=
                SIGNAL_RESULT_OK) return;
    }
    if (SignalDDS_Fill(&g_dds, g_dac, APP_WAVE_SAMPLES_PER_PERIOD) != SIGNAL_RESULT_OK) return;
    (void)SignalDACDMA_MSPM0_Start(g_dac, APP_WAVE_SAMPLES_PER_PERIOD, true);
    g_harmonic_done = 1U;
}

/*
 * 大步骤 2：复制 ST7789 README 的 8x16 字库调用。
 * 第 0 页显示扫频代表点的频率、增益、相位；第 1 页显示 1 kHz
 * 误差；第 2 页显示三谐波补偿状态。A/D 键在三页之间翻页。
 */
static void App_DrawPage(void)
{
    uint32_t row;
    static const uint32_t display_index[4] = { 0U, 5U, 10U, 15U };
    (void)TFT_ST7789_FillScreen(&g_tft, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawString(&g_tft, 4, 4, "UNKNOWN CHANNEL COMP", TFT_ST7789_FONT_8X16,
        TFT_ST7789_CYAN, TFT_ST7789_BLACK, false, false);
    (void)TFT_ST7789_DrawString(&g_tft, 4, 24, "1 SWEEP  2 1K   3 HARM", TFT_ST7789_FONT_8X16,
        TFT_ST7789_WHITE, TFT_ST7789_BLACK, false, false);
    if (g_page == 0U && g_sweep_done) {
        /* 16 个点全部保存在 RAM，这里选 1、6、11、16 点快速显示。 */
        (void)TFT_ST7789_DrawString(&g_tft, 4, 52, "FREQ Hz   GAIN   PHASE", TFT_ST7789_FONT_8X16,
            TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false, false);
        for (row = 0U; row < 4U; ++row) {
            uint32_t index = display_index[row];
            int16_t y = (int16_t)(76 + row * 24U);
            (void)TFT_ST7789_DrawFloat(&g_tft, 4, y, g_sweep_hz[index], 0,
                TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
            (void)TFT_ST7789_DrawFloat(&g_tft, 120, y, g_response[index].gain, 3,
                TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
            (void)TFT_ST7789_DrawFloat(&g_tft, 220, y, g_response[index].phase_deg, 1,
                TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
        }
    } else if (g_page == 1U) {
        (void)TFT_ST7789_DrawString(&g_tft, 4, 52, "1k GainErr% PhaseErr", TFT_ST7789_FONT_8X16,
            TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false, false);
        (void)TFT_ST7789_DrawFloat(&g_tft, 4, 76, g_gain_error, 2, TFT_ST7789_FONT_8X16,
            TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
        (void)TFT_ST7789_DrawFloat(&g_tft, 160, 76, g_phase_error, 2, TFT_ST7789_FONT_8X16,
            TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    } else {
        (void)TFT_ST7789_DrawString(&g_tft, 4, 52, g_harmonic_done ? "1/2/3 COMPENSATED" : "PRESS 3 TO RUN",
            TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false, false);
    }
    /* 最后一条扫频数据位于 y=148，翻页提示放到底部避免遮挡数据。 */
    (void)TFT_ST7789_DrawString(&g_tft, 4, 216, "A/D PAGE", TFT_ST7789_FONT_8X16,
        TFT_ST7789_MAGENTA, TFT_ST7789_BLACK, false, false);
}

/*
 * 大步骤 3：按 22_X 的 SysTick 扫描方法复制 keypad README 的扫描接口。
 * SysTick 每 1 ms 进入一次，静态毫秒计数达到 5 ms 后扫描一次键盘。
 * 22_X 的简单按键处理可直接留在中断；本题的按键会触发扫频、Lock-In 和
 * ST7789 绘图，耗时太长，因此中断只保存 symbol 和 g_keypad_status，实际
 * App_ProcessKey 仍在主循环运行，避免 DMA 采集和 SysTick 被长时间阻塞。
 */
void SysTick_Handler(void)
{
    static uint8_t milliseconds;
    char symbol;
    ++milliseconds;
    if (milliseconds < APP_KEYPAD_SCAN_PERIOD_MS) return;
    milliseconds = 0U;
    /* 22_X 同款：模块完成行列扫描、3 次消抖和鬼键过滤后返回新按键。 */
    g_keypad_status = SignalMatrixKeypad4x4_ReadNewSymbol(&symbol);
    if (g_keypad_status == SIGNAL_RESULT_OK) {
        g_key = symbol;
        g_key_pending = 1U;
    }
}

static void App_ProcessKey(void)
{
    char key;
    if (g_key_pending == 0U) return;
    key = g_key;
    g_key_pending = 0U;
    /* A/D：循环上一页/下一页；首页按 A 也会跳到第 2 页，便于确认按键有效。 */
    if (key == 'A') g_page = (g_page == 0U) ? 2U : (uint8_t)(g_page - 1U);
    else if (key == 'D') g_page = (g_page >= 2U) ? 0U : (uint8_t)(g_page + 1U);
    else if (key == '1') App_RunSweep();
    else if (key == '2') App_Run1kCompensation();
    else if (key == '3') App_RunHarmonics();
    App_DrawPage();
}

int main(void)
{
    /* 外设时钟由 SysConfig 生成；ADC、DAC DMA 使用相同的 CPU 时钟基准。 */
    const signal_dual_adc_config_t adc_cfg = { SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U };
    const signal_dac_dma_mspm0_config_t dac_cfg = { SIGNAL_DAC_UPDATE_RATE_HZ, CPUCLK_FREQ, 65536U };
    /* 第一步必须先执行生成的 SysConfig 初始化，之后才能访问外设宏。 */
    SYSCFG_DL_init();
    /* 初始化双 ADC；失败时停机，避免用未完成的采样数据计算。 */
    if (SignalDualADC_Init(&adc_cfg) != SIGNAL_RESULT_OK) while (1) { }
    /* 初始化 DAC Timer、FIFO、DMA 和 DMA_DONE 中断。 */
    if (SignalDACDMA_MSPM0_Init(&dac_cfg) != SIGNAL_RESULT_OK) while (1) { }
    /* 初始化 ST7789 SPI 和 8x16 字库适配层。 */
    if (SignalTFTST7789_MSPM0_Init(&g_tft, TFT_ST7789_ROTATION_270, 0U, 0U) != TFT_ST7789_OK) while (1) { }
    /* 启动前检查波表对象；真正的波形由 App_GenerateDDS 再填写。 */
    (void)SignalDACWaveTable_Validate(&g_wave);
    /* 清屏并配置 1 ms SysTick，键盘扫描在 SysTick_Handler 中完成。 */
    (void)TFT_ST7789_FillScreen(&g_tft, TFT_ST7789_BLACK);
    if (SysTick_Config(CPUCLK_FREQ / 1000U) != 0U) while (1) { }
    /* 显示待机页面；用户按键后才启动耗时的扫频/补偿流程。 */
    App_DrawPage();
    /* 主循环只处理按键和低功耗等待，具体算法在按键触发时执行。 */
    while (1) { App_ProcessKey(); __WFI(); }
}
