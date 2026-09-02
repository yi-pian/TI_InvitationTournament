/*
 * 24_C 总流程：ADC DMA 采样 -> 转电压/去直流 -> 统计、FFT、谐波和猝发检测 ->
 * ST7789 双路波形显示。ADC、FFT、窗函数、键盘和屏幕调用来自模块 README；
 * 动态采样率、猝发边沿判断、谐波修正、坐标轴缩放、按键队列和局部刷新由 main
 * 自己组合。改变量程改坐标轴参数，改变硬件引脚/触发源必须改 SysConfig。
 */
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "arm_const_structs.h"
#include "arm_math.h"
#include "signal_config.h"
#include "modules/signal_dual_adc_mspm0g3507.h"
#include "modules/signal_harmonic.h"
#include "modules/signal_tft_st7789_mspm0g3507.h"
#include "modules/signal_tft_st7789_font.h"
#include "modules/signal_tft_st7789.h"
#include "modules/signal_tft_waveform_st7789.h"
#include "modules/signal_timer_capture_mspm0g3507.h"
#include "ti_msp_dl_config.h"


#define FRAME_SAMPLES             SIGNAL_SAMPLE_COUNT
#define STITCHED_SAMPLES          (2U * FRAME_SAMPLES)
#define ADC_RAW_BUFFER_COUNT      (3U)
#define INVALID_BUFFER_INDEX      (UINT8_MAX)
#define FFT_BINS                  ((FRAME_SAMPLES / 2U) + 1U)
#define ADC_FULL_SCALE_CODE       (4095.0F)
#define PI_F                      (3.14159265358979323846F)
#define MIN_SIGNAL_VPP_V          (0.08F)
#define MIN_BURST_MS              (0.90F)
#define MAX_BURST_MS              (5.35F)
#define BURST_ARM_SAMPLE_RATE_HZ   SIGNAL_ADC_MAX_RATE_HZ
#define BURST_GATE_MIN_SPAN_CODE   (64U)
#define BURST_BASELINE_SAMPLES     (32U)
#define BURST_PENDING_MAX_FRAMES   (1U)
#define BURST_ANALOG_MIN_THRESHOLD_V (0.020F)
#define BURST_ANALOG_REL_THRESHOLD (0.04F)
#define BURST_BOUNDARY_PAD_US      (100U)
#define BURST_MARGIN_MAX_SAMPLES  (160U)
#define PERIODIC_DISPLAY_CYCLES   (3.0F)
#define PERIODIC_DISPLAY_MIN_SAMPLES (8U)
#define FREQUENCY_RANGE_TOLERANCE (0.05F)
#define WAVEFORM_HISTOGRAM_BINS   (8U)
#define DISPLAY_TEXT_HEIGHT       (12)
#define DISPLAY_PLOT_X            (40)
#define DISPLAY_PLOT_Y            (18)
#define DISPLAY_PLOT_WIDTH        (272U)
#define DISPLAY_PLOT_HEIGHT       (118U)

typedef enum {
    WAVE_NONE = 0,
    WAVE_SINE,
    WAVE_SAW,
    WAVE_PULSE,
    WAVE_COMPLEX,
    WAVE_BURST
} waveform_type_t;

typedef struct {
    float minimum_v;
    float maximum_v;
    float vpp_v;
    float rms_v;
    float mean_v;
} signal_statistics_t;

typedef struct {
    bool detected;
    size_t first;
    size_t last;
    float duration_ms;
    float maximum_amplitude_v;
} burst_result_t;

typedef struct {
    signal_harmonic_result_t band_energy;
    float peak_amplitude_v[SIGNAL_HARMONIC_MAX_ORDER + 1U];
} harmonic_analysis_t;

/* g_adc_raw：多帧 ADC 原始数据；g_burst_gate_raw：猝发门控通道；g_adc_raw_sample_rate_hz：
 * 每帧实际采样率；g_voltage_v：拼接后的电压；g_fft_data/g_fft_magnitude_v：FFT 工作区；
 * g_tft：屏幕句柄。其余局部变量中的 count 是点数，sample_rate_hz 是采样率，
 * threshold/axis range 分别是检测门限和显示量程。 */
/* 全局变量逐项说明：
 * g_adc_raw：双缓冲 ADC 测量通道；g_burst_gate_raw：同步门控通道；
 * g_adc_raw_sample_rate_hz：记录每块数据采集时的真实采样率，防止动态切换后用错时间轴；
 * g_voltage_v：两帧拼接后的浮点电压；g_fft_data：CMSIS Q15 复数 FFT 工作区；
 * g_fft_magnitude_v：0~Nyquist 幅度谱；g_tft：ST7789 句柄。
 * FRAME_SAMPLES/FFT_BINS 决定数组长度，修改时必须同时检查 RAM 和 CMSIS FFT 点数。 */
static uint16_t g_adc_raw[ADC_RAW_BUFFER_COUNT][FRAME_SAMPLES];
static uint16_t g_burst_gate_raw[ADC_RAW_BUFFER_COUNT][FRAME_SAMPLES];
static uint32_t g_adc_raw_sample_rate_hz[ADC_RAW_BUFFER_COUNT];
static float g_voltage_v[STITCHED_SAMPLES];
static q15_t g_fft_data[2U * FRAME_SAMPLES];
static float g_fft_magnitude_v[FFT_BINS];
static tft_st7789_t g_tft;

/* 函数索引：ClampFloat 限制数值安全范围；ADCCodeToGainRestoredVoltage/ConvertFrame
 * 做 ADC 换算；App_RemoveDC 去直流；CalculateStatistics 求均值/峰峰值/标准差；
 * HasBurstGateEdge/FindBurstGateEdge/DetectBurstFromAnalog 判断猝发起点和持续时间；
 * RunFFT、AnalyzeHarmonics 完成频谱，ClassifyWaveform 判断波形类型，WaveformName 返回
 * 屏幕名称；SelectSampleRate 选择动态采样率；SelectPeriodicDisplayWindow 选显示窗口；Draw* 是
 * 8x16 字段和坐标绘图；main 负责 ADC DMA、算法流水线、按键队列和局部刷新。samples
 * 是浮点采样数组，count 是有效点数，sample_rate_hz 是当前采样率，frequency_hz 是
 * 被测频率，x/y_range 是屏幕坐标轴量程。 */
static bool IsUsableMeasuredFrequency(float frequency_hz);

