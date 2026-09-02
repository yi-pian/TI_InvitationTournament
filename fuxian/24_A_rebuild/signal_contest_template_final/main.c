/*
 * 24_A 总流程：DDS/波表/DAC DMA 输出测试信号，ADC 采样后调用 Mean、AC RMS、
 * 频率、功率等集成算法，最后由 ST7789 和矩阵键盘完成四个比赛子题的人机交互。
 * signal_* 函数是模块 README 的复制调用；App_RunQuestion*、App_HandleKey、
 * TFT 行绘制和 main 状态机是本文件为串联模块补写的少量逻辑。默认参数在
 * APP_DEFAULT_* 附近修改，硬件资源必须在 SysConfig 修改。
 */
/**
 * @file main_template.c
 * @brief 2024 A op-amp tester: Q1 source, Q2 UGBW, Q3 slew rate, Q4 power.
 *
 * Keypad:
 *   A/B/C/D -> mode 1/2/3/4
 *   Q1: '*' selects FREQ/VPP, digits enter a value, '#' applies it
 *   Q2/Q3/Q4: '#' repeats the automatic measurement
 *
 * SysConfig generated files are never edited here. See 24A_Q2_Q4_BEGINNER_GUIDE.md.
 */

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "arm_math.h"
#include "ti_msp_dl_config.h"

#include "ad9850.h"
#include "ad9850_mspm0_platform.h"
#include "signal_adc_dma.h"
#include "signal_matrix_keypad_4x4.h"
#include "signal_robust_peak_to_peak.h"
#include "signal_slew_rate.h"
#include "signal_static_power.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_mspm0g3507.h"
#include "signal_tft_st7789_font.h"
#include "signal_vca820_gain_control.h"


/* ============================ Hardware facts ============================ */

#define DDS_REFERENCE_CLOCK_HZ          (125000000U)

/* VCA820 theoretical curve supplied by the user: K * RF/RG = 10. */
#define VCA820_GAIN_MAX                 (10.0f)
#define VCA820_VCTRL0_V                 (0.85f)
#define VCA820_VSLOPE_V                 (0.09f)
#define VCA820_CTRL_MIN_V               (0.0f)
#define VCA820_CTRL_MAX_V               (2.0f)

/*
 * The board was previously measured at about 1 Vpp. If the real DDS sine
 * amplitude is different, change this one constant after measuring it.
 */
#define DDS_THEORETICAL_VPP_V           (0.35f)
#define DAC_REFERENCE_V                 (3.3f)
#define DAC_FULL_SCALE_CODE             (4095.0f)

/*
 * SIGNAL_ADC is physical ADC0 and is the waveform-measurement input.
 * POWER_ADC is physical ADC1 and is the quiescent-current voltage input.
 */
#define APP_ADC_MAX_CODE                (4095U)
#define APP_ADC_REFERENCE_V             (3.3f)
#define APP_WAVE_SAMPLE_RATE_HZ         (3555556U)
#define APP_WAVE_SAMPLE_COUNT           (3072U)
#define APP_POWER_SAMPLE_COUNT          (256U)
#define APP_TIMER_MAX_COUNT             (65536U)

/* ============================== Q1 config =============================== */

#define APP_FREQUENCY_MIN_HZ            (1U)
#define APP_FREQUENCY_MAX_HZ            (2000000U)
#define APP_OUTPUT_VPP_MIN_CV           (10U)
#define APP_OUTPUT_VPP_MAX_CV           (1000U)
#define APP_KEYPAD_INPUT_MAX_DIGITS     (7U)

/* ============================== Q2 config =============================== */

/*
 * 20 mVpp is the signal applied to the DUT. This is physically possible only
 * when the DDS sine amplitude is around 1 Vpp, not 1 mVpp.
 */
#define MODE2_DUT_INPUT_VPP_V           (0.080f)
#define MODE2_SWEEP_START_HZ            (10000U)
#define MODE2_SWEEP_END_HZ              (2000000U)
#define MODE2_SWEEP_STEP_HZ             (25000U)
#define MODE2_SETTLE_US                 (1000U)
#define MODE2_FRAMES_PER_POINT          (3U)
#define MODE2_MINUS_3DB_RATIO           (0.70710678f)
#define MODE2_MIN_VALID_AC_RMS_V        (0.001f)

/* ============================== Q3 config =============================== */

#define MODE3_TEST_FREQUENCY_HZ         (5000U)
#define MODE3_WAVE_SAMPLE_RATE_HZ       (2000000U)
#define MODE3_SETTLE_US                 (1000U)
#define MODE3_ADC_TO_DUT_OUTPUT_SCALE   (3.0f)
#define MODE3_CONDITIONED_MIN_VPP_V     (2.4f)
#define MODE3_ROBUST_LOW_QUANTILE       (0.05f)
#define MODE3_ROBUST_HIGH_QUANTILE      (0.95f)
#define MODE3_EDGE_LOW_RATIO            (0.20f)
#define MODE3_EDGE_HIGH_RATIO           (0.80f)

/* ============================== Q4 config =============================== */

/*
 * These are intentionally centralized calibration constants.
 * Change the resistor to the real fitted value before final calibration.
 */
#define MODE4_SHUNT_RESISTANCE_OHM      (750.0f)
#define MODE4_SUPPLY_VOLTAGE_V          (24.0f)
#define MODE4_RAIL_COUNT_FACTOR         (1.0f)
#define MODE4_ADC_INPUT_SCALE           (1.0f)
#define MODE4_ADC_OFFSET_V              (0.0f)
#define MODE4_DDS_POWERDOWN_SETTLE_US   (2000U)

