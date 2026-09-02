/* example04 可编程综合测量仪分八步：波表-DDS-DAC 输出、双 ADC 基本测量、三路
 * 测频、FFT 谐波、鲁棒测量、Sine Fit/Lock-In、捕获回放、系统校准。
 * 算法和驱动均按集成库 README 调用；App_* 只写模块之间的参数状态机、键盘队列、
 * 8x16 文字和局部刷新。默认频率/幅度/偏置改 APP_DEFAULT_*，采样率策略改
 * App_SelectSampleRate()，硬件资源必须回 SysConfig 修改。 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>

#include "ti_msp_dl_config.h"
#include "signal_config.h"

/* `00_docs/recipes` 中的直接 Recipe 使用 SDK 自带 CMSIS-DSP API；
 * 普通测量不再依赖旧工程兼容封装，所需头文件和链接项由当前工程已有
 * 的 CMSIS 配置提供。 */
#include "arm_const_structs.h"
#include "arm_math.h"

#include "signal_dual_adc_mspm0g3507.h"
#include "signal_dac_dma_mspm0g3507.h"
#include "signal_dac_wave_table.h"
#include "signal_wave_output_mspm0g3507.h"

#include "signal_zero_cross.h"
#include "signal_zero_cross_interpolation.h"

#include "signal_window.h"
#include "signal_fft_parabolic_interpolation.h"
#include "signal_window_gain_correction.h"
#include "signal_harmonic.h"
#include "signal_thd.h"
#include "signal_snr.h"
#include "signal_sfdr.h"

#include "signal_mad.h"
#include "signal_median_filter.h"
#include "signal_hampel.h"
#include "signal_robust_peak_to_peak.h"
#include "signal_robust_rms.h"
#include "signal_sine_fit_3param.h"
#include "signal_sine_fit_4param.h"
#include "signal_lock_in.h"
#include "signal_trigger_capture.h"
#include "signal_single_capture_replay.h"
#include "signal_adc_gain_offset_calibration.h"
#include "signal_channel_delay_calibration.h"
#include "signal_comparator_zero_cross.h"
#include "signal_timer_capture.h"
#include "signal_timer_capture_mspm0g3507.h"

#include "signal_matrix_keypad_4x4.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_mspm0g3507.h"
#include "signal_tft_st7789_font.h"

#define APP_PAGE_COUNT          (7U)
#define APP_GRAPH_X             (8)
#define APP_GRAPH_Y             (112)
#define APP_GRAPH_W             (304)
#define APP_GRAPH_H             (90)
#define APP_ADC_MAX_CODE        (4095U)
#define APP_DEFAULT_FREQUENCY   (1000U)
#define APP_DEFAULT_AMPLITUDE_MV (600U)
#define APP_DEFAULT_OFFSET_MV   (1650U)
/* 形状参数使用 0～1 小数；方波默认 50%，锯齿波默认 100%（标准上升锯齿）。 */
#define APP_DEFAULT_SQUARE_DUTY       (0.3f)
#define APP_DEFAULT_SAW_SYMMETRY      (0.001f)
#define APP_INPUT_CAPACITY      (8U)
#define APP_KEYPAD_SCAN_MS      (5U)
#define APP_KEY_QUEUE_SIZE      (8U)
#define APP_DISPLAY_PERIOD_MS   (250U)

typedef enum {
    APP_WAVE_SINE = 0U,
    APP_WAVE_SQUARE,
    APP_WAVE_TRIANGLE,
    APP_WAVE_SAWTOOTH
} app_waveform_t;

typedef enum {
    APP_FILTER_RAW = 0U,
    APP_FILTER_MEDIAN,
    APP_FILTER_HAMPEL
} app_filter_t;

typedef enum {
    APP_EDIT_NONE = 0U,
    APP_EDIT_FREQUENCY,
    APP_EDIT_AMPLITUDE_MV,
    APP_EDIT_OFFSET_MV
} app_edit_target_t;

/* 数据缓冲区：g_raw_ch1/ch2 是双 ADC 原始码；g_capture_slots 是三次单次捕获；g_wave_table 和
 * g_dac_output 是 DAC 波表/输出缓冲；g_voltage、g_calibrated、g_centered 依次表示
 * 原始换算电压、增益/偏置校准电压、去直流电压；g_magnitude 是单边频谱幅值；
 * 后面的 g_* 变量分别保存各算法输出。参数变量 g_output_frequency_hz、
 * g_amplitude_mv、g_offset_mv 由键盘修改，g_page/g_waveform/g_filter/g_window 表示
 * 页面、波形、抗干扰模式和窗函数，g_input 是数字预输入缓存。 */
/* 额外容量供捕获页的连续三缓冲 DMA 使用；普通测量仍只处理前 512 点。 */
static uint16_t g_raw_ch1[SIGNAL_SINGLE_CAPTURE_MAX_SAMPLES *
    SIGNAL_SINGLE_CAPTURE_DMA_BLOCKS];
static uint16_t g_raw_ch2[SIGNAL_SINGLE_CAPTURE_MAX_SAMPLES *
    SIGNAL_SINGLE_CAPTURE_DMA_BLOCKS];
/* 连续 DMA 的相邻两块拼接窗口：ADC0(PA25)原始波形用于
 * 定位 COMP0(PA27)已经报告的门限跨越。 */
static uint16_t g_capture_search[SIGNAL_SINGLE_CAPTURE_MAX_SAMPLES * 2U];
/* 三个单次波形槽位；每个槽位最多保存 416 点（1 MSPS 下为 416 us）。 */
static uint16_t g_capture_slots[SIGNAL_SINGLE_CAPTURE_SLOTS]
    [SIGNAL_SINGLE_CAPTURE_MAX_SAMPLES];
static uint16_t g_capture_lengths[SIGNAL_SINGLE_CAPTURE_SLOTS];
static uint32_t g_capture_rates_hz[SIGNAL_SINGLE_CAPTURE_SLOTS];
static bool g_capture_slot_valid[SIGNAL_SINGLE_CAPTURE_SLOTS];
static signal_single_capture_replay_t g_single_capture;
static uint16_t g_wave_table[SIGNAL_DAC_TABLE_COUNT];
/* DAC DMA 输出缓冲：长度按“实际 DAC 更新率 / 输出频率”动态使用，
 * 512 点容量覆盖 100 kHz 更新率下 200 Hz~20 kHz 的单周期数据。 */
static uint16_t g_dac_output[SIGNAL_DAC_OUTPUT_COUNT];
static float g_voltage[SIGNAL_SAMPLE_COUNT];
static float g_calibrated[SIGNAL_SAMPLE_COUNT];
static float g_centered[SIGNAL_SAMPLE_COUNT];
static float g_workspace[SIGNAL_SAMPLE_COUNT];
static float g_magnitude[(SIGNAL_SAMPLE_COUNT / 2U) + 1U];
/* `cmsis_fft_spectrum.md` 的工作区：g_fft_q15 按实部、虚部交错保存
 * Q15 CFFT 的输入和输出；g_fft_magnitude_q15 保存每个复数 bin 的 Q15
 * 幅值，随后恢复为伏特量纲写入 g_magnitude。 */
static q15_t g_fft_q15[2U * SIGNAL_SAMPLE_COUNT];
static q15_t g_fft_magnitude_q15[SIGNAL_SAMPLE_COUNT];
static signal_zero_cross_event_t g_zero_events[SIGNAL_ZERO_EVENT_CAPACITY];
static float g_crossing_positions[SIGNAL_ZERO_EVENT_CAPACITY];

/* 算法结果变量字典：
 * g_dds：DDS 相位累加器状态；g_tft：ST7789 设备句柄；
 * g_adc_calibration：ADC 比例/零点校准；g_delay_calibration：CH2 相对 CH1 固定延迟；
 * g_fit3/g_fit4：三参数/四参数正弦拟合；g_lock_in：锁相 I/Q、幅度、相位；
 * g_capture_frequency：硬件捕获频率；g_fft_peak：整数 bin 粗频率；
 * g_fft_interpolated：抛物线插值频率；g_thd/g_snr/g_sfdr：频谱质量指标；
 * g_robust_vpp/g_robust_rms/g_mad：抗毛刺结果；g_mean_v/g_min_v/g_max_v、
 * g_vpp_v/g_rms_v/g_ac_rms_v/g_population_stddev_v/g_is_clipped：由 Recipe
 * 直接得到的基本测量结果。
 * 这些变量不是可调参数，不要手工赋测量值；应修改对应模块 config。 */