/* 自写安全辅助：将 value 限制在 minimum~maximum，防止坐标和增益越界。 */
static float ClampFloat(float value, float minimum, float maximum)
{
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

/* 自写换算：将 ADC code 按校准后的增益/偏置恢复为输入电压。 */
static float ADCCodeToGainRestoredVoltage(uint16_t code)
{
    float adc_voltage = (float)code * SIGNAL_ADC_VREF_V /
        ADC_FULL_SCALE_CODE;
    return adc_voltage / SIGNAL_CONDITION_GAIN;
}

/* 自写批处理：把 source 的 count 个 ADC 码转换到 destination 电压数组。 */
static void ConvertFrame(const uint16_t *source, float *destination,
    size_t count)
{
    size_t index;
    for (index = 0U; index < count; ++index) {
        destination[index] = ADCCodeToGainRestoredVoltage(source[index]);
    }
}

/* 模块调用组合：调用集成 RemoveDC 算法；samples 原地变为去直流数据。 */
static bool App_RemoveDC(float *samples, size_t count)
{
    float mean_voltage_v;

    if ((samples == NULL) || (count == 0U)) {
        return false;
    }

    /* 【CMSIS-DSP】先求均值，再用 offset 原地减去均值；本函数只包装模块调用。 */
    arm_mean_f32(samples, count, &mean_voltage_v);
    arm_offset_f32(samples, -mean_voltage_v, samples, count);
    return isfinite(mean_voltage_v);
}

/* 自写统计组合：调用 Mean/MinMax/RMS/Statistics 并返回当前帧统计结构体。 */
static signal_statistics_t CalculateStatistics(
    const float *samples, size_t count)
{
    signal_statistics_t result = {0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
    size_t index;
    float sum = 0.0F;
    float square_sum = 0.0F;

    if ((samples == NULL) || (count == 0U)) {
        return result;
    }

    result.minimum_v = samples[0];
    result.maximum_v = samples[0];
    for (index = 0U; index < count; ++index) {
        float value = samples[index];
        if (value < result.minimum_v) {
            result.minimum_v = value;
        }
        if (value > result.maximum_v) {
            result.maximum_v = value;
        }
        sum += value;
        square_sum += value * value;
    }
    result.mean_v = sum / (float)count;
    result.rms_v = sqrtf(square_sum / (float)count);
    result.vpp_v = result.maximum_v - result.minimum_v;
    return result;
}

/* 自写检测：判断门控数组是否出现超过门限的上升边沿。 */
static bool HasBurstGateEdge(const uint16_t *previous_gate_samples,
    const uint16_t *current_gate_samples, size_t frame_count)
{
    size_t count = 2U * frame_count;
    size_t index;
    uint16_t minimum_code = UINT16_MAX;
    uint16_t maximum_code = 0U;
    uint16_t midpoint_code;
    uint16_t hysteresis_code;
    bool gate_high;

    if ((previous_gate_samples == NULL) ||
        (current_gate_samples == NULL) || (frame_count == 0U)) {
        return false;
    }

    for (index = 0U; index < count; ++index) {
        uint16_t gate_code = (index < frame_count) ?
            previous_gate_samples[index] :
            current_gate_samples[index - frame_count];
        if (gate_code < minimum_code) {
            minimum_code = gate_code;
        }
        if (gate_code > maximum_code) {
            maximum_code = gate_code;
        }
    }
    if ((uint16_t)(maximum_code - minimum_code) <
        BURST_GATE_MIN_SPAN_CODE) {
        return false;
    }

    midpoint_code = minimum_code +
        (uint16_t)((maximum_code - minimum_code) / 2U);
    hysteresis_code = (uint16_t)((maximum_code - minimum_code) / 8U);
    if (hysteresis_code < 16U) {
        hysteresis_code = 16U;
    }
    gate_high = previous_gate_samples[0U] > midpoint_code;
    for (index = 1U; index < count; ++index) {
        uint16_t gate_code = (index < frame_count) ?
            previous_gate_samples[index] :
            current_gate_samples[index - frame_count];
        if (!gate_high && (gate_code >= midpoint_code + hysteresis_code)) {
            return true;
        }
        if (gate_high && (gate_code + hysteresis_code <= midpoint_code)) {
            return true;
        }
    }
    return false;
}

/* 自写检测：在前后两帧门控数据中定位猝发起始点，返回样本索引。 */
static bool FindBurstGateEdge(const uint16_t *previous_gate_samples,
    const uint16_t *current_gate_samples, size_t frame_count,
    size_t *edge_index)
{
    size_t count = 2U * frame_count;
    size_t index;
    uint16_t minimum_code = UINT16_MAX;
    uint16_t maximum_code = 0U;
    uint16_t low_threshold;
    uint16_t high_threshold;
    bool gate_high;

    if ((previous_gate_samples == NULL) ||
        (current_gate_samples == NULL) || (frame_count == 0U) ||
        (edge_index == NULL)) {
        return false;
    }

    for (index = 0U; index < count; ++index) {
        uint16_t gate_code = (index < frame_count) ?
            previous_gate_samples[index] :
            current_gate_samples[index - frame_count];
        if (gate_code < minimum_code) {
            minimum_code = gate_code;
        }
        if (gate_code > maximum_code) {
            maximum_code = gate_code;
        }
    }
    if ((uint16_t)(maximum_code - minimum_code) <
        BURST_GATE_MIN_SPAN_CODE) {
        return false;
    }

    low_threshold = minimum_code +
        (uint16_t)((maximum_code - minimum_code) * 3U / 8U);
    high_threshold = minimum_code +
        (uint16_t)((maximum_code - minimum_code) * 5U / 8U);
    gate_high = previous_gate_samples[0U] >= high_threshold;
    for (index = 1U; index < count; ++index) {
        uint16_t gate_code = (index < frame_count) ?
            previous_gate_samples[index] :
            current_gate_samples[index - frame_count];
        if (!gate_high && (gate_code >= high_threshold)) {
            *edge_index = index;
            return true;
        }
        if (gate_high && (gate_code <= low_threshold)) {
            /* Accept an active-low comparator as a valid event marker too.
             * ADC0 still decides the analog event boundaries and duration. */
            *edge_index = index;
            return true;
        }
    }
    return false;
}

/* 自写组合：根据门控边沿、持续时间和幅度判断是否为有效猝发信号。 */
static burst_result_t DetectBurstFromAnalog(
    const float *samples, size_t count, uint32_t sample_rate_hz)
{
    burst_result_t result = {false, 0U, 0U, 0.0F, 0.0F};
    size_t index;
    size_t edge_count;
    size_t boundary_padding;
    float first_mean = 0.0F;
    float last_mean = 0.0F;
    float first_variation = 0.0F;
    float last_variation = 0.0F;
    float baseline;
    float threshold;

    if ((samples == NULL) || (count < 128U) || (sample_rate_hz == 0U)) {
        return result;
    }

    edge_count = BURST_BASELINE_SAMPLES;
    if ((2U * edge_count) > count) {
        edge_count = count / 2U;
    }
    for (index = 0U; index < edge_count; ++index) {
        first_mean += samples[index];
        last_mean += samples[count - edge_count + index];
    }
    first_mean /= (float)edge_count;
    last_mean /= (float)edge_count;
    for (index = 0U; index < edge_count; ++index) {
        float first_delta = samples[index] - first_mean;
        float last_delta = samples[count - edge_count + index] - last_mean;
        first_variation += first_delta * first_delta;
        last_variation += last_delta * last_delta;
    }
    baseline = (first_variation < last_variation) ?
        first_mean : last_mean;

    for (index = 0U; index < count; ++index) {
        float amplitude = fabsf(samples[index] - baseline);
        if (amplitude > result.maximum_amplitude_v) {
            result.maximum_amplitude_v = amplitude;
        }
    }
    if (result.maximum_amplitude_v < (0.5F * MIN_SIGNAL_VPP_V)) {
        result.first = count;
        return result;
    }

    threshold = fmaxf(BURST_ANALOG_MIN_THRESHOLD_V,
        BURST_ANALOG_REL_THRESHOLD * result.maximum_amplitude_v);
    result.first = count;
    result.last = 0U;
    for (index = 0U; index < count; ++index) {
        if (fabsf(samples[index] - baseline) >= threshold) {
            if (result.first == count) {
                result.first = index;
            }
            result.last = index;
        }
    }
    if ((result.first == count) || (result.last < result.first)) {
        return result;
    }

    boundary_padding = ((size_t)sample_rate_hz *
        BURST_BOUNDARY_PAD_US + 999999U) / 1000000U;
    if (result.first > boundary_padding) {
        result.first -= boundary_padding;
    } else {
        result.first = 0U;
    }
    if (result.last + boundary_padding < count) {
        result.last += boundary_padding;
    } else {
        result.last = count - 1U;
    }
    result.duration_ms = 1000.0F *
        (float)(result.last - result.first + 1U) / (float)sample_rate_hz;
    result.detected = (result.duration_ms >= MIN_BURST_MS) &&
        (result.duration_ms <= MAX_BURST_MS);
    return result;
}

/* 模块调用组合：对 samples 去直流、加窗并调用 CMSIS FFT，输出幅度谱。 */
static void RunFFT(const float *samples, size_t count,
    uint32_t sample_rate_hz, float *fundamental_hz)
{
    size_t index;
    float scale = 0.0F;
    float coherent_gain = 0.0F;
    uint32_t minimum_bin;
    uint32_t maximum_bin;
    uint32_t strongest_bin;
    float strongest_magnitude;
    float candidate_threshold;

    if (fundamental_hz == NULL) {
        return;
    }
    *fundamental_hz = 0.0F;
    if ((samples == NULL) || (count < FRAME_SAMPLES) ||
        (sample_rate_hz == 0U)) {
        return;
    }

    for (index = 0U; index < FRAME_SAMPLES; ++index) {
        if (fabsf(samples[index]) > scale) {
            scale = fabsf(samples[index]);
        }
    }
    if (scale < 0.02F) {
        scale = 0.02F;
    }

    for (index = 0U; index < FRAME_SAMPLES; ++index) {
        float window = 0.5F - 0.5F * cosf(
            (2.0F * PI_F * (float)index) /
            (float)(FRAME_SAMPLES - 1U));
        float normalized = (samples[index] / scale) * window;
        int32_t fixed = (int32_t)(ClampFloat(normalized, -0.9999F,
            0.9999F) * 32767.0F);
        g_fft_data[2U * index] = (q15_t)fixed;
        g_fft_data[2U * index + 1U] = 0;
        coherent_gain += window;
    }
    coherent_gain /= (float)FRAME_SAMPLES;

    /* 【CMSIS-DSP FFT】执行 1024 点 Q15 复数 FFT；缩放和幅值换算在后续代码完成。 */
    arm_cfft_q15(&arm_cfft_sR_q15_len1024, g_fft_data, 0U, 1U);
    for (index = 0U; index < FFT_BINS; ++index) {
        float real = (float)g_fft_data[2U * index] / 32768.0F;
        float imaginary = (float)g_fft_data[2U * index + 1U] /
            32768.0F;
        g_fft_magnitude_v[index] =
            2.0F * scale * sqrtf(real * real + imaginary * imaginary) /
            coherent_gain;
    }
    g_fft_magnitude_v[0] = 0.0F;

    minimum_bin = (uint32_t)floorf(SIGNAL_EXPECTED_MIN_HZ *
        (float)FRAME_SAMPLES / (float)sample_rate_hz);
    maximum_bin = (uint32_t)ceilf(SIGNAL_EXPECTED_MAX_HZ *
        (float)FRAME_SAMPLES / (float)sample_rate_hz);
    if (minimum_bin < 1U) {
        minimum_bin = 1U;
    }
    if (maximum_bin >= (FFT_BINS - 1U)) {
        maximum_bin = FFT_BINS - 2U;
    }
    if (minimum_bin > maximum_bin) {
        *fundamental_hz = 0.0F;
        return;
    }

    strongest_bin = minimum_bin;
    strongest_magnitude = g_fft_magnitude_v[minimum_bin];
    for (index = minimum_bin + 1U; index <= maximum_bin; ++index) {
        if (g_fft_magnitude_v[index] > strongest_magnitude) {
            strongest_magnitude = g_fft_magnitude_v[index];
            strongest_bin = (uint32_t)index;
        }
    }

    candidate_threshold = fmaxf(0.03F, strongest_magnitude * 0.20F);
    for (index = minimum_bin; index <= strongest_bin; ++index) {
        if ((g_fft_magnitude_v[index] >= candidate_threshold) &&
            (g_fft_magnitude_v[index] >= g_fft_magnitude_v[index - 1U]) &&
            (g_fft_magnitude_v[index] >= g_fft_magnitude_v[index + 1U])) {
            strongest_bin = (uint32_t)index;
            break;
        }
    }

    if (strongest_magnitude < 0.02F) {
        *fundamental_hz = 0.0F;
    } else {
        float left = g_fft_magnitude_v[strongest_bin - 1U];
        float center = g_fft_magnitude_v[strongest_bin];
        float right = g_fft_magnitude_v[strongest_bin + 1U];
        float denominator = left + 2.0F * center + right;
        float offset = 0.0F;
        if (denominator > 1.0e-12F) {
            offset = ClampFloat(2.0F * (right - left) / denominator,
                -0.5F, 0.5F);
        }
        *fundamental_hz = ((float)strongest_bin + offset) *
            (float)sample_rate_hz / (float)FRAME_SAMPLES;
    }
}

/* 自写数学辅助：累加窗函数频响的 Dirichlet 核，用于峰值幅度修正。 */
static void AccumulateDirichletKernel(float bin_offset, float coefficient,
    float *real_sum, float *imaginary_sum)
{
    float magnitude;
    float phase;

    if (fabsf(bin_offset) < 1.0e-6F) {
        magnitude = (float)FRAME_SAMPLES;
    } else {
        magnitude = sinf(PI_F * bin_offset) /
            sinf(PI_F * bin_offset / (float)FRAME_SAMPLES);
    }
    phase = PI_F * bin_offset * (float)(FRAME_SAMPLES - 1U) /
        (float)FRAME_SAMPLES;
    *real_sum += coefficient * magnitude * cosf(phase);
    *imaginary_sum += coefficient * magnitude * sinf(phase);
}

/* 自写数学辅助：计算 Hann 窗在 bin_offset 处的响应。 */
static float HannResponseAtBinOffset(float bin_offset)
{
    float real_sum = 0.0F;
    float imaginary_sum = 0.0F;
    float window_bin_shift = (float)FRAME_SAMPLES /
        (float)(FRAME_SAMPLES - 1U);
    float coherent_sum = 0.5F * (float)(FRAME_SAMPLES - 1U);

    AccumulateDirichletKernel(bin_offset, 0.5F,
        &real_sum, &imaginary_sum);
    AccumulateDirichletKernel(bin_offset + window_bin_shift, -0.25F,
        &real_sum, &imaginary_sum);
    AccumulateDirichletKernel(bin_offset - window_bin_shift, -0.25F,
        &real_sum, &imaginary_sum);
    return sqrtf(real_sum * real_sum + imaginary_sum * imaginary_sum) /
        coherent_sum;
}

/* 自写估计：用 Hann 窗频响修正 FFT 基波幅值。 */
static float EstimateHannPeakAmplitude(float target_frequency_hz,
    uint32_t sample_rate_hz)
{
    float target_bin;
    float bin_offset;
    float response;
    uint32_t center_bin;

    if (!isfinite(target_frequency_hz) || (target_frequency_hz <= 0.0F) ||
        (sample_rate_hz == 0U) ||
        (target_frequency_hz >= (0.5F * (float)sample_rate_hz))) {
        return 0.0F;
    }
    target_bin = target_frequency_hz * (float)FRAME_SAMPLES /
        (float)sample_rate_hz;
    center_bin = (uint32_t)(target_bin + 0.5F);
    if ((center_bin == 0U) || (center_bin >= FFT_BINS)) {
        return 0.0F;
    }

    bin_offset = target_bin - (float)center_bin;
    response = HannResponseAtBinOffset(bin_offset);
    if (!isfinite(response) || (response < 0.80F)) {
        return 0.0F;
    }
    return g_fft_magnitude_v[center_bin] / response;
}

/* 模块调用组合：定位基波/H2/H3 并计算 THD、SNR、SFDR 等指标。 */
static void AnalyzeHarmonics(float fundamental_hz, uint32_t sample_rate_hz,
    harmonic_analysis_t *result)
{
    signal_harmonic_config_t config;
    float fundamental_bin;
    uint32_t index;

    result->band_energy.first_order = 1U;
    result->band_energy.last_order = 5U;
    for (index = 0U; index <= SIGNAL_HARMONIC_MAX_ORDER; ++index) {
        signal_harmonic_item_t *item = &result->band_energy.items[index];
        item->order = index;
        item->target_frequency_hz = 0.0F;
        item->target_fractional_bin = 0.0F;
        item->center_bin = 0U;
        item->start_bin = 0U;
        item->end_bin = 0U;
        item->energy = 0.0F;
        item->root_sum_square = 0.0F;
        result->peak_amplitude_v[index] = 0.0F;
    }
    if (!IsUsableMeasuredFrequency(fundamental_hz)) {
        return;
    }

    fundamental_bin = fundamental_hz * (float)FRAME_SAMPLES /
        (float)sample_rate_hz;
    config.fundamental_frequency_hz = fundamental_hz;
    config.first_order = 1U;
    config.last_order = 5U;
    config.radius_bins = (fundamental_bin >= 4.0F) ? 1U : 0U;
    /* 【Harmonic 模块】根据基波频率搜索各次谐波，main 不自己逐 bin 猜 H2/H3。 */
    (void)SignalHarmonic_Process(g_fft_magnitude_v, FFT_BINS,
        (float)sample_rate_hz, FRAME_SAMPLES, &config,
        &result->band_energy);

    for (index = 1U; index <= 5U; ++index) {
        float target_frequency_hz = fundamental_hz * (float)index;
        if (target_frequency_hz >= (0.5F * (float)sample_rate_hz)) {
            break;
        }
        result->band_energy.items[index].target_frequency_hz =
            target_frequency_hz;
        result->peak_amplitude_v[index] = EstimateHannPeakAmplitude(
            target_frequency_hz, sample_rate_hz);
    }
}

/* 自写判断：根据谐波比例粗略分类正弦、方波、三角或锯齿波。 */
static waveform_type_t ClassifyWaveform(const float *samples, size_t count,
    const signal_statistics_t *stats,
    const harmonic_analysis_t *harmonics, uint32_t sample_rate_hz,
    float fundamental_hz)
{
    size_t index;
    size_t flat_count = 0U;
    size_t rising_count = 0U;
    size_t falling_count = 0U;
    size_t slope_lag = 1U;
    size_t amplitude_histogram[WAVEFORM_HISTOGRAM_BINS] = {0U};
    float flat_band;
    float slope_band;
    float rising_slope_sum = 0.0F;
    float rising_slope_square_sum = 0.0F;
    float falling_slope_sum = 0.0F;
    float falling_slope_square_sum = 0.0F;
    float minimum_slope_variation = 1.0F;
    float maximum_slope_variation = 1.0F;
    float histogram_variation;
    float direction_ratio = 0.5F;
    float normalized_rms;
    float harmonic_ratio = 0.0F;
    float fundamental_amplitude;

    if ((samples == NULL) || (stats == NULL) || (harmonics == NULL) ||
        (count < 8U) ||
        (stats->vpp_v < MIN_SIGNAL_VPP_V)) {
        return WAVE_NONE;
    }

    if (IsUsableMeasuredFrequency(fundamental_hz) &&
        (sample_rate_hz != 0U)) {
        float period_samples = (float)sample_rate_hz / fundamental_hz;
        slope_lag = (size_t)(period_samples / 32.0F + 0.5F);
        if (slope_lag < 1U) {
            slope_lag = 1U;
        }
        if (slope_lag > (count / 16U)) {
            slope_lag = count / 16U;
        }
    }

    flat_band = stats->vpp_v * 0.12F;
    slope_band = stats->vpp_v * 0.01F;
    for (index = 0U; index < count; ++index) {
        float normalized_level = (samples[index] - stats->minimum_v) /
            stats->vpp_v;
        size_t histogram_bin = (size_t)(normalized_level *
            (float)WAVEFORM_HISTOGRAM_BINS);
        if (histogram_bin >= WAVEFORM_HISTOGRAM_BINS) {
            histogram_bin = WAVEFORM_HISTOGRAM_BINS - 1U;
        }
        amplitude_histogram[histogram_bin]++;
        if ((samples[index] <= stats->minimum_v + flat_band) ||
            (samples[index] >= stats->maximum_v - flat_band)) {
            flat_count++;
        }
        if (index >= slope_lag) {
            float difference = samples[index] - samples[index - slope_lag];
            if (difference > slope_band) {
                rising_count++;
                rising_slope_sum += difference;
                rising_slope_square_sum += difference * difference;
            } else if (difference < -slope_band) {
                falling_count++;
                difference = -difference;
                falling_slope_sum += difference;
                falling_slope_square_sum += difference * difference;
            }
        }
    }

    fundamental_amplitude = harmonics->peak_amplitude_v[1];
    if (fundamental_amplitude > 0.02F) {
        for (index = 2U; index <= 5U; ++index) {
            harmonic_ratio += harmonics->peak_amplitude_v[index];
        }
        harmonic_ratio /= fundamental_amplitude;
    }

    if ((rising_count != 0U) && (falling_count != 0U)) {
        size_t slope_count = rising_count + falling_count;
        size_t dominant = (rising_count > falling_count) ?
            rising_count : falling_count;
        float rising_mean = rising_slope_sum / (float)rising_count;
        float falling_mean = falling_slope_sum / (float)falling_count;
        float rising_variance = rising_slope_square_sum /
            (float)rising_count - rising_mean * rising_mean;
        float falling_variance = falling_slope_square_sum /
            (float)falling_count - falling_mean * falling_mean;
        float rising_variation;
        float falling_variation;
        rising_variance = fmaxf(rising_variance, 0.0F);
        falling_variance = fmaxf(falling_variance, 0.0F);
        rising_variation = sqrtf(rising_variance) / rising_mean;
        falling_variation = sqrtf(falling_variance) / falling_mean;
        direction_ratio = (float)dominant / (float)slope_count;
        minimum_slope_variation = fminf(rising_variation,
            falling_variation);
        maximum_slope_variation = fmaxf(rising_variation,
            falling_variation);
    }
    {
        float expected_bin_count = (float)count /
            (float)WAVEFORM_HISTOGRAM_BINS;
        float histogram_square_sum = 0.0F;
        for (index = 0U; index < WAVEFORM_HISTOGRAM_BINS; ++index) {
            float difference = (float)amplitude_histogram[index] -
                expected_bin_count;
            histogram_square_sum += difference * difference;
        }
        histogram_variation = sqrtf(histogram_square_sum /
            (float)WAVEFORM_HISTOGRAM_BINS) / expected_bin_count;
    }
    normalized_rms = 2.0F * stats->rms_v / stats->vpp_v;

    if ((float)flat_count / (float)count > 0.62F) {
        return WAVE_PULSE;
    }
    if (((rising_count + falling_count) > (count / 2U)) &&
        (direction_ratio > 0.82F) && (harmonic_ratio > 0.18F)) {
        return WAVE_SAW;
    }
    /* The contest UI groups symmetric and asymmetric triangles as SAW. */
    if (((rising_count + falling_count) > (count / 2U)) &&
        (normalized_rms > 0.48F) && (normalized_rms < 0.65F) &&
        (minimum_slope_variation < 0.25F) &&
        (maximum_slope_variation < 0.45F) &&
        (histogram_variation < 0.30F)) {
        return WAVE_SAW;
    }
    if (harmonic_ratio > 0.22F) {
        return WAVE_COMPLEX;
    }
    return WAVE_SINE;
}

/* 自写校验：判断测得频率是否落在题目允许范围内。 */
static bool IsUsableMeasuredFrequency(float frequency_hz)
{
    float minimum_hz = SIGNAL_EXPECTED_MIN_HZ *
        (1.0F - FREQUENCY_RANGE_TOLERANCE);
    float maximum_hz = SIGNAL_EXPECTED_MAX_HZ *
        (1.0F + FREQUENCY_RANGE_TOLERANCE);

    return isfinite(frequency_hz) && (frequency_hz >= minimum_hz) &&
        (frequency_hz <= maximum_hz);
}

/* 自写策略：根据 measured_frequency_hz 选择 ADC 采样率，保证奈奎斯特余量。 */
static uint32_t SelectSampleRate(float measured_frequency_hz)
{
    uint32_t rate;
    if (!IsUsableMeasuredFrequency(measured_frequency_hz)) {
        return SIGNAL_SAMPLE_RATE_HZ;
    }
    rate = (uint32_t)(20.0F * measured_frequency_hz + 0.5F);
    if (rate < 20000U) {
        rate = 20000U;
    }
    if (rate > SIGNAL_ADC_MAX_RATE_HZ) {
        rate = SIGNAL_ADC_MAX_RATE_HZ;
    }
    return rate;
}

/* 自写显示算法：从周期信号中选连续窗口，使屏幕尽量显示整数周期。 */
static void SelectPeriodicDisplayWindow(const float *samples,
    size_t sample_count, uint32_t sample_rate_hz, float frequency_hz,
    const float **display_samples, size_t *display_count)
{
    size_t desired_count;
    size_t last_start;
    size_t index;
    float mean = 0.0F;
    float desired_count_f;

    *display_samples = samples;
    *display_count = sample_count;
    if ((samples == NULL) || (sample_count == 0U) ||
        (sample_rate_hz == 0U) ||
        !IsUsableMeasuredFrequency(frequency_hz)) {
        return;
    }

    desired_count_f = PERIODIC_DISPLAY_CYCLES * (float)sample_rate_hz /
        frequency_hz;
    desired_count = (size_t)(desired_count_f + 0.5F);
    if (desired_count < PERIODIC_DISPLAY_MIN_SAMPLES) {
        desired_count = PERIODIC_DISPLAY_MIN_SAMPLES;
    }
    if (desired_count > sample_count) {
        desired_count = sample_count;
    }

    for (index = 0U; index < sample_count; ++index) {
        mean += samples[index];
    }
    mean /= (float)sample_count;

    last_start = sample_count - desired_count;
    for (index = 1U; index <= last_start; ++index) {
        if ((samples[index - 1U] <= mean) && (samples[index] > mean)) {
            *display_samples = &samples[index];
            break;
        }
    }
    *display_count = desired_count;
}

/* 自写显示辅助：把波形枚举转换为屏幕文字。 */
static const char *WaveformName(waveform_type_t type)
{
    switch (type) {
        case WAVE_SINE: return "SINE";
        case WAVE_SAW: return "SAW";
        case WAVE_PULSE: return "PULSE";
        case WAVE_COMPLEX: return "COMPLEX";
        case WAVE_BURST: return "BURST";
        default: return "NO SIGNAL";
    }
}

/* 自写包装：使用 ST7789 8x16 字库绘制文字。 */
static void DrawText(int32_t x, int32_t y, const char *text,
    uint16_t color)
{
    (void)TFT_ST7789_DrawString(&g_tft, x, y, text,
        TFT_ST7789_FONT_6X12, color, TFT_ST7789_BLACK, false, false);
}

/* 自写包装：绘制浮点数 value；decimals 是小数位数。 */
static void DrawFloat(int32_t x, int32_t y, float value, uint8_t decimals,
    uint16_t color)
{
    (void)TFT_ST7789_DrawFloat(&g_tft, x, y, value, decimals,
        TFT_ST7789_FONT_6X12, color, TFT_ST7789_BLACK, false);
}

/* 自写包装：绘制整数 value，适合频率、采样率和计数。 */
static void DrawInteger(int32_t x, int32_t y, int32_t value,
    uint16_t color)
{
    (void)TFT_ST7789_DrawInt32(&g_tft, x, y, value,
        TFT_ST7789_FONT_6X12, color, TFT_ST7789_BLACK, false);
}

/* 自写局部刷新：清除一个字段矩形，不触碰边框、网格和其他文字。 */
static void ClearDisplayField(int32_t x, int32_t y, int32_t width)
{
    (void)TFT_ST7789_FillRect(&g_tft, x, y, width,
        DISPLAY_TEXT_HEIGHT, TFT_ST7789_BLACK);
}

/* 自写局部刷新：先清除 width 宽字段，再绘制文字。 */
static void DrawTextField(int32_t x, int32_t y, int32_t width,
    const char *text, uint16_t color)
{
    ClearDisplayField(x, y, width);
    DrawText(x, y, text, color);
}

/* 自写局部刷新：先清除字段，再显示浮点测量值。 */
static void DrawFloatField(int32_t x, int32_t y, int32_t width,
    float value, uint8_t decimals, uint16_t color)
{
    ClearDisplayField(x, y, width);
    DrawFloat(x, y, value, decimals, color);
}

/* 自写局部刷新：先清除字段，再显示整数测量值。 */
static void DrawIntegerField(int32_t x, int32_t y, int32_t width,
    int32_t value, uint16_t color)
{
    ClearDisplayField(x, y, width);
    DrawInteger(x, y, value, color);
}

/* 自写显示：按 order 显示某次谐波幅值/频率。 */
static void DrawHarmonicValue(int32_t x, int32_t y, uint32_t order,
    const harmonic_analysis_t *harmonics)
{
    DrawIntegerField(x + 18, y, 32,
        (int32_t)(harmonics->band_energy.items[order].target_frequency_hz +
            0.5F),
        TFT_ST7789_WHITE);
    DrawFloatField(x + 56, y, 50,
        harmonics->peak_amplitude_v[order], 2U, TFT_ST7789_YELLOW);
}

/* 自写显示：只在启动或页面改变时画边框、标题、网格和固定标签。 */
static void DrawStaticScreen(void)
{
    (void)TFT_ST7789_FillScreen(&g_tft, TFT_ST7789_BLACK);

    DrawText(0, 2, "TYPE:", TFT_ST7789_CYAN);
    DrawText(0, 18, "2.75", TFT_ST7789_WHITE);
    DrawText(30, 18, "V", TFT_ST7789_CYAN);
    DrawText(24, 71, "0", TFT_ST7789_WHITE);
    DrawText(0, 124, "-2.75", TFT_ST7789_WHITE);
    DrawText(DISPLAY_PLOT_X, 137, "0", TFT_ST7789_WHITE);
    DrawText(238, 137, "t=", TFT_ST7789_WHITE);
    DrawText(292, 137, "ms", TFT_ST7789_WHITE);

    DrawText(0, 150, "F:", TFT_ST7789_CYAN);
    DrawText(66, 150, "Hz D:", TFT_ST7789_WHITE);
    DrawText(132, 150, "% Fs:", TFT_ST7789_WHITE);
    DrawText(204, 150, "S/s", TFT_ST7789_WHITE);

    DrawText(0, 162, "MAX:", TFT_ST7789_CYAN);
    DrawText(60, 162, "V MIN:", TFT_ST7789_WHITE);
    DrawText(132, 162, "V VPP:", TFT_ST7789_WHITE);
    DrawText(204, 162, "V RMS:", TFT_ST7789_WHITE);
    DrawText(276, 162, "V", TFT_ST7789_WHITE);

    DrawText(0, 178, "H1:", TFT_ST7789_CYAN);
    DrawText(50, 178, "/", TFT_ST7789_WHITE);
    DrawText(106, 178, "H2:", TFT_ST7789_CYAN);
    DrawText(156, 178, "/", TFT_ST7789_WHITE);
    DrawText(212, 178, "H3:", TFT_ST7789_CYAN);
    DrawText(262, 178, "/", TFT_ST7789_WHITE);
    DrawText(0, 194, "H4:", TFT_ST7789_CYAN);
    DrawText(50, 194, "/", TFT_ST7789_WHITE);
    DrawText(106, 194, "H5:", TFT_ST7789_CYAN);
    DrawText(156, 194, "/", TFT_ST7789_WHITE);
    DrawText(220, 194, "Hz/Vpk", TFT_ST7789_GREEN);

    DrawText(0, 214, "H1-H5: frequency Hz / peak amplitude V",
        TFT_ST7789_GREEN);
    (void)TFT_ST7789_DrawRect(&g_tft, DISPLAY_PLOT_X, DISPLAY_PLOT_Y,
        DISPLAY_PLOT_WIDTH, DISPLAY_PLOT_HEIGHT, TFT_ST7789_BLUE);
}

/* 自写显示组合：按当前页面刷新测量数字和双路波形，调用局部字段接口。 */
static void RenderScreen(const float *samples, size_t count,
    uint32_t sample_rate_hz, waveform_type_t waveform,
    const signal_statistics_t *stats,
    const signal_timer_capture_mspm0_result_t *capture,
    float fundamental_hz, const harmonic_analysis_t *harmonics,
    const burst_result_t *burst)
{
    signal_tft_waveform_st7789_result_t plot_result;
    signal_tft_waveform_st7789_config_t plot = {
        .x = DISPLAY_PLOT_X + 1,
        .y = DISPLAY_PLOT_Y + 1,
        .width = DISPLAY_PLOT_WIDTH - 2U,
        .height = DISPLAY_PLOT_HEIGHT - 2U,
        .mode = burst->detected ? SIGNAL_TFT_WAVEFORM_MIN_MAX_ENVELOPE :
            SIGNAL_TFT_WAVEFORM_DECIMATE,
        .scale_mode = SIGNAL_TFT_WAVEFORM_FIXED_SCALE,
        .minimum_value = -2.75F,
        .maximum_value = 2.75F,
        .baseline_value = 0.0F,
        .waveform_color = TFT_ST7789_YELLOW,
        .background_color = TFT_ST7789_BLACK,
        .grid_color = TFT_ST7789_RGB565(55U, 75U, 85U),
        .baseline_color = TFT_ST7789_CYAN,
        .horizontal_grid_divisions = 4U,
        .vertical_grid_divisions = 5U,
        .clear_background = true,
        .draw_grid = true,
        .draw_border = false,
        .draw_baseline = true,
    };
    const char *acquisition_state;
    uint16_t acquisition_state_color;
    float window_ms = 1000.0F * (float)count / (float)sample_rate_hz;

    if (burst->detected) {
        acquisition_state = "LOCKED BURST";
        acquisition_state_color = TFT_ST7789_RED;
    } else if (waveform == WAVE_NONE) {
        acquisition_state = "BURST ARMED";
        acquisition_state_color = TFT_ST7789_GREEN;
    } else {
        acquisition_state = "LIVE";
        acquisition_state_color = TFT_ST7789_GREEN;
    }

    DrawTextField(30, 2, 90, WaveformName(waveform),
        TFT_ST7789_WHITE);
    DrawTextField(126, 2, 90, acquisition_state,
        acquisition_state_color);

    /* 【ST7789 Waveform 模块】负责坐标轴裁剪、网格和连续曲线；main 只提供数据和配置。 */
    (void)SignalTFTWaveformST7789_Draw(
        &g_tft, samples, count, &plot, &plot_result);
    DrawFloatField(250, 137, 42, window_ms, 2U, TFT_ST7789_WHITE);

    DrawFloatField(12, 150, 54, capture->valid ? capture->frequency_hz :
        fundamental_hz, 1U, TFT_ST7789_WHITE);
    DrawFloatField(96, 150, 36,
        capture->valid ? capture->duty_percent : 0.0F,
        1U, TFT_ST7789_WHITE);
    DrawIntegerField(162, 150, 42, (int32_t)sample_rate_hz,
        TFT_ST7789_GREEN);

    DrawFloatField(24, 162, 36, stats->maximum_v, 2U,
        TFT_ST7789_WHITE);
    DrawFloatField(96, 162, 36, stats->minimum_v, 2U,
        TFT_ST7789_WHITE);
    DrawFloatField(168, 162, 36, stats->vpp_v, 2U,
        TFT_ST7789_YELLOW);
    DrawFloatField(240, 162, 36, stats->rms_v, 2U,
        TFT_ST7789_WHITE);

    DrawHarmonicValue(0, 178, 1U, harmonics);
    DrawHarmonicValue(106, 178, 2U, harmonics);
    DrawHarmonicValue(212, 178, 3U, harmonics);
    DrawHarmonicValue(0, 194, 4U, harmonics);
    DrawHarmonicValue(106, 194, 5U, harmonics);

    if (burst->detected) {
        ClearDisplayField(0, 214, 320);
        DrawText(0, 214, "BURST T:", TFT_ST7789_RED);
        DrawFloat(48, 214, burst->duration_ms, 2U, TFT_ST7789_WHITE);
        DrawText(84, 214, "ms Amax:", TFT_ST7789_WHITE);
        DrawFloat(132, 214, burst->maximum_amplitude_v, 2U,
            TFT_ST7789_YELLOW);
        DrawText(168, 214, "V", TFT_ST7789_WHITE);
    }
}

/* main：初始化 ADC/DMA、键盘和 ST7789；每帧根据测得频率选择采样率，执行去直流、
 * 统计、FFT、谐波/猝发分析，再把当前窗口送给 RenderScreen。A/B/C/D 等键值只改变
 * 量程、通道或显示模式；硬件中断负责采样完成，main 不在中断里做浮点和 SPI。 */
int main(void)
{
    /* ===== 比赛步骤 1：双路同步 ADC 与 Timer Capture ===== */
    signal_dual_adc_config_t adc_config = {
        SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, UINT16_MAX
    };
    signal_timer_capture_mspm0_config_t capture_config = {
        SIGNAL_CAPTURE_TIMER_HZ, SIGNAL_CAPTURE_INST_LOAD_VALUE
    };
    signal_timer_capture_mspm0_result_t capture = {0U, 0U, 0.0F, 0.0F,
        false};
    uint32_t capture_sample_rate_hz;
    uint32_t pending_sample_rate_hz;
    uint8_t capture_buffer = 0U;
    uint8_t previous_buffer = INVALID_BUFFER_INDEX;
    uint8_t burst_pending_frames = 0U;
    uint8_t burst_trigger_block = INVALID_BUFFER_INDEX;
    uint32_t burst_continuous_consumed_sequence = 0U;
    uint32_t burst_trigger_sequence = 0U;
    bool quiet_screen_drawn = false;
    bool burst_trigger_latched = false;
    bool burst_continuous_mode = false;
    bool burst_waiting_post_block = false;

    SYSCFG_DL_init();
    DL_TimerG_setCoreHaltBehavior(SIGNAL_CAPTURE_INST,
        DL_TIMER_CORE_HALT_IMMEDIATE);

    /* 【ST7789 MSPM0 模块】初始化 SPI 屏幕并设置横屏。 */
    (void)SignalTFTST7789_MSPM0_Init(&g_tft, TFT_ST7789_ROTATION_270, 0U, 0U);
    DrawStaticScreen();
    /* 【双 ADC 模块】初始化同步触发、DMA 和采样率。 */
    (void)SignalDualADC_Init(&adc_config);
    /* 【Timer Capture 模块】初始化硬件测频通道，用作动态采样率依据。 */
    (void)SignalTimerCapture_MSPM0_Init(&capture_config);
    (void)SignalTimerCapture_MSPM0_Start();
    capture_sample_rate_hz = SignalDualADC_GetConfiguredRate();
    pending_sample_rate_hz = capture_sample_rate_hz;
    g_adc_raw_sample_rate_hz[capture_buffer] = capture_sample_rate_hz;
    if (SignalDualADC_Start(g_adc_raw[capture_buffer],
        g_burst_gate_raw[capture_buffer], FRAME_SAMPLES) != SIGNAL_RESULT_OK) {
        while (1) {
            __WFI();
        }
    }

    while (1) {
        /* ===== 比赛步骤 2：帧换算、去直流与时域统计 ===== */
        const float *periodic_samples;
        const float *display_samples;
        const float *fft_samples;
        uint32_t sample_rate_hz;
        uint32_t desired_rate_hz;
        uint8_t completed_buffer;
        uint8_t next_buffer;
        bool have_previous;
        bool gate_flag;
        bool burst_pending;
        size_t display_count = FRAME_SAMPLES;
        signal_statistics_t stats;
        signal_statistics_t classification_stats;
        harmonic_analysis_t harmonics;
        burst_result_t burst = {false, 0U, 0U, 0.0F, 0.0F};
        waveform_type_t waveform;
        float fft_fundamental_hz;
        float periodic_frequency_hz;

        if (burst_continuous_mode) {
            uint32_t block_sequence;
            size_t gate_edge_index;

            if (!SignalDualADC_GetContinuousSnapshot(&block_sequence,
                    &completed_buffer)) {
                __WFI();
                continue;
            }
            if (block_sequence == burst_continuous_consumed_sequence) {
                __WFI();
                continue;
            }
            if ((block_sequence - burst_continuous_consumed_sequence) >
                ADC_RAW_BUFFER_COUNT) {
                burst_trigger_latched = false;
                burst_waiting_post_block = false;
                burst_trigger_block = INVALID_BUFFER_INDEX;
            }
            burst_continuous_consumed_sequence = block_sequence;
            if (completed_buffer >= ADC_RAW_BUFFER_COUNT) {
                continue;
            }
            /* 【Timer Capture 模块】读取最新周期和频率；无有效边沿时保留上一策略。 */
            (void)SignalTimerCapture_MSPM0_GetResult(&capture);

            if (!burst_trigger_latched && (block_sequence >= 2U)) {
                previous_buffer = (uint8_t)((completed_buffer +
                    ADC_RAW_BUFFER_COUNT - 1U) % ADC_RAW_BUFFER_COUNT);
                if (FindBurstGateEdge(
                        g_burst_gate_raw[previous_buffer],
                        g_burst_gate_raw[completed_buffer], FRAME_SAMPLES,
                        &gate_edge_index)) {
                    burst_trigger_latched = true;
                    if (gate_edge_index < FRAME_SAMPLES) {
                        burst_trigger_block = previous_buffer;
                        burst_trigger_sequence = block_sequence - 1U;
                        burst_waiting_post_block = false;
                    } else {
                        burst_trigger_block = completed_buffer;
                        burst_trigger_sequence = block_sequence;
                        burst_waiting_post_block = true;
                    }
                }
            }

            if (!burst_trigger_latched ||
                (burst_waiting_post_block &&
                 (block_sequence <= burst_trigger_sequence))) {
                continue;
            }

            SignalDualADC_Stop();
            sample_rate_hz = SignalDualADC_GetConfiguredRate();
            next_buffer = (uint8_t)((burst_trigger_block + 1U) %
                ADC_RAW_BUFFER_COUNT);
            ConvertFrame(g_adc_raw[burst_trigger_block], g_voltage_v,
                FRAME_SAMPLES);
            ConvertFrame(g_adc_raw[next_buffer],
                &g_voltage_v[FRAME_SAMPLES], FRAME_SAMPLES);
            if (!App_RemoveDC(g_voltage_v, STITCHED_SAMPLES)) {
                burst.detected = false;
            } else {
                burst = DetectBurstFromAnalog(g_voltage_v,
                    STITCHED_SAMPLES, sample_rate_hz);
            }

            if (!burst.detected) {
                signal_statistics_t armed_stats = CalculateStatistics(
                    g_voltage_v, STITCHED_SAMPLES);

                burst_trigger_latched = false;
                burst_waiting_post_block = false;
                burst_trigger_block = INVALID_BUFFER_INDEX;
                burst_continuous_consumed_sequence = 0U;
                if (armed_stats.vpp_v >= MIN_SIGNAL_VPP_V) {
                    /* A sustained waveform is not a 1-5 ms event.  Return to
                     * the normal finite-frame path so frequency adaptation,
                     * waveform classification and display continue working. */
                    burst_continuous_mode = false;
                    quiet_screen_drawn = false;
                    previous_buffer = INVALID_BUFFER_INDEX;
                    capture_buffer = 0U;
                    capture_sample_rate_hz = sample_rate_hz;
                    pending_sample_rate_hz = sample_rate_hz;
                    g_adc_raw_sample_rate_hz[capture_buffer] =
                        capture_sample_rate_hz;
                    if (SignalDualADC_Start(g_adc_raw[capture_buffer],
                            g_burst_gate_raw[capture_buffer], FRAME_SAMPLES) !=
                        SIGNAL_RESULT_OK) {
                        while (1) {
                            __WFI();
                        }
                    }
                    continue;
                }
                if (SignalDualADC_StartContinuous(&g_adc_raw[0][0],
                        &g_burst_gate_raw[0][0], FRAME_SAMPLES,
                        ADC_RAW_BUFFER_COUNT) != SIGNAL_RESULT_OK) {
                    while (1) {
                        __WFI();
                    }
                }
                continue;
            }

            {
                size_t margin = sample_rate_hz / 1000U;
                size_t view_start;
                size_t view_end;
                size_t fft_start;
                if (margin > BURST_MARGIN_MAX_SAMPLES) {
                    margin = BURST_MARGIN_MAX_SAMPLES;
                }
                view_start = (burst.first > margin) ?
                    burst.first - margin : 0U;
                view_end = burst.last + margin + 1U;
                if (view_end > STITCHED_SAMPLES) {
                    view_end = STITCHED_SAMPLES;
                }
                display_samples = &g_voltage_v[view_start];
                display_count = view_end - view_start;
                fft_start = (burst.first > (FRAME_SAMPLES / 4U)) ?
                    burst.first - (FRAME_SAMPLES / 4U) : 0U;
                if (fft_start + FRAME_SAMPLES > STITCHED_SAMPLES) {
                    fft_start = STITCHED_SAMPLES - FRAME_SAMPLES;
                }
                fft_samples = &g_voltage_v[fft_start];
            }
            RunFFT(fft_samples, FRAME_SAMPLES, sample_rate_hz,
                &fft_fundamental_hz);
            AnalyzeHarmonics(fft_fundamental_hz, sample_rate_hz,
                &harmonics);
            stats = CalculateStatistics(display_samples, display_count);
            RenderScreen(display_samples, display_count, sample_rate_hz,
                WAVE_BURST, &stats, &capture, fft_fundamental_hz,
                &harmonics, &burst);
            while (1) {
                __WFI();
            }
        }

        while (!SignalDualADC_IsFinished()) {
            __WFI();
        }

        completed_buffer = capture_buffer;
        sample_rate_hz = g_adc_raw_sample_rate_hz[completed_buffer];
        if (pending_sample_rate_hz != capture_sample_rate_hz) {
            /* 【双 ADC 模块】仅在当前 DMA 停止后修改采样率，避免中途改变一帧时间基准。 */
            if (SignalDualADC_SetSampleRate(pending_sample_rate_hz) ==
                SIGNAL_RESULT_OK) {
                capture_sample_rate_hz =
                    SignalDualADC_GetConfiguredRate();
            }
        }

        /* Re-arm both ADCs before doing FFT/TFT work. */
        next_buffer = (uint8_t)((completed_buffer + 1U) %
            ADC_RAW_BUFFER_COUNT);
        g_adc_raw_sample_rate_hz[next_buffer] = capture_sample_rate_hz;
        if (SignalDualADC_Start(g_adc_raw[next_buffer],
            g_burst_gate_raw[next_buffer], FRAME_SAMPLES) != SIGNAL_RESULT_OK) {
            __WFI();
            continue;
        }
        capture_buffer = next_buffer;
        (void)SignalTimerCapture_MSPM0_GetResult(&capture);

        have_previous = (previous_buffer != INVALID_BUFFER_INDEX) &&
            (g_adc_raw_sample_rate_hz[previous_buffer] == sample_rate_hz);
        if (have_previous) {
            ConvertFrame(g_adc_raw[previous_buffer], g_voltage_v,
                FRAME_SAMPLES);
            ConvertFrame(g_adc_raw[completed_buffer],
                &g_voltage_v[FRAME_SAMPLES], FRAME_SAMPLES);
        } else {
            ConvertFrame(g_adc_raw[completed_buffer], g_voltage_v,
                FRAME_SAMPLES);
        }

        if (!App_RemoveDC(g_voltage_v,
             have_previous ? STITCHED_SAMPLES : FRAME_SAMPLES)) {
            previous_buffer = completed_buffer;
            quiet_screen_drawn = false;
            continue;
        }

        if (have_previous) {
            periodic_samples = &g_voltage_v[FRAME_SAMPLES];
            gate_flag = HasBurstGateEdge(
                g_burst_gate_raw[previous_buffer],
                g_burst_gate_raw[completed_buffer], FRAME_SAMPLES);
            if (gate_flag) {
                /* ADC1 only latches an observed level transition. All burst
                 * boundaries and measurements come from the ADC0 waveform. */
                if (!burst_trigger_latched) {
                    burst_pending_frames = 0U;
                }
                burst_trigger_latched = true;
            }
            if (burst_trigger_latched) {
                burst = DetectBurstFromAnalog(g_voltage_v,
                    STITCHED_SAMPLES, sample_rate_hz);
            }
        } else {
            periodic_samples = g_voltage_v;
            gate_flag = false;
        }

        classification_stats = CalculateStatistics(periodic_samples,
            FRAME_SAMPLES);
        burst_pending = have_previous && burst_trigger_latched &&
            !burst.detected && (burst.first < STITCHED_SAMPLES) &&
            (burst.last >=
                (STITCHED_SAMPLES - BURST_BASELINE_SAMPLES));

        /* A burst that reaches the newest frame edge needs the next frame's
         * post-trigger baseline. Defer all expensive work until it arrives. */
        if (burst_pending &&
            (burst_pending_frames < BURST_PENDING_MAX_FRAMES)) {
            burst_pending_frames++;
            previous_buffer = completed_buffer;
            quiet_screen_drawn = false;
            continue;
        }

        if (!burst.detected) {
            /* A trigger with no valid short analog event is a normal
             * periodic signal, a noise pulse, or an over-long activity span. */
            burst_trigger_latched = false;
            burst_pending_frames = 0U;
        } else {
            burst_pending_frames = 0U;
        }

        /* Once NO SIGNAL is visible, quiet frames only run the burst scanner.
         * This keeps processing shorter than a DMA frame while armed. */
        if (!burst.detected &&
            (classification_stats.vpp_v < MIN_SIGNAL_VPP_V)) {
            pending_sample_rate_hz = BURST_ARM_SAMPLE_RATE_HZ;
            /* ST7789 full-page drawing is relatively slow.  Arm continuous
             * acquisition before touching the display, so a one-shot burst
             * cannot fall into a TFT-induced blind interval. */
            SignalDualADC_Stop();
            if (SignalDualADC_SetSampleRate(BURST_ARM_SAMPLE_RATE_HZ) !=
                SIGNAL_RESULT_OK) {
                while (1) {
                    __WFI();
                }
            }
            capture_sample_rate_hz = SignalDualADC_GetConfiguredRate();
            burst_continuous_consumed_sequence = 0U;
            burst_trigger_latched = false;
            burst_waiting_post_block = false;
            burst_trigger_block = INVALID_BUFFER_INDEX;
            if (SignalDualADC_StartContinuous(&g_adc_raw[0][0],
                    &g_burst_gate_raw[0][0], FRAME_SAMPLES,
                    ADC_RAW_BUFFER_COUNT) != SIGNAL_RESULT_OK) {
                while (1) {
                    __WFI();
                }
            }
            burst_continuous_mode = true;
            previous_buffer = INVALID_BUFFER_INDEX;
            if (!quiet_screen_drawn) {
                /* Only these two small fields change while armed.  The ISR
                 * continues rotating the three DMA blocks during this write. */
                DrawTextField(30, 2, 90, "NO SIGNAL", TFT_ST7789_WHITE);
                DrawTextField(126, 2, 90, "BURST ARMED",
                    TFT_ST7789_GREEN);
                quiet_screen_drawn = true;
            }
            continue;
        }
        quiet_screen_drawn = false;

        display_samples = periodic_samples;
        fft_samples = periodic_samples;
        if (burst.detected) {
            size_t margin = sample_rate_hz / 1000U;
            size_t view_start;
            size_t view_end;
            size_t fft_start;
            if (margin > BURST_MARGIN_MAX_SAMPLES) {
                margin = BURST_MARGIN_MAX_SAMPLES;
            }
            view_start = (burst.first > margin) ? burst.first - margin : 0U;
            view_end = burst.last + margin + 1U;
            if (view_end > STITCHED_SAMPLES) {
                view_end = STITCHED_SAMPLES;
            }
            display_samples = &g_voltage_v[view_start];
            display_count = view_end - view_start;
            fft_start = (burst.first > (FRAME_SAMPLES / 4U)) ?
                burst.first - (FRAME_SAMPLES / 4U) : 0U;
            if (fft_start + FRAME_SAMPLES > STITCHED_SAMPLES) {
                fft_start = STITCHED_SAMPLES - FRAME_SAMPLES;
            }
            fft_samples = &g_voltage_v[fft_start];
        }

        /* ===== 比赛步骤 3：FFT、谐波与波形分类 ===== */
        RunFFT(fft_samples, FRAME_SAMPLES, sample_rate_hz,
            &fft_fundamental_hz);
        AnalyzeHarmonics(fft_fundamental_hz, sample_rate_hz, &harmonics);
        if (burst.detected) {
            waveform = WAVE_BURST;
            SignalDualADC_Stop();
        } else {
            waveform = ClassifyWaveform(periodic_samples, FRAME_SAMPLES,
                &classification_stats, &harmonics, sample_rate_hz,
                fft_fundamental_hz);
            periodic_frequency_hz = fft_fundamental_hz;
            if ((waveform != WAVE_COMPLEX) && capture.valid &&
                IsUsableMeasuredFrequency(capture.frequency_hz)) {
                periodic_frequency_hz = capture.frequency_hz;
            }
            SelectPeriodicDisplayWindow(periodic_samples, FRAME_SAMPLES,
                sample_rate_hz, periodic_frequency_hz, &display_samples,
                &display_count);

            /* Apply adaptive Fs at the next DMA boundary. Quiet/burst frames
             * never overwrite this periodic-signal decision. */
            if (IsUsableMeasuredFrequency(periodic_frequency_hz)) {
                /* Hardware capture remains preferred.  FFT is the fallback
                 * when PB2 has not produced a valid period yet; without it,
                 * 100 kS/s x 1024 points cannot contain three 100 Hz cycles. */
                desired_rate_hz = SelectSampleRate(periodic_frequency_hz);
                pending_sample_rate_hz = desired_rate_hz;
            }
        }
        stats = CalculateStatistics(display_samples, display_count);

        /* ===== 比赛步骤 4：ST7789 局部波形显示 ===== */
        RenderScreen(display_samples, display_count, sample_rate_hz,
            waveform, &stats, &capture, fft_fundamental_hz, &harmonics,
            &burst);

        if (burst.detected) {
            while (1) {
                __WFI();
            }
        }
        previous_buffer = completed_buffer;
    }
}