/* ============================== App types =============================== */

typedef enum {
    APP_MODE_QUESTION_1 = 0,
    APP_MODE_QUESTION_2,
    APP_MODE_QUESTION_3,
    APP_MODE_QUESTION_4
} app_mode_t;

typedef enum {
    APP_Q1_INPUT_FREQUENCY = 0,
    APP_Q1_INPUT_VPP
} app_q1_input_field_t;

typedef enum {
    APP_MEAS_READY = 0,
    APP_MEAS_RUNNING,
    APP_MEAS_DONE,
    APP_MEAS_LIMIT
} app_measurement_state_t;

/* ============================= App storage ============================== */

typedef union {
    uint16_t raw[APP_WAVE_SAMPLE_COUNT];
    float robust_workspace[APP_WAVE_SAMPLE_COUNT];
} app_wave_capture_buffer_t;

/* 缓冲区和状态变量：g_wave_capture 保存 ADC 波形；g_wave_voltage 是换算后的电压；
 * g_power_* 保存功率题采样；g_target_* 是键盘设定的输出参数；g_mode2/3/4_* 是
 * 三个测量题的结果；g_app_mode 是当前页面；g_keypad_input_* 是数字预输入；
 * g_tft_dirty、g_q1_*_dirty 是局部刷新的脏标志；config 结构体来自模块 README。 */
static app_wave_capture_buffer_t g_wave_capture;
static float g_wave_voltage[APP_WAVE_SAMPLE_COUNT];

static uint16_t g_power_raw[APP_POWER_SAMPLE_COUNT];
static float g_power_voltage[APP_POWER_SAMPLE_COUNT];

/* 参数/结果变量逐项说明：
 * g_target_output_vpp：Q1 目标输出峰峰值，可改默认 2.0 V；g_vca820_dac_code：控制码结果；
 * g_target_frequency_hz：Q1 DDS 频率，可改默认 1000 Hz；g_mode2_*：UGBW 测量结果；
 * g_mode3_*：上升/下降压摆率、时间和 Vpp；g_mode4_*：分流电压、电流和功率；
 * g_app_mode：当前 Q1~Q4；g_q1_input_field：当前编辑频率还是 Vpp；
 * g_keypad_input_value/digits/active：数字预输入值、位数、输入状态；
 * g_measurement_state/requested：自动测量状态和请求；g_tft_dirty：需要重画页面；
 * g_q1_input_dirty/value_dirty：只更新 Q1 输入或数值区域。
 * 默认值可在此处或 APP_* 宏修改；测量结果变量不应手动赋固定值。 */
static float g_target_output_vpp = 2.0f;
static uint16_t g_vca820_dac_code;
static uint32_t g_target_frequency_hz = 1000U;

static float g_mode2_ugbw_hz;
static float g_mode2_reference_ac_rms_v;
static float g_mode2_cutoff_ac_rms_v;
static float g_mode2_mean_voltage_v;
static bool g_mode2_above_limit;

static float g_mode3_rise_slew_rate_v_per_us;
static float g_mode3_fall_slew_rate_v_per_us;
static float g_mode3_rise_time_us;
static float g_mode3_fall_time_us;
static float g_mode3_output_vpp_v;

static float g_mode4_shunt_voltage_v;
static float g_mode4_current_ma;
static float g_mode4_power_mw;

static app_mode_t g_app_mode = APP_MODE_QUESTION_1;
static app_q1_input_field_t g_q1_input_field = APP_Q1_INPUT_FREQUENCY;
static uint32_t g_keypad_input_value;
static uint8_t g_keypad_input_digits;
static bool g_keypad_input_active;

static app_measurement_state_t g_measurement_state = APP_MEAS_DONE;
static bool g_measurement_requested;

static tft_st7789_t g_tft;
static bool g_tft_dirty = true;
static bool g_q1_input_dirty;
static bool g_q1_value_dirty;

/* ============================== AD9850 ================================= */

static ad9850_t g_ad9850;

static ad9850_mspm0_platform_t g_ad9850_platform = {
    .w_clk_port = DDS_GPIO_PORT,
    .w_clk_pin = DDS_GPIO_W_CLK_PIN,
    .fq_ud_port = DDS_GPIO_PORT,
    .fq_ud_pin = DDS_GPIO_FQ_UD_PIN,
    .data_port = DDS_GPIO_PORT,
    .data_pin = DDS_GPIO_DATA_PIN,
    .reset_port = DDS_GPIO_PORT,
    .reset_pin = DDS_GPIO_RESET_PIN,
    .system_clock_hz = CPUCLK_FREQ,
};

static const ad9850_config_t g_ad9850_config = {
    .io_context = &g_ad9850_platform,
    .write_line = AD9850_MSPM0_WriteLine,
    .delay_us = AD9850_MSPM0_DelayUs,
    .reference_clock_hz = DDS_REFERENCE_CLOCK_HZ,
    .edge_delay_us = 1U,
};

static const signal_robust_peak_to_peak_config_t g_mode3_robust_config = {
    .lower_quantile = MODE3_ROBUST_LOW_QUANTILE,
    .upper_quantile = MODE3_ROBUST_HIGH_QUANTILE,
};