static tft_st7789_t g_tft;
static signal_adc_gain_offset_calibration_t g_adc_calibration = {1.0f, 0.0f};
static signal_channel_delay_calibration_t g_delay_calibration = {0.0f};
static signal_sine_fit_3param_result_t g_fit3;
static signal_sine_fit_4param_result_t g_fit4;
static signal_lock_in_result_t g_lock_in;
static signal_timer_capture_mspm0_result_t g_capture_frequency;
static struct {
    uint16_t bin;
    float peak_value;
    float frequency_hz;
} g_fft_peak;
static signal_fft_parabolic_result_t g_fft_interpolated;
/* 保存每次 FFT 的各阶谐波结果，供频谱页显示 H2/H3。 */
static signal_harmonic_result_t g_harmonics;
static signal_thd_result_t g_thd;
static signal_snr_result_t g_snr;
static signal_sfdr_result_t g_sfdr;
static signal_robust_peak_to_peak_result_t g_robust_vpp;
static signal_robust_rms_result_t g_robust_rms;
static signal_mad_result_t g_mad;
static float g_mean_v;
static float g_min_v;
static float g_max_v;
static float g_vpp_v;
static float g_rms_v;
static float g_ac_rms_v;
static float g_population_stddev_v;
static bool g_is_clipped;

/* 可修改运行参数字典：
 * g_output_frequency_hz：DAC 输出频率；g_sample_rate_hz：请求采样率；
 * g_effective_sample_rate_hz：硬件实际采样率，只读；g_amplitude_mv：输出 Vpk-to-pk 配置；
 * g_offset_mv：DAC 直流偏置；g_trigger_level/hysteresis：触发码和迟滞；
 * g_page：0~6 页面；g_waveform：四种波形；g_filter：RAW/MEDIAN/HAMPEL；
 * g_window：FFT 窗；g_edit_target/g_input/g_input_length：数字预输入状态；
 * valid/status 变量表示对应结果是否可用；key_queue/head/tail 是按键环形队列；
 * g_display_elapsed_ms/g_display_due 控制 250 ms 局部刷新。
 * 默认值改 APP_DEFAULT_*；量程限制改 App_CommitInput；刷新周期改 APP_DISPLAY_PERIOD_MS。 */
static uint32_t g_output_frequency_hz = APP_DEFAULT_FREQUENCY;
static uint32_t g_sample_rate_hz = SIGNAL_SAMPLE_RATE_HZ;
static uint32_t g_effective_sample_rate_hz = SIGNAL_SAMPLE_RATE_HZ;
static uint16_t g_amplitude_mv = APP_DEFAULT_AMPLITUDE_MV;
static uint16_t g_offset_mv = APP_DEFAULT_OFFSET_MV;
static uint16_t g_trigger_level = 2048U;
static uint16_t g_trigger_hysteresis = 32U;
static uint8_t g_page;
static app_waveform_t g_waveform;
static app_filter_t g_filter;
static signal_window_type_t g_window = SIGNAL_WINDOW_HANN;
static app_edit_target_t g_edit_target;
static char g_input[APP_INPUT_CAPACITY];
static uint8_t g_input_length;
/* D 键只负责进入等待状态；主循环持续检查每一帧，直到未知时刻的单次波形到来。 */
/* COMP0 边沿中断置位；主循环在安全的 DMA 块边界读取并清零。 */
static bool g_measurement_valid;
static bool g_spectrum_valid;
static volatile signal_result_t g_module_status;
static volatile signal_algorithm_status_t g_algorithm_status;
static volatile char g_key_queue[APP_KEY_QUEUE_SIZE];
static volatile uint8_t g_key_head;
static volatile uint8_t g_key_tail;
static volatile uint16_t g_display_elapsed_ms;
static volatile bool g_display_due = true;

/* 所有页面文字统一经过集成的 ST7789 8×16 字库，既保持旧页面调用可读，
 * 又避免混用底层不同字号接口造成布局不一致。 */
/* 函数索引：App_DrawString8x16/App_DrawUint8x16 是统一 8x16 字体包装；
 * App_ParseUint 和 App_Begin/Append/Commit/CancelInput 是数字预输入状态机；
 * App_ApplyWaveform 执行波表->DDS->DAC DMA；App_AcquireFrame 等待双 ADC DMA；
 * App_BasicMeasurements、App_TimeFrequency、App_Spectrum、App_RobustMeasurement、
 * App_SineFitAndLockIn、App_TriggerCapture、App_ReplayCapture、App_Calibrate 分别对应
 * 题目八个步骤；App_Draw* 只刷新当前页；App_QueueKey/ProcessQueuedKeys/SysTick 是
 * 22_X 风格键盘扫描；App_ProcessKey 做键值到功能的映射；main 负责总调度。
 * g_raw_ch1/ch2 是 ADC 原始缓冲，g_voltage/g_calibrated 是电压数组，g_* 结构体保存
 * 各模块结果；改变默认参数改 APP_DEFAULT_*，改变算法参数改对应 config 初始化。 */
/* 自写字体包装：强制所有文字使用 8x16；底层 DrawString 来自 ST7789 README。 */
static signal_result_t App_DrawString8x16(tft_st7789_t *tft,
    int32_t x, int32_t y, const char *text, uint8_t unused_scale,
    uint16_t foreground, uint16_t background)
{
    (void)unused_scale;
    return (signal_result_t)TFT_ST7789_DrawString(tft, x, y, text,
        TFT_ST7789_FONT_8X16, foreground, background, false, false);
}

/* 自写字体包装：强制整数使用 8x16；digits/scale 参数仅为兼容旧调用。 */
static signal_result_t App_DrawUint8x16(tft_st7789_t *tft,
    int32_t x, int32_t y, uint32_t value, uint8_t digits,
    uint8_t unused_scale, uint16_t foreground, uint16_t background)
{
    (void)digits;
    (void)unused_scale;
    return (signal_result_t)TFT_ST7789_DrawInt32(tft, x, y,
        (int32_t)value, TFT_ST7789_FONT_8X16, foreground, background,
        false);
}

#define SignalTFTST7789Text_DrawString App_DrawString8x16
#define SignalTFTST7789Text_DrawUint   App_DrawUint8x16

/* 自写显示换算：把浮点电压绝对值转成毫单位整数，便于屏幕显示。 */
static uint32_t App_AbsMilli(float value)
{
    return (value < 0.0f) ? (uint32_t)(-value * 1000.0f + 0.5f) :
        (uint32_t)(value * 1000.0f + 0.5f);
}

/* 自写策略：按输出频率选择 ADC 采样率；频率阈值可在本函数修改。 */
static uint32_t App_SelectSampleRate(uint32_t frequency_hz)
{
    /* 捕获页的单次脉冲只有 50~200 us，使用 1 MSPS 才能得到足够的时域点。 */
    if (g_page == 5U) return 1000000U;
    /* 40 kSPS 能为 200 Hz 保留多个周期；100 kSPS 满足 20 kHz 的 Nyquist
     * 采样要求，并与 ADC/DAC 默认时序保持一致。 */
    return (frequency_hz < 4000U) ? 40000U : 100000U;
}

/* 自写输入逻辑：把键盘缓存 text 转成无符号整数，检查非法字符和溢出。 */
static bool App_ParseUint(const char *text, uint8_t length, uint32_t *value)
{
    uint8_t i;
    uint32_t parsed = 0U;
    if ((text == NULL) || (value == NULL) || (length == 0U)) return false;
    for (i = 0U; i < length; ++i) {
        uint32_t digit;
        if ((text[i] < '0') || (text[i] > '9')) return false;
        digit = (uint32_t)(text[i] - '0');
        if (parsed > (UINT32_MAX - digit) / 10U) return false;
        parsed = parsed * 10U + digit;
    }
    *value = parsed;
    return true;
}

/* 自写输入状态机：选择要编辑的参数 target 并清空输入缓存。 */
static void App_BeginInput(app_edit_target_t target)
{
    g_edit_target = target;
    g_input_length = 0U;
    g_input[0] = '\0';
}

/* 自写输入状态机：把数字 symbol 追加到 g_input，超过容量则忽略。 */
static void App_AppendInput(char symbol)
{
    if ((symbol < '0') || (symbol > '9') ||
        (g_edit_target == APP_EDIT_NONE) ||
        (g_input_length + 1U >= APP_INPUT_CAPACITY)) return;
    g_input[g_input_length++] = symbol;
    g_input[g_input_length] = '\0';
}

/* 前置声明：参数确认后需要重新生成波表并重启 DAC DMA。 */
static signal_result_t App_ApplyWaveform(void);