static const signal_vca820_gain_config_t g_vca820_config = {
    .dds_vpp_v = DDS_THEORETICAL_VPP_V,
    .gain_max = VCA820_GAIN_MAX,
    .vctrl0_v = VCA820_VCTRL0_V,
    .vctrl_slope_v = VCA820_VSLOPE_V,
    .control_min_v = VCA820_CTRL_MIN_V,
    .control_max_v = VCA820_CTRL_MAX_V,
    .dac_reference_v = DAC_REFERENCE_V,
    .dac_full_scale_code = DAC_FULL_SCALE_CODE,
};

static const signal_slew_rate_config_t g_mode3_slew_config = {
    .low_ratio = MODE3_EDGE_LOW_RATIO,
    .high_ratio = MODE3_EDGE_HIGH_RATIO,
};

/* ============================ Small helpers ============================= */

/* 函数索引：App_DelayUs 是外设时序延时；App_DDSWakeAndSetFrequency 设置 DDS；
 * VCA820_* 把目标 Vpp 换算成 DAC 码；App_ConvertADCToVoltage 做 ADC 码到电压；
 * App_CaptureWaveformVoltage/ App_MeasureACRMS 是采样与测量；App_RunQuestion1~4
 * 分别执行四个子题；App_HandleKey 处理键盘；App_MeasurementStateText 把测量状态转成文字；
 * App_TFTPrepare/Draw/Refresh 负责静态
 * 页面和局部字段；main 按“初始化->键盘->测量->显示”循环。raw_adc 是原始 ADC 码，
 * voltage 是浮点电压，g_* 全局结果保存当前页面数据，key symbol 是键盘字符。 */
/* 自写辅助：按 CPU 时钟提供微秒延时，给 AD9850 时序使用；delay_us 越大阻塞越久。 */
static void App_DelayUs(uint32_t delay_us)
{
    while (delay_us > 0U) {
        DL_Common_delayCycles(CPUCLK_FREQ / 1000000U);
        --delay_us;
    }
}

/* 模块调用组合：唤醒 AD9850 并写入 frequency_hz；DDS 相位累加和 SPI 时序由模块完成。 */
static void App_DDSWakeAndSetFrequency(uint32_t frequency_hz)
{
    /* 【AD9850 模块】退出掉电后再写频率，底层 GPIO 串行时序由模块完成。 */
    (void)AD9850_SetPowerDown(&g_ad9850, false);
    (void)AD9850_SetFrequencyHz(&g_ad9850, frequency_hz);
}

/* 自写换算：把目标 Vpp 换成 VCA820 控制 DAC 码；修改增益模型应改这里。 */
static uint16_t VCA820_TargetVppToDACCode(float target_vpp)
{
    uint16_t dac_code = 0U;

    /* 【VCA820 Gain Control 模块】根据标定参数把目标 Vpp 换算为控制电压/DAC 码。 */
    (void)SignalVCA820_TargetVppToDACCode(
        target_vpp, &g_vca820_config, &dac_code);
    return dac_code;
}

/* 自写组合：限幅 target_vpp、换算 DAC 码并写入 VCA820；返回实际控制码。 */
static uint16_t VCA820_SetTargetVpp(float target_vpp)
{
    uint16_t dac_code = VCA820_TargetVppToDACCode(target_vpp);

    DL_DAC12_output12(DAC0, dac_code);
    return dac_code;
}

/* 自写小逻辑：按参考电压和满量程把 raw_codes 转为 voltage 数组。 */
static void App_ConvertADCToVoltage(const uint16_t *raw_codes,
    float *voltage_v, uint32_t count, float input_scale,
    float offset_voltage_v)
{
    uint32_t index;
    float volts_per_code = (APP_ADC_REFERENCE_V * input_scale) /
        (float)APP_ADC_MAX_CODE;

    for (index = 0U; index < count; ++index) {
        voltage_v[index] = ((float)raw_codes[index] * volts_per_code) +
            offset_voltage_v;
    }
}

/* 模块调用组合：启动 ADC DMA，等待一帧完成，再将原始码转换到 g_wave_voltage。 */
static void App_CaptureWaveformVoltage(void)
{
    /* 【ADC DMA 模块】设置目标缓冲和点数后启动一帧采集。 */
    (void)SignalADC_Start(
        g_wave_capture.raw, APP_WAVE_SAMPLE_COUNT);

    while (!SignalADC_IsFinished()) {
        __WFI();
    }

    App_ConvertADCToVoltage(g_wave_capture.raw, g_wave_voltage,
        APP_WAVE_SAMPLE_COUNT, 1.0f, 0.0f);
}

/* 模块调用组合：去直流后调用 AC RMS 模块，保存指定采样窗口的结果。 */
static void App_MeasureACRMS(
    uint32_t frame_count, float *ac_rms_v, float *mean_voltage_v)
{
    uint32_t frame_index;
    float ac_rms_sum = 0.0f;
    float mean_sum = 0.0f;

    for (frame_index = 0U; frame_index < frame_count; ++frame_index) {
        float ac_rms_frame_v;
        float mean_frame_v;

        App_CaptureWaveformVoltage();
        /* 【CMSIS-DSP】mean 求 DC，offset 去 DC，rms 求交流有效值；main 只串联三步。 */
        arm_mean_f32(g_wave_voltage, APP_WAVE_SAMPLE_COUNT, &mean_frame_v);
        arm_offset_f32(g_wave_voltage, -mean_frame_v,
            g_wave_capture.robust_workspace, APP_WAVE_SAMPLE_COUNT);
        arm_rms_f32(g_wave_capture.robust_workspace,
            APP_WAVE_SAMPLE_COUNT, &ac_rms_frame_v);
        ac_rms_sum += ac_rms_frame_v;
        mean_sum += mean_frame_v;
    }

    *ac_rms_v = ac_rms_sum / (float)frame_count;
    *mean_voltage_v = mean_sum / (float)frame_count;
}

/* ============================= Q2: UGBW ================================ */

/* 比赛步骤 Q2（自写编排）：扫频/测量运放 UGBW，并更新 g_mode2_* 结果。 */
static void App_RunQuestion2(void)
{
    uint32_t frequency_hz;
    uint32_t previous_frequency_hz;
    float target_ac_rms_v;
    float previous_ac_rms_v;
    float previous_mean_v;

    g_mode2_ugbw_hz = 0.0f;
    g_mode2_reference_ac_rms_v = 0.0f;
    g_mode2_cutoff_ac_rms_v = 0.0f;
    g_mode2_mean_voltage_v = 0.0f;
    g_mode2_above_limit = false;

    /* Mode 3 uses a safer 2 MHz time base, so restore the Q2 rate here. */
    /* 【ADC DMA 模块】Q2 扫频前切到波形测量采样率。 */
    (void)SignalADC_SetSampleRate(APP_WAVE_SAMPLE_RATE_HZ);

    g_vca820_dac_code = VCA820_SetTargetVpp(MODE2_DUT_INPUT_VPP_V);
    App_DDSWakeAndSetFrequency(MODE2_SWEEP_START_HZ);

    App_DelayUs(MODE2_SETTLE_US);
    App_MeasureACRMS(
        MODE2_FRAMES_PER_POINT,
        &g_mode2_reference_ac_rms_v,
        &g_mode2_mean_voltage_v);
    if (g_mode2_reference_ac_rms_v < MODE2_MIN_VALID_AC_RMS_V) {
        g_measurement_state = APP_MEAS_DONE;
        return;
    }

    target_ac_rms_v =
        g_mode2_reference_ac_rms_v * MODE2_MINUS_3DB_RATIO;
    previous_frequency_hz = MODE2_SWEEP_START_HZ;
    previous_ac_rms_v = g_mode2_reference_ac_rms_v;
    previous_mean_v = g_mode2_mean_voltage_v;

    frequency_hz =
        MODE2_SWEEP_START_HZ + MODE2_SWEEP_STEP_HZ;
    while (frequency_hz <= MODE2_SWEEP_END_HZ) {
        float current_ac_rms_v;
        float current_mean_v;

        /* 【AD9850 模块】每个扫频点只更新 DDS 频率。 */
        (void)AD9850_SetFrequencyHz(&g_ad9850, frequency_hz);

        App_DelayUs(MODE2_SETTLE_US);
        App_MeasureACRMS(
            MODE2_FRAMES_PER_POINT,
            &current_ac_rms_v,
            &current_mean_v);

        if ((previous_ac_rms_v > target_ac_rms_v) &&
            (current_ac_rms_v <= target_ac_rms_v)) {
            float denominator = previous_ac_rms_v - current_ac_rms_v;
            float fraction = 0.0f;

            if (denominator > 0.0f) {
                fraction =
                    (previous_ac_rms_v - target_ac_rms_v) / denominator;
            }
            if (fraction < 0.0f) {
                fraction = 0.0f;
            }
            if (fraction > 1.0f) {
                fraction = 1.0f;
            }

            g_mode2_ugbw_hz =
                (float)previous_frequency_hz +
                fraction * (float)(
                    frequency_hz - previous_frequency_hz);
            g_mode2_cutoff_ac_rms_v = target_ac_rms_v;
            g_mode2_mean_voltage_v = current_mean_v;
            g_measurement_state = APP_MEAS_DONE;
            return;
        }

        previous_frequency_hz = frequency_hz;
        previous_ac_rms_v = current_ac_rms_v;
        previous_mean_v = current_mean_v;

        if ((MODE2_SWEEP_END_HZ - frequency_hz) <
            MODE2_SWEEP_STEP_HZ) {
            break;
        }
        frequency_hz += MODE2_SWEEP_STEP_HZ;
    }

    g_mode2_ugbw_hz = (float)MODE2_SWEEP_END_HZ;
    g_mode2_cutoff_ac_rms_v = previous_ac_rms_v;
    g_mode2_mean_voltage_v = previous_mean_v;
    g_mode2_above_limit = true;
    g_measurement_state = APP_MEAS_LIMIT;
}