/* 自写输入状态机：解析并限幅输入值，然后应用频率/幅度/偏置参数。 */
static void App_CommitInput(void)
{
    uint32_t value;
    bool parameter_changed = false;
    if ((g_edit_target == APP_EDIT_NONE) ||
        !App_ParseUint(g_input, g_input_length, &value)) return;
    if (g_edit_target == APP_EDIT_FREQUENCY) {
        if ((value >= 200U) && (value <= 20000U) &&
            (g_output_frequency_hz != value)) {
            g_output_frequency_hz = value;
            parameter_changed = true;
        }
    } else if (g_edit_target == APP_EDIT_AMPLITUDE_MV) {
        if ((value >= 10U) && (value <= 1500U) &&
            (g_amplitude_mv != (uint16_t)value)) {
            g_amplitude_mv = (uint16_t)value;
            parameter_changed = true;
        }
    } else if (g_edit_target == APP_EDIT_OFFSET_MV) {
        if ((value <= 3000U) && (g_offset_mv != (uint16_t)value)) {
            g_offset_mv = (uint16_t)value;
            parameter_changed = true;
        }
    }
    g_edit_target = APP_EDIT_NONE;
    g_input_length = 0U;
    g_input[0] = '\0';

    /* 关键修复：变量改变后重新生成波表/DDS 缓冲，并重新启动 DAC DMA。
     * 只改 g_output_frequency_hz 等变量不会改变正在播放的 g_dac_output。 */
    if (parameter_changed) {
        (void)App_ApplyWaveform();
    }
}

/* 自写输入状态机：D 键取消本次输入，不改变原参数。 */
static void App_CancelInput(void)
{
    g_edit_target = APP_EDIT_NONE;
    g_input_length = 0U;
    g_input[0] = '\0';
}

/* 【Recipe：adc_to_voltage.md】ADC DMA 的原始码不是电压。这里先把每个
 * code 乘以“参考电压×前端比例/满量程码”，再加固定校准偏移；该函数不访问
 * 寄存器，因此同样用于主循环测量和 0.5 V/2.5 V 两点校准。 */
static void App_RecipeADCToVoltage(const uint16_t *raw, float *voltage_v,
    uint32_t count, float vref_v, uint32_t adc_max_code,
    float input_scale, float offset_v)
{
    uint32_t index;
    float volts_per_code = (vref_v * input_scale) / (float)adc_max_code;

    for (index = 0U; index < count; ++index) {
        voltage_v[index] = (float)raw[index] * volts_per_code + offset_v;
    }
}

/* 【Recipe：clipping_detect.md】在滤波前逐点检查量程边界。返回的只是触边
 * 样本总数；显示层只需“是否有削顶”，故不再保留旧 config/result 封装。 */
static uint32_t App_RecipeCountClipped(const float *samples, uint32_t count,
    float low_limit_v, float high_limit_v)
{
    uint32_t index;
    uint32_t clipped = 0U;

    for (index = 0U; index < count; ++index) {
        if ((samples[index] <= low_limit_v) ||
            (samples[index] >= high_limit_v)) {
            ++clipped;
        }
    }
    return clipped;
}

/* 【Recipe：peak_detect.md】在指定闭区间找首个最大值。频谱调用从 bin 1 开始，
 * 有意跳过直流 bin 0；严格使用“>”使相同幅值时仍返回最早 bin，保持旧行为。 */
static uint32_t App_RecipePeakIndex(const float *samples, uint32_t first,
    uint32_t last, float *peak_value)
{
    uint32_t index;
    uint32_t peak = first;

    for (index = first + 1U; index <= last; ++index) {
        if (samples[index] > samples[peak]) {
            peak = index;
        }
    }
    if (peak_value != NULL) {
        *peak_value = samples[peak];
    }
    return peak;
}

/* 标准差没有独立的 recipes 页面。为删除冻结兼容封装，同时不改变原工程显示的
 * “总体标准差”定义，这里将旧模块采用的 Welford 单遍计算直接写在应用层。
 * 它避免“大直流偏置 + 小交流纹波”用 E[x²]-E[x]² 时的消去误差。 */
static float App_PopulationStdDev(const float *samples, uint32_t count)
{
    uint32_t index;
    float mean = samples[0];
    float m2 = 0.0f;

    for (index = 1U; index < count; ++index) {
        float delta = samples[index] - mean;
        mean += delta / (float)(index + 1U);
        m2 += delta * (samples[index] - mean);
    }
    return sqrtf((m2 > 0.0f) ? (m2 / (float)count) : 0.0f);
}

/* 【Recipe：cmsis_fft_spectrum.md】Recipe 的核心是交错 Q15 缓冲、CMSIS
 * arm_cfft_q15 和 arm_cmplx_mag_q15。输入先按本帧绝对峰值归一化，防止 Q15
 * 溢出；变换后再乘回“峰值×N/32767”，所以后级窗增益、谐波、THD 和屏幕显示
 * 继续使用原来的伏特量纲，而不会把 Q15 数值误当作电压。 */
static bool App_RecipeCMSISSpectrumQ15(const float *input, uint32_t count,
    float *magnitude)
{
    const arm_cfft_instance_q15 *instance;
    uint32_t index;
    uint32_t bin_count;
    float maximum_absolute = 0.0f;
    float restore_scale;

    switch (count) {
        case 16U: instance = &arm_cfft_sR_q15_len16; break;
        case 32U: instance = &arm_cfft_sR_q15_len32; break;
        case 64U: instance = &arm_cfft_sR_q15_len64; break;
        case 128U: instance = &arm_cfft_sR_q15_len128; break;
        case 256U: instance = &arm_cfft_sR_q15_len256; break;
        case 512U: instance = &arm_cfft_sR_q15_len512; break;
        case 1024U: instance = &arm_cfft_sR_q15_len1024; break;
        case 2048U: instance = &arm_cfft_sR_q15_len2048; break;
        case 4096U: instance = &arm_cfft_sR_q15_len4096; break;
        default: return false;
    }
    for (index = 0U; index < count; ++index) {
        float absolute = fabsf(input[index]);
        if (!isfinite(input[index])) {
            return false;
        }
        if (absolute > maximum_absolute) {
            maximum_absolute = absolute;
        }
    }
    bin_count = (count / 2U) + 1U;
    if (maximum_absolute == 0.0f) {
        for (index = 0U; index < bin_count; ++index) {
            magnitude[index] = 0.0f;
        }
        return true;
    }
    for (index = 0U; index < count; ++index) {
        g_fft_q15[2U * index] =
            (q15_t)((input[index] / maximum_absolute) * 32767.0f);
        g_fft_q15[(2U * index) + 1U] = 0;
    }
    arm_cfft_q15(instance, g_fft_q15, 0U, 1U);
    arm_cmplx_mag_q15(g_fft_q15, g_fft_magnitude_q15, count);
    restore_scale = maximum_absolute * (float)count / 32767.0f;
    for (index = 0U; index < bin_count; ++index) {
        magnitude[index] = (float)g_fft_magnitude_q15[index] * restore_scale;
    }
    return true;
}

/* 模块调用组合：生成正弦/方波/三角/锯齿波表，经 DDS 和 DAC DMA 输出。 */
static signal_result_t App_ApplyWaveform(void)
{
    float frequency_hz = (float)g_output_frequency_hz;
    float vpp_v = (float)g_amplitude_mv / 1000.0f;
    float offset_v = (float)g_offset_mv / 1000.0f;
    /* 回放页可能把 DAC 更新率临时提高到 1 MSPS；普通发生器恢复 100 kSPS。 */
    SignalDACDMA_MSPM0_Stop();
    if (SignalDACDMA_MSPM0_SetUpdateRate(SIGNAL_DAC_UPDATE_RATE_HZ) !=
        SIGNAL_RESULT_OK) return SIGNAL_RESULT_OUT_OF_RANGE;
    /* 【Wave Output 整合模块】接口内部完成波表、DDS 整周期缓冲和 DAC DMA。
     * main 只负责选择波形并传入参数；方波占空比固定 50%，锯齿波对称度固定 100%。 */
    if (g_waveform == APP_WAVE_SINE) {
        return SignalWaveOutput_SineWithOffset(frequency_hz, vpp_v, offset_v);
    } else if (g_waveform == APP_WAVE_SQUARE) {
        return SignalWaveOutput_SquareWithDuty(frequency_hz, vpp_v, offset_v,
            APP_DEFAULT_SQUARE_DUTY);
    } else if (g_waveform == APP_WAVE_TRIANGLE) {
        return SignalWaveOutput_TriangleWithOffset(frequency_hz, vpp_v, offset_v);
    }
    return SignalWaveOutput_SawtoothWithSymmetry(frequency_hz, vpp_v, offset_v,
        APP_DEFAULT_SAW_SYMMETRY);
}