/* 比赛步骤 Q3（自写编排）：调用 slew-rate 模块测上升/下降斜率和时间。 */
static void App_RunQuestion3(void)
{
    signal_robust_peak_to_peak_result_t levels = {0};
    signal_slew_rate_result_t edge_times = {0};
    uint32_t configured_sample_rate_hz;

    g_mode3_rise_slew_rate_v_per_us = 0.0f;
    g_mode3_fall_slew_rate_v_per_us = 0.0f;
    g_mode3_rise_time_us = 0.0f;
    g_mode3_fall_time_us = 0.0f;
    g_mode3_output_vpp_v = 0.0f;

    /*
     * AD9850 SIN_OUT and SQ_OUT/QOUT are separate physical outputs.
     * Programming the frequency drives both; no software waveform bit exists.
     * The square output must be wired to the Q3 bypass path.
    */
    DL_DAC12_output12(DAC0, 0U);
    App_DDSWakeAndSetFrequency(MODE3_TEST_FREQUENCY_HZ);
    App_DelayUs(MODE3_SETTLE_US);

    /* 32 MHz / 16 = exactly 2 MHz for a reliable ADC/DMA time base. */
    (void)SignalADC_SetSampleRate(MODE3_WAVE_SAMPLE_RATE_HZ);
    configured_sample_rate_hz =
        SignalADC_GetConfiguredTriggerRate();

    App_CaptureWaveformVoltage();

    /* 【Robust Peak-to-Peak 模块】忽略少量异常点，得到比普通 max-min 更稳定的 Vpp。 */
    (void)SignalRobustPeakToPeak_Process(
        g_wave_voltage,
        APP_WAVE_SAMPLE_COUNT,
        &g_mode3_robust_config,
        g_wave_capture.robust_workspace,
        APP_WAVE_SAMPLE_COUNT,
        &levels);

    g_mode3_output_vpp_v =
        levels.robust_vpp_v * MODE3_ADC_TO_DUT_OUTPUT_SCALE;

    if (levels.robust_vpp_v >= MODE3_CONDITIONED_MIN_VPP_V) {
        /* 【Slew Rate 模块】用 10%~90% 区间拟合上升/下降斜率和转换时间。 */
        if (SignalSlewRate_Process(
            g_wave_voltage,
            APP_WAVE_SAMPLE_COUNT,
            levels.lower_voltage_v,
            levels.upper_voltage_v,
            configured_sample_rate_hz,
            &g_mode3_slew_config,
            &edge_times) == SIGNAL_ALGORITHM_OK) {
            g_mode3_rise_time_us = edge_times.rise_time_us;
            g_mode3_fall_time_us = edge_times.fall_time_us;
        }

        if (g_mode3_rise_time_us > 0.0f) {
            g_mode3_rise_slew_rate_v_per_us =
                ((MODE3_EDGE_HIGH_RATIO - MODE3_EDGE_LOW_RATIO) *
                    g_mode3_output_vpp_v) /
                g_mode3_rise_time_us;
        }
        if (g_mode3_fall_time_us > 0.0f) {
            g_mode3_fall_slew_rate_v_per_us =
                ((MODE3_EDGE_HIGH_RATIO - MODE3_EDGE_LOW_RATIO) *
                    g_mode3_output_vpp_v) /
                g_mode3_fall_time_us;
        }
    }

    g_measurement_state = APP_MEAS_DONE;
}

/* ========================= Q4: static power ============================= */