/* 模块调用组合：启动双 ADC DMA 并等待一帧完成，结果放入 g_raw_ch1/ch2。 */
static signal_result_t App_AcquireFrame(void)
{
    signal_dual_adc_config_t config = {
        g_sample_rate_hz, CPUCLK_FREQ, 65536U
    };
    /* 【双 ADC 模块】动态修改共用触发定时器的采样率。 */
    g_module_status = SignalDualADC_SetSampleRate(g_sample_rate_hz);
    if (g_module_status != SIGNAL_RESULT_OK) return g_module_status;
    /* 【双 ADC 模块】两路使用同一个硬件触发，DMA 分别写入两个缓冲区。 */
    g_module_status = SignalDualADC_Start(g_raw_ch1, g_raw_ch2,
        SIGNAL_SAMPLE_COUNT);
    if (g_module_status != SIGNAL_RESULT_OK) return g_module_status;
    /* DMA_IRQHandler 在模块内部置完成标志，等待期间 CPU 用 WFI 休眠。 */
    while (!SignalDualADC_IsFinished()) { __WFI(); }
    g_effective_sample_rate_hz = SignalDualADC_GetConfiguredRate();
    (void)config;
    return SIGNAL_RESULT_OK;
}

/* 按 `00_docs/recipes` 执行 Mean、MinMax/Vpp、RMS、AC RMS 与 Clipping。 */
static void App_BasicMeasurements(void)
{
    uint32_t ignored_min_index;
    uint32_t ignored_max_index;

    /* mean.md、minmax.md、vpp.md、rms.md：这些 CMSIS 调用均只读取
     * g_calibrated，不会改变后续频谱和鲁棒测量仍要使用的原始校准波形。 */
    arm_mean_f32(g_calibrated, SIGNAL_SAMPLE_COUNT, &g_mean_v);
    arm_min_f32(g_calibrated, SIGNAL_SAMPLE_COUNT, &g_min_v,
        &ignored_min_index);
    arm_max_f32(g_calibrated, SIGNAL_SAMPLE_COUNT, &g_max_v,
        &ignored_max_index);
    g_vpp_v = g_max_v - g_min_v;
    arm_rms_f32(g_calibrated, SIGNAL_SAMPLE_COUNT, &g_rms_v);

    /* ac_rms.md：使用 g_workspace 保存去 DC 的交流分量，保证
     * g_calibrated 仍保留包含 DC 的校准电压，供总 RMS 与显示复用。 */
    arm_offset_f32(g_calibrated, -g_mean_v, g_workspace,
        SIGNAL_SAMPLE_COUNT);
    arm_rms_f32(g_workspace, SIGNAL_SAMPLE_COUNT, &g_ac_rms_v);

    g_population_stddev_v = App_PopulationStdDev(g_calibrated,
        SIGNAL_SAMPLE_COUNT);
    /* clipping_detect.md：沿用原 0.02~3.28 V 安全边界，防止改变既有
     * 超量程提示灵敏度；只要存在一个触边样本就置 g_is_clipped。 */
    g_is_clipped = App_RecipeCountClipped(g_calibrated,
        SIGNAL_SAMPLE_COUNT, 0.02f, 3.28f) != 0U;
}

/* 模块调用组合：执行 Timer Capture、过零插值和多周期平均三路测频。 */
static void App_TimeFrequency(void)
{
    signal_zero_cross_config_t zero_config = {
        0.0f, 0.005f, SIGNAL_ZERO_CROSS_RISING
    };
    signal_zero_cross_result_t zero_result;
    signal_zero_cross_interpolation_result_t interpolation_result;
    float mean_ticks;
    float capture_frequency;
    signal_timer_capture_config_t capture_config = {
        CPUCLK_FREQ, SIGNAL_CAPTURE_INST_LOAD_VALUE + 1U
    };

    /* remove_dc.md：把中心移动到 0 V，后续门限才能固定为 0。 */
    arm_mean_f32(g_calibrated, SIGNAL_SAMPLE_COUNT, &g_mean_v);
    arm_offset_f32(g_calibrated, -g_mean_v, g_centered,
        SIGNAL_SAMPLE_COUNT);
    /* 【Zero Cross】按 zero_config 的上升沿和 5 mV 迟滞寻找过零事件。 */
    g_algorithm_status = SignalZeroCross_Process(g_centered,
        SIGNAL_SAMPLE_COUNT, &zero_config, g_zero_events,
        SIGNAL_ZERO_EVENT_CAPACITY, &zero_result);
    if ((g_algorithm_status == SIGNAL_ALGORITHM_OK) &&
        (zero_result.rising_count >= 2U)) {
        /* 【Zero Cross Interpolation】在相邻采样间线性插值，得到小数采样点位置。 */
        g_algorithm_status = SignalZeroCrossInterpolation_Process(g_centered,
            SIGNAL_SAMPLE_COUNT, 0.0f, g_zero_events,
            zero_result.event_count, g_crossing_positions,
            SIGNAL_ZERO_EVENT_CAPACITY, &interpolation_result);
        if (g_algorithm_status == SIGNAL_ALGORITHM_OK) {
            /* 该页只显示硬件捕获和频谱频率；旧的多周期平均结果从未被
             * 保存或显示，删除其兼容调用，仍保留过零检测与插值功能。 */
        }
    }
    /* 【Timer Capture MSPM0】读取比较器输出脉冲的硬件捕获频率。 */
    if (SignalTimerCapture_MSPM0_GetResult(&g_capture_frequency) !=
        SIGNAL_RESULT_OK) {
        g_capture_frequency.valid = false;
    }
    if (SignalTimerCapture_MeanPeriod(NULL, 0U, &capture_config,
            &mean_ticks, &capture_frequency) == SIGNAL_RESULT_OK) {
        (void)mean_ticks;
    }
}

/* 模块调用组合：去直流、加窗、FFT、峰值插值、谐波、THD、SNR、SFDR。 */
static void App_Spectrum(void)
{
    signal_window_result_t window_result;
    signal_harmonic_config_t harmonic_config;
    signal_snr_config_t snr_config;
    signal_sfdr_config_t sfdr_config;

    /* 任一步失败都保持 false，屏幕不能把旧 FFT 数据误认为本帧结果。 */
    g_spectrum_valid = false;
    /* remove_dc.md。 */
    arm_mean_f32(g_calibrated, SIGNAL_SAMPLE_COUNT, &g_mean_v);
    arm_offset_f32(g_calibrated, -g_mean_v, g_centered,
        SIGNAL_SAMPLE_COUNT);
    /* 【Window】g_window 由按键选择 Rect/Hann/Hamming/Blackman。 */
    g_algorithm_status = SignalWindow_Apply(g_centered, g_voltage,
        SIGNAL_SAMPLE_COUNT, g_window, &window_result);
    if (g_algorithm_status != SIGNAL_ALGORITHM_OK) return;
    /* cmsis_fft_spectrum.md：直接运行 Q15 CFFT 和复数幅值。 */
    if (!App_RecipeCMSISSpectrumQ15(g_voltage, SIGNAL_SAMPLE_COUNT,
            g_magnitude)) return;
    /* 【Window Gain Correction】用 coherent_gain 补偿加窗造成的幅度衰减。 */
    g_algorithm_status = SignalWindowGainCorrection_Apply(g_magnitude,
        g_magnitude, (SIGNAL_SAMPLE_COUNT / 2U) + 1U, SIGNAL_SAMPLE_COUNT,
        window_result.coherent_gain);
    if (g_algorithm_status != SIGNAL_ALGORITHM_OK) return;
    /* peak_detect.md：从 bin1 开始搜索，故意跳过 DC bin0。 */
    g_fft_peak.bin = (uint16_t)App_RecipePeakIndex(g_magnitude, 1U,
        SIGNAL_SAMPLE_COUNT / 2U, &g_fft_peak.peak_value);
    g_fft_peak.frequency_hz = (float)g_fft_peak.bin *
        (float)g_effective_sample_rate_hz / (float)SIGNAL_SAMPLE_COUNT;
    /* 【FFT Parabolic Interpolation】使用峰值左右三个 bin 精修基波频率。 */
    (void)SignalFFTParabolicInterpolation_Process(g_magnitude,
        (SIGNAL_SAMPLE_COUNT / 2U) + 1U, g_fft_peak.bin,
        (float)g_effective_sample_rate_hz, SIGNAL_SAMPLE_COUNT,
        &g_fft_interpolated);
    harmonic_config.fundamental_frequency_hz = g_fft_interpolated.frequency_hz;
    harmonic_config.first_order = 1U;
    harmonic_config.last_order =
        (g_fft_interpolated.frequency_hz * 3.0f <
         (float)g_effective_sample_rate_hz * 0.5f) ? 3U : 2U;
    harmonic_config.radius_bins = 1U;
    /* 【Harmonic】按基波频率搜索 H1~H3；结果保存到全局供屏幕使用。 */
    if (SignalHarmonic_Process(g_magnitude, (SIGNAL_SAMPLE_COUNT / 2U) + 1U,
            (float)g_effective_sample_rate_hz, SIGNAL_SAMPLE_COUNT,
            &harmonic_config, &g_harmonics) == SIGNAL_ALGORITHM_OK) {
        (void)SignalTHD_Process(&g_harmonics, &g_thd);
    }
    snr_config.signal_start_bin = g_fft_peak.bin > 1U ?
        g_fft_peak.bin - 1U : 1U;
    snr_config.signal_end_bin = g_fft_peak.bin + 1U;
    snr_config.analysis_start_bin = 1U;
    snr_config.analysis_end_bin = SIGNAL_SAMPLE_COUNT / 2U;
    snr_config.excluded_ranges = NULL;
    snr_config.excluded_range_count = 0U;
    (void)SignalSNR_Process(g_magnitude, (SIGNAL_SAMPLE_COUNT / 2U) + 1U,
        &snr_config, &g_snr);
    sfdr_config.main_start_bin = snr_config.signal_start_bin;
    sfdr_config.main_end_bin = snr_config.signal_end_bin;
    sfdr_config.analysis_start_bin = 1U;
    sfdr_config.analysis_end_bin = SIGNAL_SAMPLE_COUNT / 2U;
    (void)SignalSFDR_Process(g_magnitude, (SIGNAL_SAMPLE_COUNT / 2U) + 1U,
        &sfdr_config, &g_sfdr);
    g_spectrum_valid = true;
}

/* 模块调用组合：根据 RAW/MEDIAN/HAMPEL 选择抗毛刺处理并计算鲁棒 VPP/RMS。 */
static void App_RobustMeasurement(void)
{
    const float *input = g_calibrated;
    signal_mad_result_t mad_result;
    signal_robust_peak_to_peak_config_t vpp_config = {0.05f, 0.95f};
    signal_robust_rms_config_t rms_config = {0.05f, 0.95f, 1U};
    signal_hampel_config_t hampel_config = {5U, 3.0f, 0.001f};
    if (g_filter == APP_FILTER_MEDIAN) {
        (void)SignalMedianFilter_Process(g_calibrated, g_voltage,
            SIGNAL_SAMPLE_COUNT, 5U, g_workspace, SIGNAL_SAMPLE_COUNT);
        input = g_voltage;
    } else if (g_filter == APP_FILTER_HAMPEL) {
        (void)SignalHampel_Process(g_calibrated, g_voltage,
            SIGNAL_SAMPLE_COUNT, &hampel_config, g_workspace,
            SIGNAL_SAMPLE_COUNT, &(signal_hampel_result_t){0});
        input = g_voltage;
    }
    (void)SignalMAD_Process(input, SIGNAL_SAMPLE_COUNT, g_workspace,
        SIGNAL_SAMPLE_COUNT, &mad_result);
    g_mad = mad_result;
    (void)SignalRobustPeakToPeak_Process(input, SIGNAL_SAMPLE_COUNT,
        &vpp_config, g_workspace, SIGNAL_SAMPLE_COUNT, &g_robust_vpp);
    (void)SignalRobustRMS_Process(input, SIGNAL_SAMPLE_COUNT, &rms_config,
        g_workspace, SIGNAL_SAMPLE_COUNT, &g_robust_rms);
}

/* 模块调用组合：调用 Sine Fit 3P/4P 和已知 DDS 参考的 Lock-In。 */
static void App_SineFitAndLockIn(void)
{
    signal_sine_fit_3param_config_t fit3_config = {
        g_fft_interpolated.frequency_hz, (float)g_effective_sample_rate_hz
    };
    signal_sine_fit_4param_config_t fit4_config = {
        g_fft_interpolated.frequency_hz, 10.0f,
        (float)g_effective_sample_rate_hz, 12U
    };
    signal_lock_in_config_t lock_config = {
        (float)g_output_frequency_hz, (float)g_effective_sample_rate_hz,
        0.0f, 1U
    };
    if (!g_spectrum_valid) return;
    (void)SignalSineFit3Param_Process(g_calibrated, SIGNAL_SAMPLE_COUNT,
        &fit3_config, &g_fit3);
    (void)SignalSineFit4Param_Process(g_calibrated, SIGNAL_SAMPLE_COUNT,
        &fit4_config, &g_fit4);
    (void)SignalLockIn_Process(g_calibrated, SIGNAL_SAMPLE_COUNT,
        &lock_config, &g_lock_in);
}

/* 组合模块回调：硬件宏必须留在main，由当前工程的SysConfig决定。 */
static void App_ClearCaptureTrigger(void *context)
{
    (void)context;
    DL_COMP_clearInterruptStatus(SIGNAL_COMP_INST,
        DL_COMP_INTERRUPT_OUTPUT_EDGE | DL_COMP_INTERRUPT_OUTPUT_EDGE_INV);
    NVIC_ClearPendingIRQ(SIGNAL_COMP_INST_INT_IRQN);
}

static bool App_ConsumeCaptureTrigger(void *context)
{
    uint32_t edges;
    (void)context;
    edges = DL_COMP_getRawInterruptStatus(SIGNAL_COMP_INST,
        DL_COMP_INTERRUPT_OUTPUT_EDGE | DL_COMP_INTERRUPT_OUTPUT_EDGE_INV);
    if (edges == 0U) return false;
    DL_COMP_clearInterruptStatus(SIGNAL_COMP_INST, edges);
    return true;
}

static void App_ArmCapture(void)
{
    (void)SignalSingleCaptureReplay_Arm(&g_single_capture);
}

static void App_ServiceCapture(void)
{
    if (SignalSingleCaptureReplay_Service(&g_single_capture) ==
        SIGNAL_RESULT_OK) {
        g_display_due = true;
    }
}

/* MSPM0G3507 的 COMP0 使用 GROUP1 共享中断向量，不是 COMP0_IRQHandler。
 * 内部比较器边沿只做触发锁存，不在 ISR 中访问 DMA 或 SPI。 */
void GROUP1_IRQHandler(void)
{
    DL_COMP_IIDX index = DL_COMP_getPendingInterrupt(SIGNAL_COMP_INST);
    if ((index == DL_COMP_IIDX_OUTPUT_EDGE) ||
        (index == DL_COMP_IIDX_OUTPUT_EDGE_INV)) {
        SignalSingleCaptureReplay_NotifyTrigger(&g_single_capture);
        DL_COMP_clearInterruptStatus(SIGNAL_COMP_INST,
            DL_COMP_INTERRUPT_OUTPUT_EDGE | DL_COMP_INTERRUPT_OUTPUT_EDGE_INV);
    }
}

/* 模块调用组合：把指定槽位重采样为任意波表，以捕获采样率循环回放。 */
static void App_ReplayCapture(void)
{
    (void)SignalSingleCaptureReplay_ReplaySelected(&g_single_capture);
}

/* 模块调用组合：执行 ADC 增益/偏置校准和双通道固定延迟校准。 */
static void App_Calibrate(void)
{
    float low_mean_v;
    float high_mean_v;
    uint16_t low_code;
    uint16_t high_code;
    if (SignalDACWaveTable_NormalizedToRaw(0.0f, SIGNAL_DAC_BITS,
            0.5f / SIGNAL_ADC_VREF_V, 0.0f, &low_code) != SIGNAL_RESULT_OK) return;
    if (SignalDACWaveTable_NormalizedToRaw(0.0f, SIGNAL_DAC_BITS,
            2.5f / SIGNAL_ADC_VREF_V, 0.0f, &high_code) != SIGNAL_RESULT_OK) return;
    SignalDACDMA_MSPM0_Stop();
    g_wave_table[0] = low_code;
    if (SignalDACDMA_MSPM0_Start(g_wave_table, 1U, true) != SIGNAL_RESULT_OK) return;
    if (App_AcquireFrame() != SIGNAL_RESULT_OK) return;
    /* adc_to_voltage.md 后立刻按 mean.md 取 0.5 V 平台平均值。 */
    App_RecipeADCToVoltage(g_raw_ch1, g_voltage, SIGNAL_SAMPLE_COUNT,
        SIGNAL_ADC_VREF_V, APP_ADC_MAX_CODE, 1.0f, 0.0f);
    arm_mean_f32(g_voltage, SIGNAL_SAMPLE_COUNT, &low_mean_v);
    SignalDACDMA_MSPM0_Stop();
    g_wave_table[0] = high_code;
    if (SignalDACDMA_MSPM0_Start(g_wave_table, 1U, true) != SIGNAL_RESULT_OK) return;
    if (App_AcquireFrame() != SIGNAL_RESULT_OK) return;
    /* 同一换算与均值链测得 2.5 V 平台，保持原先两点校准的物理含义。 */
    App_RecipeADCToVoltage(g_raw_ch1, g_voltage, SIGNAL_SAMPLE_COUNT,
        SIGNAL_ADC_VREF_V, APP_ADC_MAX_CODE, 1.0f, 0.0f);
    arm_mean_f32(g_voltage, SIGNAL_SAMPLE_COUNT, &high_mean_v);
    (void)SignalADCGainOffsetCalibration_Compute(low_mean_v,
        0.5f, high_mean_v, 2.5f, &g_adc_calibration);
    (void)SignalChannelDelayCalibration_Compute(
        g_fit3.phase_deg, 0.0f, (float)g_output_frequency_hz,
        &g_delay_calibration);
    (void)App_ApplyWaveform();
}