/* 模块调用组合：读取功率题 ADC DMA 数据并换算为电压数组。 */
static void App_ReadPowerADCBlock(void)
{
    uint32_t sample_index;

    for (sample_index = 0U;
         sample_index < APP_POWER_SAMPLE_COUNT;
         ++sample_index) {
        DL_ADC12_clearInterruptStatus(
            POWER_ADC_INST,
            DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
        DL_ADC12_startConversion(POWER_ADC_INST);

        while (DL_ADC12_getRawInterruptStatus(
                   POWER_ADC_INST,
                   DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED) == 0U) {
        }

        g_power_raw[sample_index] = DL_ADC12_getMemResult(
            POWER_ADC_INST, POWER_ADC_ADCMEM_0);

        /* TI single-conversion example re-arms ENC before the next sample. */
        DL_ADC12_enableConversions(POWER_ADC_INST);
    }
}

/* 比赛步骤 Q4（自写编排）：根据分流电阻电压计算电流和功率。 */
static void App_RunQuestion4(void)
{
    g_mode4_shunt_voltage_v = 0.0f;
    g_mode4_current_ma = 0.0f;
    g_mode4_power_mw = 0.0f;

    (void)AD9850_SetPowerDown(&g_ad9850, true);

    DL_DAC12_output12(DAC0, 0U);
    App_DelayUs(MODE4_DDS_POWERDOWN_SETTLE_US);

    App_ReadPowerADCBlock();
    App_ConvertADCToVoltage(g_power_raw, g_power_voltage,
        APP_POWER_SAMPLE_COUNT, MODE4_ADC_INPUT_SCALE, MODE4_ADC_OFFSET_V);
    arm_mean_f32(g_power_voltage, APP_POWER_SAMPLE_COUNT,
        &g_mode4_shunt_voltage_v);
    /* 【Static Power 模块】根据分流电压、电阻和电源电压计算电流/功耗。 */
    (void)SignalStaticPower_Calculate(
        g_mode4_shunt_voltage_v,
        MODE4_SHUNT_RESISTANCE_OHM,
        MODE4_SUPPLY_VOLTAGE_V,
        MODE4_RAIL_COUNT_FACTOR,
        &g_mode4_current_ma,
        &g_mode4_power_mw);
    g_measurement_state = APP_MEAS_DONE;
}

/* ============================== Keypad ================================= */

/* 自写状态机：清空数字预输入缓冲，退出当前输入状态。 */
static void App_ClearKeypadEntry(void)
{
    g_keypad_input_value = 0U;
    g_keypad_input_digits = 0U;
    g_keypad_input_active = false;
}

/* 自写状态机：把键盘输入的频率/Vpp 校验、限幅后应用到 DDS/VCA。 */
static void App_ApplyQuestion1Entry(void)
{
    if (!g_keypad_input_active ||
        (g_keypad_input_digits == 0U)) {
        return;
    }

    if (g_q1_input_field == APP_Q1_INPUT_FREQUENCY) {
        if ((g_keypad_input_value >= APP_FREQUENCY_MIN_HZ) &&
            (g_keypad_input_value <= APP_FREQUENCY_MAX_HZ)) {
            App_DDSWakeAndSetFrequency(g_keypad_input_value);
            g_target_frequency_hz = g_keypad_input_value;
        }
    } else {
        if ((g_keypad_input_value >= APP_OUTPUT_VPP_MIN_CV) &&
            (g_keypad_input_value <= APP_OUTPUT_VPP_MAX_CV)) {
            g_target_output_vpp =
                (float)g_keypad_input_value / 100.0f;
            g_vca820_dac_code =
                VCA820_SetTargetVpp(g_target_output_vpp);
        }
    }

    App_ClearKeypadEntry();
    g_q1_value_dirty = true;
    g_q1_input_dirty = true;
}

/* 自写页面逻辑：切换当前题目 mode，清理输入并置位整页/局部刷新标志。 */
static void App_SelectMode(app_mode_t mode)
{
    g_app_mode = mode;
    g_q1_input_field = APP_Q1_INPUT_FREQUENCY;
    App_ClearKeypadEntry();

    if (mode == APP_MODE_QUESTION_1) {
        App_DDSWakeAndSetFrequency(g_target_frequency_hz);
        g_vca820_dac_code =
            VCA820_SetTargetVpp(g_target_output_vpp);
        g_measurement_state = APP_MEAS_DONE;
        g_measurement_requested = false;
    } else {
        g_measurement_state = APP_MEAS_READY;
        g_measurement_requested = true;
    }

    g_tft_dirty = true;
    g_q1_input_dirty = false;
    g_q1_value_dirty = false;
}

/* 自写键盘状态机：数字键累积输入，# 确认，A/B/C/D 切换题目或触发测量。 */
static void App_HandleKey(char symbol)
{
    if ((symbol >= 'A') && (symbol <= 'D')) {
        App_SelectMode((app_mode_t)(symbol - 'A'));
        return;
    }

    if (g_app_mode != APP_MODE_QUESTION_1) {
        if (symbol == '#') {
            g_measurement_state = APP_MEAS_READY;
            g_measurement_requested = true;
        }
        return;
    }

    if (symbol == '*') {
        g_q1_input_field =
            (g_q1_input_field == APP_Q1_INPUT_FREQUENCY)
                ? APP_Q1_INPUT_VPP
                : APP_Q1_INPUT_FREQUENCY;
        App_ClearKeypadEntry();
        g_q1_input_dirty = true;
        return;
    }

    if (symbol == '#') {
        App_ApplyQuestion1Entry();
        g_q1_input_dirty = true;
        return;
    }

    if ((symbol >= '0') && (symbol <= '9')) {
        uint32_t digit = (uint32_t)(symbol - '0');

        if (!g_keypad_input_active) {
            App_ClearKeypadEntry();
            g_keypad_input_active = true;
        }
        if (g_keypad_input_digits <
            APP_KEYPAD_INPUT_MAX_DIGITS) {
            g_keypad_input_value =
                g_keypad_input_value * 10U + digit;
            ++g_keypad_input_digits;
            g_q1_input_dirty = true;
        }
    }
}

/* ================================ TFT ================================== */

/* 自写显示辅助：把测量状态枚举转换为屏幕提示字符串。 */
static const char *App_MeasurementStateText(void)
{
    switch (g_measurement_state) {
        case APP_MEAS_READY:
            return "READY";
        case APP_MEAS_RUNNING:
            return "RUN";
        case APP_MEAS_DONE:
            return "DONE";
        case APP_MEAS_LIMIT:
            return "LIMIT";
        default:
            return "DONE";
    }
}

/* 自写局部刷新：清除 y 行字段并绘制固定 label；只在该行变化时使用。 */
static void App_TFTPrepareRow(int16_t y, const char *label)
{
    (void)TFT_ST7789_FillRect(
        &g_tft, 8, y, 304, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawString(
        &g_tft, 8, y, label,
        TFT_ST7789_FONT_8X16,
        TFT_ST7789_WHITE, TFT_ST7789_BLACK,
        false, false);
}

/* 自写包装：在一行显示文字 value；ST7789 字库调用来自模块 README。 */
static void App_TFTDrawTextRow(
    int16_t y,
    const char *label,
    const char *text,
    uint16_t color)
{
    App_TFTPrepareRow(y, label);
    (void)TFT_ST7789_DrawString(
        &g_tft, 104, y, text,
        TFT_ST7789_FONT_8X16,
        color, TFT_ST7789_BLACK,
        false, false);
}

/* 自写包装：在一行显示整数 value，digits 控制显示宽度。 */
static void App_TFTDrawIntRow(
    int16_t y,
    const char *label,
    int32_t value,
    const char *unit,
    uint16_t color)
{
    App_TFTPrepareRow(y, label);
    (void)TFT_ST7789_DrawInt32(
        &g_tft, 104, y, value,
        TFT_ST7789_FONT_8X16,
        color, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawString(
        &g_tft, 240, y, unit,
        TFT_ST7789_FONT_8X16,
        TFT_ST7789_WHITE, TFT_ST7789_BLACK,
        false, false);
}

/* 自写包装：在一行显示浮点 value 和 decimals 位小数。 */
static void App_TFTDrawFloatRow(
    int16_t y,
    const char *label,
    float value,
    uint8_t precision,
    const char *unit,
    uint16_t color)
{
    App_TFTPrepareRow(y, label);
    (void)TFT_ST7789_DrawFloat(
        &g_tft, 104, y, value, precision,
        TFT_ST7789_FONT_8X16,
        color, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawString(
        &g_tft, 240, y, unit,
        TFT_ST7789_FONT_8X16,
        TFT_ST7789_WHITE, TFT_ST7789_BLACK,
        false, false);
}

/* 自写显示：绘制边框、标题、固定标签；动态刷新不重画这些内容。 */
static void App_TFTDrawStaticLayout(void)
{
    (void)TFT_ST7789_DrawString(
        &g_tft, 8, 8, "2024A OPAMP TESTER",
        TFT_ST7789_FONT_8X16,
        TFT_ST7789_CYAN, TFT_ST7789_BLACK,
        false, false);
}

/* 自写显示：绘制 Q1 波形发生和输入提示页面。 */
static void App_TFTDrawQuestion1(void)
{
    const char *edit_text =
        (g_q1_input_field == APP_Q1_INPUT_FREQUENCY)
            ? "FREQ" : "VPP";

    App_TFTDrawIntRow(
        40, "MODE:", 1, "", TFT_ST7789_YELLOW);
    App_TFTDrawIntRow(
        72, "FREQ:", (int32_t)g_target_frequency_hz,
        "Hz", TFT_ST7789_GREEN);
    App_TFTDrawFloatRow(
        104, "VPP:", g_target_output_vpp,
        2U, "V", TFT_ST7789_GREEN);
    App_TFTDrawTextRow(
        136, "EDIT:", edit_text, TFT_ST7789_YELLOW);
    if (g_keypad_input_active) {
        App_TFTDrawIntRow(
            168, "INPUT:", (int32_t)g_keypad_input_value,
            "", TFT_ST7789_WHITE);
    } else {
        App_TFTDrawTextRow(
            168, "INPUT:", "-", TFT_ST7789_WHITE);
    }
    App_TFTDrawTextRow(
        200, "STATUS:", "OK", TFT_ST7789_GREEN);
}

/* 自写显示：绘制 Q2 UGBW 测量结果页面。 */
static void App_TFTDrawQuestion2(void)
{
    App_TFTDrawIntRow(
        40, "MODE:", 2, "", TFT_ST7789_YELLOW);
    if (g_mode2_above_limit) {
        App_TFTDrawTextRow(
            72, "UGBW:", ">=2000000 Hz", TFT_ST7789_GREEN);
    } else {
        App_TFTDrawIntRow(
            72, "UGBW:", (int32_t)(g_mode2_ugbw_hz + 0.5f),
            "Hz", TFT_ST7789_GREEN);
    }
    App_TFTDrawFloatRow(
        104, "REF RMS:", g_mode2_reference_ac_rms_v,
        4U, "V", TFT_ST7789_WHITE);
    App_TFTDrawFloatRow(
        136, "-3DB RMS:", g_mode2_cutoff_ac_rms_v,
        4U, "V", TFT_ST7789_WHITE);
    App_TFTDrawFloatRow(
        168, "DC:", g_mode2_mean_voltage_v,
        3U, "V", TFT_ST7789_WHITE);
    App_TFTDrawTextRow(
        200, "STATUS:", App_MeasurementStateText(),
        TFT_ST7789_GREEN);
}

/* 自写显示：绘制 Q3 slew-rate 测量结果页面。 */
static void App_TFTDrawQuestion3(void)
{
    App_TFTDrawIntRow(
        40, "MODE:", 3, "", TFT_ST7789_YELLOW);
    App_TFTDrawFloatRow(
        72, "RISE SR:", g_mode3_rise_slew_rate_v_per_us,
        3U, "V/us", TFT_ST7789_GREEN);
    App_TFTDrawFloatRow(
        104, "FALL SR:", g_mode3_fall_slew_rate_v_per_us,
        3U, "V/us", TFT_ST7789_GREEN);
    App_TFTDrawFloatRow(
        136, "RISE T:", g_mode3_rise_time_us,
        3U, "us", TFT_ST7789_WHITE);
    App_TFTDrawFloatRow(
        168, "FALL T:", g_mode3_fall_time_us,
        3U, "us", TFT_ST7789_WHITE);
    App_TFTDrawFloatRow(
        200, "OUT VPP:", g_mode3_output_vpp_v,
        3U, "V", TFT_ST7789_WHITE);
}

/* 自写显示：绘制 Q4 功率测量结果页面。 */
static void App_TFTDrawQuestion4(void)
{
    App_TFTDrawIntRow(
        40, "MODE:", 4, "", TFT_ST7789_YELLOW);
    App_TFTDrawFloatRow(
        72, "SHUNT V:", g_mode4_shunt_voltage_v,
        4U, "V", TFT_ST7789_GREEN);
    App_TFTDrawFloatRow(
        104, "CURRENT:", g_mode4_current_ma,
        3U, "mA", TFT_ST7789_WHITE);
    App_TFTDrawFloatRow(
        136, "POWER:", g_mode4_power_mw,
        2U, "mW", TFT_ST7789_GREEN);
    App_TFTDrawFloatRow(
        168, "R SHUNT:", MODE4_SHUNT_RESISTANCE_OHM,
        1U, "ohm", TFT_ST7789_WHITE);
    App_TFTDrawTextRow(
        200, "STATUS:", App_MeasurementStateText(),
        TFT_ST7789_GREEN);
}

/* 自写显示：按当前 mode 刷新测量数字，不清空整屏。 */
static void App_TFTRefreshValues(void)
{
    if (!g_tft_dirty) {
        return;
    }

    switch (g_app_mode) {
        case APP_MODE_QUESTION_1:
            App_TFTDrawQuestion1();
            break;
        case APP_MODE_QUESTION_2:
            App_TFTDrawQuestion2();
            break;
        case APP_MODE_QUESTION_3:
            App_TFTDrawQuestion3();
            break;
        case APP_MODE_QUESTION_4:
        default:
            App_TFTDrawQuestion4();
            break;
    }

    g_tft_dirty = false;
}

/* 自写显示：仅刷新 Q1 被修改的频率、Vpp 或输入串。 */
static void App_TFTRefreshQuestion1Changed(void)
{
    const char *edit_text;

    if (g_app_mode != APP_MODE_QUESTION_1) {
        g_q1_input_dirty = false;
        g_q1_value_dirty = false;
        return;
    }
    edit_text = (g_q1_input_field == APP_Q1_INPUT_FREQUENCY) ?
        "FREQ" : "VPP";
    if (g_q1_value_dirty) {
        (void)TFT_ST7789_FillRect(&g_tft, 104, 72, 128, 16,
            TFT_ST7789_BLACK);
        (void)TFT_ST7789_DrawInt32(&g_tft, 104, 72,
            (int32_t)g_target_frequency_hz, TFT_ST7789_FONT_8X16,
            TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
        (void)TFT_ST7789_FillRect(&g_tft, 104, 104, 128, 16,
            TFT_ST7789_BLACK);
        (void)TFT_ST7789_DrawFloat(&g_tft, 104, 104,
            g_target_output_vpp, 2U, TFT_ST7789_FONT_8X16,
            TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
        g_q1_value_dirty = false;
    }
    if (g_q1_input_dirty) {
        (void)TFT_ST7789_FillRect(&g_tft, 104, 136, 64, 16,
            TFT_ST7789_BLACK);
        (void)TFT_ST7789_DrawString(&g_tft, 104, 136, edit_text,
            TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW,
            TFT_ST7789_BLACK, false, false);
        (void)TFT_ST7789_FillRect(&g_tft, 104, 168, 136, 16,
            TFT_ST7789_BLACK);
        if (g_keypad_input_active) {
            (void)TFT_ST7789_DrawInt32(&g_tft, 104, 168,
                (int32_t)g_keypad_input_value, TFT_ST7789_FONT_8X16,
                TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
        } else {
            (void)TFT_ST7789_DrawString(&g_tft, 104, 168, "-",
                TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE,
                TFT_ST7789_BLACK, false, false);
        }
        g_q1_input_dirty = false;
    }
}

/* ================================ main ================================= */

/* main：SYSCFG_DL_init 后初始化 DDS、DAC DMA、ADC、键盘和 ST7789；随后按当前模式
 * 执行一次测量并刷新页面。键盘中断只产生 symbol，主循环通过 App_HandleKey 改参数
 * 或切换题目；dirty 标志决定是否刷新局部字段，避免每次采样整屏重画。 */
int main(void)
{
    const signal_adc_dma_config_t adc_config = {
        .sample_rate_hz = APP_WAVE_SAMPLE_RATE_HZ,
        .timer_clock_hz = CPUCLK_FREQ,
        .timer_max_count = APP_TIMER_MAX_COUNT,
    };

    SYSCFG_DL_init();
    /* 【ADC DMA 模块】初始化采样定时器、ADC 和 DMA 完成中断。 */
    (void)SignalADC_Init(&adc_config);
    /* 【AD9850 模块】把 MSPM0 GPIO 平台回调和 DDS 配置绑定到 g_ad9850。 */
    (void)AD9850_Init(&g_ad9850, &g_ad9850_config);

    g_vca820_dac_code =
        VCA820_SetTargetVpp(g_target_output_vpp);
    App_DDSWakeAndSetFrequency(g_target_frequency_hz);
    /* 【ST7789 MSPM0 模块】绑定 SysConfig SPI/CS/DC/RST 并设置横屏方向。 */
    (void)SignalTFTST7789_MSPM0_Init(
        &g_tft, TFT_ST7789_ROTATION_270, 0U, 0U);
    (void)TFT_ST7789_FillScreen(&g_tft, TFT_ST7789_BLACK);
    App_TFTDrawStaticLayout();
    App_TFTRefreshValues();

    for (;;) {
        char symbol;

        /* 【矩阵键盘模块】内部完成固定 GPIO 扫描、消抖和鬼键过滤。 */
        if (SignalMatrixKeypad4x4_ReadNewSymbol(&symbol) ==
            SIGNAL_RESULT_OK) {
            App_HandleKey(symbol);
        }

        App_TFTRefreshQuestion1Changed();

        if (g_measurement_requested) {
            g_measurement_requested = false;
            g_measurement_state = APP_MEAS_RUNNING;
            g_tft_dirty = true;
            App_TFTRefreshValues();

            switch (g_app_mode) {
                case APP_MODE_QUESTION_2:
                    App_RunQuestion2();
                    break;
                case APP_MODE_QUESTION_3:
                    App_RunQuestion3();
                    break;
                case APP_MODE_QUESTION_4:
                    App_RunQuestion4();
                    break;
                case APP_MODE_QUESTION_1:
                default:
                    break;
            }
            g_tft_dirty = true;
        }

        App_TFTRefreshValues();

        /* 5 ms scan period; three equal scans give about 15 ms debounce. */
        DL_Common_delayCycles(CPUCLK_FREQ / 200U);
    }
}