/* 自写局部显示：固定画 label，只清除并重画右侧数值区域。 */
static void App_DrawValue(int32_t x, int32_t y, const char *label,
    uint32_t value, uint16_t color)
{
    /* 标签固定不变；只擦除并重绘右侧数值字段。 */
    (void)SignalTFTST7789Text_DrawString(&g_tft, x, y, label, 1U,
        TFT_ST7789_WHITE, TFT_ST7789_BLACK);
    (void)TFT_ST7789_FillRect(&g_tft, x + 54, y, 104, 16,
        TFT_ST7789_BLACK);
    (void)SignalTFTST7789Text_DrawUint(&g_tft, x + 54, y, value, 5U, 1U,
        color, TFT_ST7789_BLACK);
}

/* 自写局部显示：清除状态字段后绘制 text，避免整屏刷新。 */
static void App_DrawStatus(int32_t x, int32_t y, const char *text,
    uint16_t color)
{
    (void)TFT_ST7789_FillRect(&g_tft, x, y, 304, 16, TFT_ST7789_BLACK);
    (void)SignalTFTST7789Text_DrawString(&g_tft, x, y, text, 1U,
        color, TFT_ST7789_BLACK);
}

/* 自写提示显示：把当前页面、数字输入内容和常用按键功能显示出来。
 * 这些提示只清除各自的 304x16 字段，因此不会刷新蓝色边框和测量区域。 */
static void App_DrawControlInfo(void)
{
    const char *mode = (g_page == 0U) ? "MODE:GEN" :
        (g_page == 1U) ? "MODE:MEASURE" :
        (g_page == 2U) ? "MODE:FREQ" :
        (g_page == 3U) ? "MODE:SPECTRUM" :
        (g_page == 4U) ? "MODE:ROBUST" :
        (g_page == 5U) ? "MODE:CAPTURE" : "MODE:CAL";
    const char *keys = (g_page == 0U) ? "A:NEXT B:F C:A *:O D:C" :
        (g_page == 3U) ? "A:NEXT 8/9/0/#:WINDOW" :
        (g_page == 4U) ? "A:NEXT 5/6/7:FILTER" :
        (g_page == 5U) ? "A:NEXT D:SAVE #:PLAY 1/2/3:SEL" :
        (g_page == 6U) ? "A:NEXT D:CALIBRATE" :
        "A:NEXT 1-4:WAVE";
    char input_text[APP_INPUT_CAPACITY + 5U];
    uint8_t i;

    /* 第一行右侧告诉用户当前处于哪个功能页面；标题在 x=8，模式放在 x=120，
     * 这样不会占用第二行，也不会压住捕获页从 y=32 开始的曲线区域。 */
    (void)TFT_ST7789_FillRect(&g_tft, 120, 8, 192, 16, TFT_ST7789_BLACK);
    (void)SignalTFTST7789Text_DrawString(&g_tft, 120, 8, mode, 1U,
        TFT_ST7789_CYAN, TFT_ST7789_BLACK);

    /* 第二行显示正在输入的数字；没有输入时显示提示文字。 */
    for (i = 0U; i < sizeof(input_text); ++i) input_text[i] = '\0';
    input_text[0] = 'I';
    input_text[1] = 'N';
    input_text[2] = ':';
    if (g_edit_target == APP_EDIT_FREQUENCY) {
        input_text[3] = 'F';
        input_text[4] = '=';
    } else if (g_edit_target == APP_EDIT_AMPLITUDE_MV) {
        input_text[3] = 'A';
        input_text[4] = '=';
    } else if (g_edit_target == APP_EDIT_OFFSET_MV) {
        input_text[3] = 'O';
        input_text[4] = '=';
    } else {
        input_text[3] = '-';
        input_text[4] = '-';
    }
    if (g_edit_target != APP_EDIT_NONE) {
        for (i = 0U; i < g_input_length; ++i) {
            input_text[5U + i] = g_input[i];
        }
    } else {
        input_text[5] = ' '; input_text[6] = ' '; input_text[7] = ' '; 
        input_text[8] = 'N'; input_text[9] = 'O'; input_text[10] = 'N';
        input_text[11] = 'E';
    }
    App_DrawStatus(8, 208, input_text, TFT_ST7789_YELLOW);

    /* 在状态栏右侧覆盖显示按键提示；CAPTURE 页面图形区域结束于 202。 */
    App_DrawStatus(8, 224, keys, TFT_ST7789_WHITE);
}

/* 自写页面：清屏并绘制当前页边框、标题和固定布局；A 翻页时调用。 */
static void App_DrawStaticPage(void)
{
    const char *title = (g_page == 0U) ? "GEN" :
        (g_page == 1U) ? "MEASURE" : (g_page == 2U) ? "FREQ" :
        (g_page == 3U) ? "SPECTRUM" : (g_page == 4U) ? "ROBUST" :
        (g_page == 5U) ? "CAPTURE" : "CAL";
    (void)TFT_ST7789_FillScreen(&g_tft, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawRect(&g_tft, 1, 1, 318, 238, TFT_ST7789_BLUE);
    (void)SignalTFTST7789Text_DrawString(&g_tft, 8, 8, title, 2U,
        TFT_ST7789_CYAN, TFT_ST7789_BLACK);
}

/* 自写页面：依据 g_page 刷新当前测量值；普通页仅清数字字段，捕获页只清曲线框。 */
static void App_DrawDynamicPage(void)
{
    uint16_t color = g_is_clipped ? TFT_ST7789_RED : TFT_ST7789_GREEN;
    if (g_page == 0U) {
        App_DrawValue(8, 36, "FREQ", g_output_frequency_hz, TFT_ST7789_YELLOW);
        App_DrawValue(8, 54, "AMP", g_amplitude_mv, TFT_ST7789_YELLOW);
        App_DrawValue(8, 72, "OFF", g_offset_mv, TFT_ST7789_YELLOW);
        (void)SignalTFTST7789Text_DrawString(&g_tft, 8, 96, "SINE 1 SQ 2 TRI 3 SAW 4", 1U,
            TFT_ST7789_WHITE, TFT_ST7789_BLACK);
    } else if (g_page == 1U) {
        App_DrawValue(8, 36, "DC", App_AbsMilli(g_mean_v), color);
        App_DrawValue(8, 54, "VPP", App_AbsMilli(g_vpp_v), color);
        App_DrawValue(8, 72, "RMS", App_AbsMilli(g_rms_v), color);
        App_DrawValue(8, 90, "AC", App_AbsMilli(g_ac_rms_v), color);
        App_DrawValue(8, 108, "MIN", App_AbsMilli(g_min_v), color);
        App_DrawValue(8, 126, "MAX", App_AbsMilli(g_max_v), color);
        App_DrawValue(8, 144, "STD", App_AbsMilli(g_population_stddev_v), color);
        App_DrawStatus(8, 166, g_is_clipped ? "CLIPPING" : "SAFE", color);
    } else if (g_page == 2U) {
        App_DrawValue(8, 36, "TIMER", (uint32_t)g_capture_frequency.frequency_hz, TFT_ST7789_YELLOW);
        App_DrawValue(8, 54, "ZERO", (uint32_t)g_fft_peak.frequency_hz, TFT_ST7789_YELLOW);
        App_DrawValue(8, 72, "FFT", (uint32_t)g_fft_interpolated.frequency_hz, TFT_ST7789_CYAN);
        App_DrawValue(8, 90, "FIT", (uint32_t)g_fit4.frequency_hz, TFT_ST7789_GREEN);
        App_DrawValue(8, 108, "LOCK", App_AbsMilli(g_lock_in.amplitude_peak_v), TFT_ST7789_MAGENTA);
    } else if (g_page == 3U) {
        App_DrawValue(8, 36, "FREQ", (uint32_t)g_fft_interpolated.frequency_hz, TFT_ST7789_CYAN);
        /* H2/H3 显示对应谐波带内的真实 RSS 幅值，单位 mV。 */
        App_DrawValue(8, 54, "H2", (g_harmonics.last_order >= 2U) ?
            App_AbsMilli(g_harmonics.items[2].root_sum_square) : 0U,
            TFT_ST7789_YELLOW);
        App_DrawValue(8, 72, "H3", (g_harmonics.last_order >= 3U) ?
            App_AbsMilli(g_harmonics.items[3].root_sum_square) : 0U,
            TFT_ST7789_YELLOW);
        App_DrawValue(8, 90, "THD", (uint32_t)g_thd.thd_percent, TFT_ST7789_RED);
        App_DrawValue(8, 108, "SNR", (uint32_t)g_snr.snr_db, TFT_ST7789_GREEN);
        App_DrawValue(8, 126, "SFDR", (uint32_t)g_sfdr.sfdr_db, TFT_ST7789_GREEN);
        App_DrawStatus(8, 150,
            (g_window == SIGNAL_WINDOW_RECTANGULAR) ? "RECT" :
            (g_window == SIGNAL_WINDOW_HANN) ? "HANN" :
            (g_window == SIGNAL_WINDOW_HAMMING) ? "HAMMING" : "BLACKMAN",
            TFT_ST7789_WHITE);
    } else if (g_page == 4U) {
        App_DrawValue(8, 36, "VPP", App_AbsMilli(g_vpp_v), TFT_ST7789_WHITE);
        App_DrawValue(8, 54, "RVPP", App_AbsMilli(g_robust_vpp.robust_vpp_v), TFT_ST7789_CYAN);
        App_DrawValue(8, 72, "RMS", App_AbsMilli(g_rms_v), TFT_ST7789_WHITE);
        App_DrawValue(8, 90, "RRMS", App_AbsMilli(g_robust_rms.robust_rms_v), TFT_ST7789_CYAN);
        App_DrawValue(8, 108, "MAD", App_AbsMilli(g_mad.mad_value), TFT_ST7789_YELLOW);
        App_DrawStatus(8, 132,
            (g_filter == APP_FILTER_RAW) ? "RAW" :
            (g_filter == APP_FILTER_MEDIAN) ? "MEDIAN" : "HAMPEL",
            TFT_ST7789_WHITE);
    } else if (g_page == 5U) {
        const uint16_t *capture_samples = NULL;
        signal_single_capture_info_t capture_info;
        const uint8_t selected_slot =
            SignalSingleCaptureReplay_GetSelectedSlot(&g_single_capture);
        const bool slot_ready =
            (SignalSingleCaptureReplay_GetSelected(&g_single_capture,
                &capture_samples, &capture_info) == SIGNAL_RESULT_OK);
        (void)TFT_ST7789_FillRect(&g_tft, APP_GRAPH_X, APP_GRAPH_Y,
            APP_GRAPH_W, APP_GRAPH_H, TFT_ST7789_BLACK);
        /* GATE 是 COMP0 的内部 DAC 门限对应 ADC 码；PA27 进 COMP0，
         * PA25 由 ADC0 持续采集原始波形。 */
        App_DrawValue(8, 36, "GATE", g_trigger_level, TFT_ST7789_YELLOW);
        (void)SignalTFTST7789Text_DrawString(&g_tft, 150, 36,
            "PA27 COMP", 1U, TFT_ST7789_CYAN, TFT_ST7789_BLACK);
        App_DrawStatus(8, 54,
            SignalSingleCaptureReplay_IsArmed(&g_single_capture) ?
                "WAIT SIGNAL" : (slot_ready ? "SLOT READY" : "SLOT EMPTY"),
            SignalSingleCaptureReplay_IsArmed(&g_single_capture) ?
                TFT_ST7789_YELLOW :
                (slot_ready ? TFT_ST7789_GREEN : TFT_ST7789_RED));
        App_DrawStatus(8, 72, (selected_slot == 0U) ? "S1 SELECT" :
            (selected_slot == 1U) ? "S2 SELECT" : "S3 SELECT",
            TFT_ST7789_CYAN);
        if (slot_ready) {
            (void)capture_samples;
            App_DrawValue(8, 90, "DUR", capture_info.duration_us, TFT_ST7789_WHITE);
            (void)SignalTFTST7789Text_DrawString(&g_tft, 174, 90,
                "us AUTO X/Y", 1U, TFT_ST7789_WHITE, TFT_ST7789_BLACK);
            {
                const signal_single_capture_plot_config_t plot = {
                    APP_GRAPH_X, APP_GRAPH_Y, APP_GRAPH_W, APP_GRAPH_H,
                    TFT_ST7789_YELLOW, TFT_ST7789_BLACK, true
                };
                (void)SignalSingleCaptureReplay_DrawSelectedST7789(
                    &g_single_capture, &g_tft, &plot, &capture_info);
            }
        } else {
            App_DrawStatus(8, 90, "RAW WIN:416us", TFT_ST7789_WHITE);
        }
    } else {
        App_DrawStatus(8, 36, "CAL DAC 0V5 2V5", TFT_ST7789_WHITE);
        App_DrawValue(8, 54, "GAIN", (uint32_t)(g_adc_calibration.gain * 1000.0f), TFT_ST7789_CYAN);
        App_DrawValue(8, 72, "OFF", App_AbsMilli(g_adc_calibration.offset_v), TFT_ST7789_CYAN);
        App_DrawValue(8, 90, "DELAY", App_AbsMilli(g_delay_calibration.delay_b_relative_to_a_s), TFT_ST7789_YELLOW);
        App_DrawStatus(8, 114, "D CALIBRATE", TFT_ST7789_WHITE);
    }
    App_DrawControlInfo();
}

static void App_ProcessKey(char symbol);

/* 自写队列：把 SysTick 扫描到的稳定 symbol 放入环形缓冲；满时丢弃新键。 */
static void App_QueueKey(char symbol)
{
    uint8_t next = (uint8_t)((g_key_head + 1U) % APP_KEY_QUEUE_SIZE);
    if (next == g_key_tail) return;
    g_key_queue[g_key_head] = symbol;
    g_key_head = next;
}

/* 自写队列：主循环取出待处理字符并调用 App_ProcessKey。 */
static void App_ProcessQueuedKeys(void)
{
    while (g_key_tail != g_key_head) {
        char symbol = g_key_queue[g_key_tail];
        g_key_tail = (uint8_t)((g_key_tail + 1U) % APP_KEY_QUEUE_SIZE);
        App_ProcessKey(symbol);
    }
}

/* 自写中断：1 ms 计时、5 ms 键盘扫描、250 ms 显示节拍；不做 FFT/SPI。 */
void SysTick_Handler(void)
{
    static uint8_t scan_milliseconds;
    char symbol;
    ++scan_milliseconds;
    ++g_display_elapsed_ms;
    if (g_display_elapsed_ms >= APP_DISPLAY_PERIOD_MS) {
        g_display_elapsed_ms = 0U;
        /* 捕获结果是静态保存的数据，不必每 250 ms 擦除重画；捕获完成、
         * 选择槽位或按键时会单独置 g_display_due，曲线因此保持不闪。 */
        if (g_page != 5U) g_display_due = true;
    }
    if (scan_milliseconds < APP_KEYPAD_SCAN_MS) return;
    scan_milliseconds = 0U;
    if (SignalMatrixKeypad4x4_ReadNewSymbol(&symbol) == SIGNAL_RESULT_OK) {
        App_QueueKey(symbol);
    }
}

/* 自写状态机：数字键进入预输入，# 确认，D 取消/武装捕获/校准，A 翻页。 */
static void App_ProcessKey(char symbol)
{
    g_display_due = true;
    if (g_edit_target != APP_EDIT_NONE) {
        if ((symbol >= '0') && (symbol <= '9')) App_AppendInput(symbol);
        else if (symbol == '#') App_CommitInput();
        else if (symbol == 'D') App_CancelInput();
        return;
    }
    if ((g_page == 5U) && (symbol >= '1') && (symbol <= '3')) {
        if (SignalSingleCaptureReplay_SelectSlot(&g_single_capture,
                (uint8_t)(symbol - '1')) == SIGNAL_RESULT_OK) {
            App_ReplayCapture();
        }
    } else if ((symbol >= '1') && (symbol <= '4')) {
        g_waveform = (app_waveform_t)(symbol - '1');
        (void)App_ApplyWaveform();
    } else if (symbol == '5') g_filter = APP_FILTER_RAW;
    else if (symbol == '6') g_filter = APP_FILTER_MEDIAN;
    else if (symbol == '7') g_filter = APP_FILTER_HAMPEL;
    else if (symbol == '8') g_window = SIGNAL_WINDOW_RECTANGULAR;
    else if (symbol == '9') g_window = SIGNAL_WINDOW_HANN;
    else if (symbol == '0') g_window = SIGNAL_WINDOW_HAMMING;
    else if ((g_page == 5U) && (symbol == '#')) App_ReplayCapture();
    else if (symbol == '#') g_window = SIGNAL_WINDOW_BLACKMAN;
    else if (symbol == 'A') {
        if (SignalSingleCaptureReplay_IsArmed(&g_single_capture)) {
            SignalSingleCaptureReplay_Cancel(&g_single_capture);
        }
        g_page = (uint8_t)((g_page + 1U) % APP_PAGE_COUNT);
        App_DrawStaticPage();
    } else if (symbol == 'B') App_BeginInput(APP_EDIT_FREQUENCY);
    else if (symbol == 'C') App_BeginInput(APP_EDIT_AMPLITUDE_MV);
    else if (symbol == '*') App_BeginInput(APP_EDIT_OFFSET_MV);
    else if (symbol == 'D') {
        if (g_page == 5U) {
            /* D：选择下一个槽位并进入等待；主循环随后服务连续 DMA，
             * 不依赖信号结束沿，因此适用于图示的任意波形。 */
            (void)SignalSingleCaptureReplay_SelectSlot(&g_single_capture,
                SignalSingleCaptureReplay_GetNextSlot(&g_single_capture));
            App_ArmCapture();
            if (SignalSingleCaptureReplay_IsArmed(&g_single_capture))
                App_DrawStatus(8, 54, "WAIT SIGNAL", TFT_ST7789_YELLOW);
        } else if (g_page == 6U) App_Calibrate();
    }
}

/* main：初始化全部硬件模块和默认正弦输出；主循环先处理按键，再采集一帧并依次执行
 * 基本测量、三路测频、频谱、鲁棒、拟合/锁相等流水线。g_display_due 为真时才刷新
 * 当前页；SysTick 负责 5 ms 键盘扫描和 250 ms 显示节拍，避免 FFT/SPI 阻塞按键。 */
int main(void)
{
    signal_dual_adc_config_t adc_config = {
        SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U
    };
    signal_dac_dma_mspm0_config_t dac_config = {
        SIGNAL_DAC_UPDATE_RATE_HZ, CPUCLK_FREQ, 65536U
    };
    signal_comparator_config_t comparator_config;
    signal_timer_capture_mspm0_config_t capture_config = {
        CPUCLK_FREQ, SIGNAL_CAPTURE_INST_LOAD_VALUE
    };
    const signal_single_capture_replay_config_t capture_replay_config = {
        .adc_channel_a = g_raw_ch1,
        .adc_channel_b = g_raw_ch2,
        .adc_capacity_per_channel =
            SIGNAL_SINGLE_CAPTURE_MAX_SAMPLES *
            SIGNAL_SINGLE_CAPTURE_DMA_BLOCKS,
        .search_buffer = g_capture_search,
        .search_capacity = SIGNAL_SINGLE_CAPTURE_MAX_SAMPLES * 2U,
        .slot_samples = &g_capture_slots[0][0],
        .slot_sample_capacity =
            SIGNAL_SINGLE_CAPTURE_SLOTS *
            SIGNAL_SINGLE_CAPTURE_MAX_SAMPLES,
        .slot_lengths = g_capture_lengths,
        .slot_sample_rates_hz = g_capture_rates_hz,
        .slot_valid = g_capture_slot_valid,
        .replay_table = g_wave_table,
        .replay_table_capacity = SIGNAL_DAC_TABLE_COUNT,
        .samples_per_block = SIGNAL_SINGLE_CAPTURE_MAX_SAMPLES,
        .dma_block_count = SIGNAL_SINGLE_CAPTURE_DMA_BLOCKS,
        .slot_count = SIGNAL_SINGLE_CAPTURE_SLOTS,
        .pretrigger_samples = SIGNAL_SINGLE_CAPTURE_PRETRIGGER,
        .baseline_samples = SIGNAL_SINGLE_CAPTURE_BASELINE_SAMPLES,
        .edge_margin_samples = SIGNAL_SINGLE_CAPTURE_EDGE_MARGIN,
        .minimum_activity_codes = SIGNAL_SINGLE_CAPTURE_MIN_ACTIVITY,
        .quiet_tail_samples = SIGNAL_SINGLE_CAPTURE_QUIET_SAMPLES,
        .activity_run_samples = SIGNAL_SINGLE_CAPTURE_ACTIVITY_RUN,
        .adc_max_code = APP_ADC_MAX_CODE,
        .trigger_level_code = g_trigger_level,
        .trigger_hysteresis_code = g_trigger_hysteresis,
        .trigger_edge = SIGNAL_TRIGGER_EITHER,
        .requested_sample_rate_hz = 1000000U,
        .clear_trigger = App_ClearCaptureTrigger,
        .consume_trigger = App_ConsumeCaptureTrigger,
        .trigger_context = NULL
    };
    SYSCFG_DL_init();
    g_module_status = SignalDualADC_Init(&adc_config);
    if (g_module_status != SIGNAL_RESULT_OK) while (1) { }
    g_module_status = SignalSingleCaptureReplay_Init(&g_single_capture,
        &capture_replay_config);
    if (g_module_status != SIGNAL_RESULT_OK) while (1) { }
    {
        const signal_wave_output_config_t wave_config = {
            g_wave_table, SIGNAL_DAC_TABLE_COUNT,
            g_dac_output, SIGNAL_DAC_OUTPUT_COUNT,
            dac_config, SIGNAL_DAC_BITS, SIGNAL_ADC_VREF_V
        };
        g_module_status = SignalWaveOutput_Init(&wave_config);
    }
    if (g_module_status != SIGNAL_RESULT_OK) while (1) { }
    g_module_status = SignalComparatorZeroCross_MakeConfig(1.65f, 0.005f,
        &comparator_config);
    if (g_module_status != SIGNAL_RESULT_OK) while (1) { }
    /* COMP0 的比较参考为 VDDA-DAC。SysConfig 应生成 0x80（约 1.65 V）；
     * 这里再次写入，避免旧生成文件使用 0 V 门限导致 PA27 没有有效过零边沿。 */
    DL_COMP_setDACCode0(SIGNAL_COMP_INST, 128U);
    DL_COMP_setOutputInterruptEdge(SIGNAL_COMP_INST,
        DL_COMP_OUTPUT_INT_EDGE_RISING);
    DL_COMP_clearInterruptStatus(SIGNAL_COMP_INST,
        DL_COMP_INTERRUPT_OUTPUT_EDGE | DL_COMP_INTERRUPT_OUTPUT_EDGE_INV);
    DL_COMP_enableInterrupt(SIGNAL_COMP_INST,
        DL_COMP_INTERRUPT_OUTPUT_EDGE | DL_COMP_INTERRUPT_OUTPUT_EDGE_INV);
    NVIC_ClearPendingIRQ(SIGNAL_COMP_INST_INT_IRQN);
    NVIC_EnableIRQ(SIGNAL_COMP_INST_INT_IRQN);
    g_module_status = SignalTimerCapture_MSPM0_Init(&capture_config);
    if (g_module_status != SIGNAL_RESULT_OK) while (1) { }
    (void)SignalTimerCapture_MSPM0_Start();
    g_module_status = SignalTFTST7789_MSPM0_Init(&g_tft,
        TFT_ST7789_ROTATION_270, 0U, 0U);
    if (g_module_status != TFT_ST7789_OK) while (1) { }
    (void)SignalTFTST7789Text_DrawString(&g_tft, 8, 8,
        "SIGNAL ANALYZER", 1U, TFT_ST7789_WHITE, TFT_ST7789_BLACK);
    g_module_status = App_ApplyWaveform();
    if (g_module_status != SIGNAL_RESULT_OK) while (1) { }
    App_DrawStaticPage();
    if (SysTick_Config(CPUCLK_FREQ / 1000U) != 0U) while (1) { }

    while (1) {
        App_ProcessQueuedKeys();
        g_sample_rate_hz = App_SelectSampleRate(g_output_frequency_hz);
        if (g_page == 5U) {
            if (SignalSingleCaptureReplay_IsArmed(&g_single_capture)) {
                App_ServiceCapture();
            }
        } else if (App_AcquireFrame() == SIGNAL_RESULT_OK) {
            /* adc_to_voltage.md 的固定线性换算后，再复用既有两点增益/偏置
             * 校准模块；采集、量程和后续测量链的功能保持不变。 */
            App_RecipeADCToVoltage(g_raw_ch1, g_voltage,
                SIGNAL_SAMPLE_COUNT, SIGNAL_ADC_VREF_V, APP_ADC_MAX_CODE,
                1.0f, 0.0f);
            if (SignalADCGainOffsetCalibration_Apply(g_voltage, g_calibrated,
                    SIGNAL_SAMPLE_COUNT, &g_adc_calibration) == SIGNAL_ALGORITHM_OK) {
                App_BasicMeasurements();
                App_TimeFrequency();
                App_Spectrum();
                App_RobustMeasurement();
                App_SineFitAndLockIn();
                g_measurement_valid = true;
            }
        }
        /* 连续捕获期间不做 SPI 刷屏，DMA 捕获完成后再显示，减少漏掉短信号的风险。 */
        if (((g_page == 5U) || g_measurement_valid) &&
            g_display_due &&
            !SignalSingleCaptureReplay_IsArmed(&g_single_capture)) {
            g_display_due = false;
            App_DrawDynamicPage();
        }
        __WFI();
    }
}
