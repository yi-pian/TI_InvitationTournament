/* ============================================================
 * 工程：05_bode_sweep_analyzer
 * 用途：主动输出正弦的幅频/相频扫频仪。
 * 接线：DAC0→被测网络输入；CH1 测输入，CH2 测输出。
 * 页面：GAIN、PHASE、CURRENT。
 * KEY MAP：A/B 页面；C 选择 Start/Stop/Points/Vpp；
 * 星号/井号减小/增大当前参数；D START/STOP，HOLD 时 D RESTART。
 *
 * 来源：90、04、30、40、70、80；参考 example02/05/07。
 * 平台闭包来自已验证 example02。经授权复制 modules/.syscfg，模块未修改。
 * ============================================================ */
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "ti_msp_dl_config.h"
#include "signal_dac_dma_mspm0g3507.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_dual_adc_phase.h"
#include "signal_frequency_sweep.h"
#include "signal_matrix_keypad_4x4.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_font.h"
#include "signal_tft_st7789_mspm0g3507.h"
#include "signal_wave_output_mspm0g3507.h"

#define SAMPLE_COUNT            (512U)
#define MAX_SWEEP_POINTS        (32U)
#define SAMPLE_RATE_REQUEST_HZ  (100000U)
#define DAC_UPDATE_RATE_HZ      (100000U)
#define ADC_REFERENCE_V         (3.3f)
#define DDS_TABLE_COUNT         (256U)
#define DDS_OUTPUT_COUNT        (2048U)
#define GRAPH_X                 (8)
#define GRAPH_Y                 (64)
#define GRAPH_W                 (304)
#define GRAPH_H                 (136)
#define KEYPAD_SCAN_MS          (5U)
#define KEY_QUEUE_SIZE          (8U)

typedef enum { PAGE_GAIN = 0U, PAGE_PHASE, PAGE_CURRENT,
    PAGE_COUNT } app_page_t;
typedef enum { PARAM_START = 0U, PARAM_STOP, PARAM_POINTS,
    PARAM_VPP, PARAM_COUNT } sweep_parameter_t;
typedef enum { SWEEP_STOPPED = 0U, SWEEP_RUNNING, SWEEP_HOLD } sweep_state_t;

static uint16_t adc_ch1_samples[SAMPLE_COUNT];
static uint16_t adc_ch2_samples[SAMPLE_COUNT];
static uint16_t dds_wave_table[DDS_TABLE_COUNT];
static uint16_t dds_output_buffer[DDS_OUTPUT_COUNT];
static float sweep_frequency_hz[MAX_SWEEP_POINTS];
static float sweep_gain_db[MAX_SWEEP_POINTS];
static float sweep_phase_deg[MAX_SWEEP_POINTS];
static float sample_rate_hz = (float)SAMPLE_RATE_REQUEST_HZ;
static float start_frequency_hz = 200.0f;
static float stop_frequency_hz = 10000.0f;
static uint32_t sweep_points = 16U;
static float target_vpp_v = 1.0f;
static float gain_ratio, gain_db, phase_deg;
static uint32_t sweep_index;
static app_page_t current_page = PAGE_GAIN;
static app_page_t displayed_page = PAGE_COUNT;
static sweep_parameter_t selected_parameter = PARAM_START;
static sweep_state_t sweep_state = SWEEP_STOPPED;
static tft_st7789_t tft;
static volatile char key_queue[KEY_QUEUE_SIZE];
static volatile uint8_t key_queue_head;
static volatile uint8_t key_queue_tail;
static bool display_dirty = true;

static void DrawText(int32_t x, int32_t y, const char *text, uint16_t color)
{
    (void)TFT_ST7789_DrawString(&tft, x, y, text, TFT_ST7789_FONT_8X16,
        color, TFT_ST7789_BLACK, false, false);
}

/* ============================================================
 * [函数] GenerateSweepTable
 * [功能] 按当前 start/stop/points 生成扫频频点。
 * [来源] [FUYONG_COPY] example02 的 SignalFrequencySweep_Generate 调用。
 * [输入] 三个扫频参数；[输出] sweep_frequency_hz。
 * [单位] Hz；[全局] 频点数组。
 * [步骤] 构造 config→模块生成；[原因] 不在 main 重写线性/对数步进。
 * [单帧唯一] 每次 START/RESTART 一次；[复用] RunSweepPoint。
 * [差异] 运行时点数可调；[依赖] signal_frequency_sweep。
 * ============================================================ */
static bool GenerateSweepTable(void)
{
    const signal_frequency_sweep_config_t config = {
        start_frequency_hz, stop_frequency_hz, sweep_points, false
    };
    return SignalFrequencySweep_Generate(&config, sweep_frequency_hz,
        MAX_SWEEP_POINTS) == SIGNAL_RESULT_OK;
}

/* ============================================================
 * [函数] AcquireDualADCFrame
 * [功能] 扫频点稳定后获取同步 CH1 输入/CH2 输出帧。
 * [来源] [FUYONG_COPY] 04_dual_adc_dma。
 * [输入] 模拟通道；[输出] 两路 ADC code；[单位] code。
 * [全局] 数组/Fs；[步骤] Start→双 DMA 完成→实际 Fs。
 * [原因] Gain/Phase 必须属于同一频点同一帧。
 * [单帧唯一] 每频点一次；[复用] Basic/Phase。
 * [差异] 统一命名；[依赖] dual_adc。
 * ============================================================ */
static bool AcquireDualADCFrame(void)
{
    if (SignalDualADC_Start(adc_ch1_samples, adc_ch2_samples,
            SAMPLE_COUNT) != SIGNAL_RESULT_OK) return false;
    while (!SignalDualADC_IsFinished()) __WFI();
    sample_rate_hz = (float)SignalDualADC_GetConfiguredRate();
    return sample_rate_hz > 0.0f;
}

/* ============================================================
 * [函数] MeasureVpp
 * [功能] 参数化计算任一路 ADC code 峰峰值。
 * [来源] [FUYONG_ADAPTED] 30_basic_measurement 的 min/max/Vpp 步骤。
 * [输入] samples/count；[输出] Vpp；[单位] code→V。
 * [全局] 无；[步骤] 单遍 min/max→乘 V/code。
 * [原因] Gain 比值不需要额外 float 数组，节省扫频工程 RAM。
 * [单帧唯一] 每通道一次；[复用] Gain；[差异] 直接在 code 域找界限。
 * [依赖] 无。
 * ============================================================ */
static float MeasureVpp(const uint16_t *samples, uint32_t count)
{
    uint32_t index;
    uint16_t minimum = samples[0], maximum = samples[0];
    for (index = 1U; index < count; ++index) {
        if (samples[index] < minimum) minimum = samples[index];
        if (samples[index] > maximum) maximum = samples[index];
    }
    return (float)(maximum - minimum) * ADC_REFERENCE_V / 4095.0f;
}

/* ============================================================
 * [函数] RunSweepPoint
 * [功能] 完成一个频点的“输出→采集→Gain→Phase→保存”。
 * [来源] [FUYONG_ADAPTED] example02/App_RunSweepPoint，内部调用 90/04/40。
 * [输入] sweep_frequency[index]；[输出] gain/phase 数组和 current 值。
 * [单位] Hz/V/ratio/dB/degree；[全局] 扫频状态与数组。
 * [步骤] Set sine→短暂 settle→同步 ADC→两路 Vpp→phase 模块→store。
 * [原因] 每点只采一帧，不混用不同频率的数据。
 * [单帧唯一] 每频点一次；[复用] 三页面。
 * [差异] 保存真实 dB/degree，不做 0~1 显示归一化。
 * [依赖] Wave Output、dual_adc_phase。
 * ============================================================ */
static bool RunSweepPoint(void)
{
    const signal_dual_adc_phase_config_t phase_config = {
        16U, 64U, 1U, 16U, 64U
    };
    signal_dual_adc_phase_result_t phase_result;
    float ch1_vpp, ch2_vpp;
    if (sweep_index >= sweep_points) return false;
    if (SignalWaveOutput_SineWithOffset(sweep_frequency_hz[sweep_index],
            target_vpp_v, 1.65f) != SIGNAL_RESULT_OK) return false;
    DL_Common_delayCycles(CPUCLK_FREQ / 200U);
    if (!AcquireDualADCFrame()) return false;
    ch1_vpp = MeasureVpp(adc_ch1_samples, SAMPLE_COUNT);
    ch2_vpp = MeasureVpp(adc_ch2_samples, SAMPLE_COUNT);
    gain_ratio = ch1_vpp > 0.000001f ? ch2_vpp / ch1_vpp : 0.0f;
    gain_db = gain_ratio > 0.0f ? 20.0f * log10f(gain_ratio) : -120.0f;
    phase_deg = 0.0f;
    if (SignalDualADCPhase_Process(adc_ch1_samples, adc_ch2_samples,
            SAMPLE_COUNT, (uint32_t)sample_rate_hz, &phase_config,
            &phase_result) == SIGNAL_ALGORITHM_OK && phase_result.valid != 0U)
        phase_deg = (float)phase_result.phase_degrees;
    sweep_gain_db[sweep_index] = gain_db;
    sweep_phase_deg[sweep_index] = phase_deg;
    ++sweep_index;
    display_dirty = true;
    if (sweep_index >= sweep_points) sweep_state = SWEEP_HOLD;
    return true;
}

/* [READY_PROJECT_LOCAL] 限幅确保 start<stop、输出不越 DAC、点数不越静态数组。 */
static void ClampParameters(void)
{
    if (start_frequency_hz < 100.0f) start_frequency_hz = 100.0f;
    if (start_frequency_hz > 19000.0f) start_frequency_hz = 19000.0f;
    if (stop_frequency_hz < start_frequency_hz + 100.0f)
        stop_frequency_hz = start_frequency_hz + 100.0f;
    if (stop_frequency_hz > 20000.0f) stop_frequency_hz = 20000.0f;
    if (sweep_points < 4U) sweep_points = 4U;
    if (sweep_points > MAX_SWEEP_POINTS) sweep_points = MAX_SWEEP_POINTS;
    if (target_vpp_v < 0.1f) target_vpp_v = 0.1f;
    if (target_vpp_v > 3.0f) target_vpp_v = 3.0f;
}

/* [READY_PROJECT_LOCAL] 统一参数增减；运行中改变参数会 STOP，防止数组混入两套配置。 */
static void AdjustParameter(bool increase)
{
    const float sign = increase ? 1.0f : -1.0f;
    if (selected_parameter == PARAM_START) start_frequency_hz += sign * 100.0f;
    else if (selected_parameter == PARAM_STOP) stop_frequency_hz += sign * 500.0f;
    else if (selected_parameter == PARAM_POINTS) {
        if (increase && sweep_points < MAX_SWEEP_POINTS) sweep_points += 4U;
        if (!increase && sweep_points > 4U) sweep_points -= 4U;
    } else target_vpp_v += sign * 0.1f;
    ClampParameters();
    sweep_state = SWEEP_STOPPED; display_dirty = true;
}

/* [READY_PROJECT_LOCAL] 页面状态、参数选择和 START/STOP 的最小状态机。 */
static void HandleKeypad(void)
{
    char key;
    while (key_queue_tail != key_queue_head) {
        key = key_queue[key_queue_tail];
        key_queue_tail = (uint8_t)((key_queue_tail + 1U) % KEY_QUEUE_SIZE);
        if (key == 'A') current_page = current_page == PAGE_GAIN ?
            PAGE_CURRENT : (app_page_t)(current_page - 1U);
        else if (key == 'B') current_page =
            (app_page_t)((current_page + 1U) % PAGE_COUNT);
        else if (key == 'C') selected_parameter =
            (sweep_parameter_t)((selected_parameter + 1U) % PARAM_COUNT);
        else if (key == '*') AdjustParameter(false);
        else if (key == '#') AdjustParameter(true);
        else if (key == 'D') {
            if (sweep_state == SWEEP_RUNNING) sweep_state = SWEEP_STOPPED;
            else {
                ClampParameters();
                if (GenerateSweepTable()) {
                    sweep_index = 0U;
                    sweep_state = SWEEP_RUNNING;
                }
            }
        }
        display_dirty = true;
    }
}

/* ============================================================
 * [函数] DrawCurve
 * [功能] 参数化绘制已完成点的 Gain 或 Phase 曲线，Y 自动量程。
 * [来源] [READY_PROJECT_LOCAL]，参考 example02/App_DrawPoint。
 * [输入] values/count/color；[输出] TFT 曲线；[单位] dB 或 degree。
 * [全局] tft；[步骤] 求 min/max→屏宽映射→折线。
 * [原因] fuyong 无完整双页自动 Y 曲线；静态数组避免栈大对象。
 * [单帧唯一] 当前曲线页刷新一次；[复用] Gain/Phase 共用。
 * [差异] 参数化 Y 数据；[依赖] TFT。
 * ============================================================ */
static void DrawCurve(const float *values, uint32_t count, uint16_t color)
{
    uint32_t index;
    float minimum, maximum;
    if (count == 0U) return;
    minimum = values[0]; maximum = values[0];
    for (index = 1U; index < count; ++index) {
        if (values[index] < minimum) minimum = values[index];
        if (values[index] > maximum) maximum = values[index];
    }
    if (maximum - minimum < 0.1f) maximum = minimum + 0.1f;
    for (index = 1U; index < count; ++index) {
        const int32_t x0 = GRAPH_X + 1 + (int32_t)((index - 1U) * (GRAPH_W - 3U) / (sweep_points - 1U));
        const int32_t x1 = GRAPH_X + 1 + (int32_t)(index * (GRAPH_W - 3U) / (sweep_points - 1U));
        const int32_t y0 = GRAPH_Y + GRAPH_H - 2 - (int32_t)((values[index - 1U] - minimum) * (GRAPH_H - 3) / (maximum - minimum));
        const int32_t y1 = GRAPH_Y + GRAPH_H - 2 - (int32_t)((values[index] - minimum) * (GRAPH_H - 3) / (maximum - minimum));
        (void)TFT_ST7789_DrawLine(&tft, x0, y0, x1, y1, color);
    }
    (void)TFT_ST7789_DrawFloat(&tft, 8, 48, maximum, 1U, TFT_ST7789_FONT_8X16, color, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 248, 200, minimum, 1U, TFT_ST7789_FONT_8X16, color, TFT_ST7789_BLACK, false);
}

static const char *StateName(void)
{
    if (sweep_state == SWEEP_STOPPED) return "STOP";
    if (sweep_state == SWEEP_RUNNING) return "RUN";
    return "HOLD";
}

/* ============================================================
 * [函数] UpdateDisplay
 * [功能] 绘制 GAIN/PHASE/CURRENT 三页面及可调参数。
 * [来源] [FUYONG_ADAPTED] 80/DrawPage；曲线/状态为 LOCAL。
 * [输入] 扫频数组/状态；[输出] TFT；[单位] Hz/dB/deg/V。
 * [全局] page/state/parameters；[步骤] 翻页画静态页→局部更新状态/曲线/数值。
 * [原因] 每个扫频点完成后刷新，数据和显示点严格对应。
 * [单帧唯一] 每点一次；[复用] main。
 * [差异] 三页与参数状态机；[依赖] TFT/font。
 * ============================================================ */
static void DrawStaticUi(void)
{
    (void)TFT_ST7789_FillScreen(&tft, TFT_ST7789_BLACK);
    DrawText(8, 4, current_page == PAGE_GAIN ? "BODE - GAIN" :
        (current_page == PAGE_PHASE ? "BODE - PHASE" : "BODE - CURRENT"),
        TFT_ST7789_CYAN);
    DrawText(8, 28, "Start/Stop/Pts/Vpp:", TFT_ST7789_WHITE);
    if (current_page == PAGE_GAIN || current_page == PAGE_PHASE) {
        (void)TFT_ST7789_DrawRect(&tft, GRAPH_X, GRAPH_Y, GRAPH_W,
            GRAPH_H, TFT_ST7789_BLUE);
    } else {
        DrawText(8, 68, "Frequency:", TFT_ST7789_WHITE);
        DrawText(8, 100, "Gain ratio/dB:", TFT_ST7789_WHITE);
        DrawText(8, 132, "Phase(deg):", TFT_ST7789_WHITE);
        DrawText(8, 164, "Point:", TFT_ST7789_WHITE);
    }
    displayed_page = current_page;
}

static void UpdateDisplay(void)
{
    if (displayed_page != current_page) DrawStaticUi();
    (void)TFT_ST7789_FillRect(&tft, 232, 4, 80, 16, TFT_ST7789_BLACK);
    DrawText(232, 4, StateName(), sweep_state == SWEEP_RUNNING ?
        TFT_ST7789_GREEN : TFT_ST7789_YELLOW);
    if (current_page == PAGE_GAIN || current_page == PAGE_PHASE) {
        (void)TFT_ST7789_FillRect(&tft, 0, 48, 96, 16, TFT_ST7789_BLACK);
        (void)TFT_ST7789_FillRect(&tft, GRAPH_X + 1, GRAPH_Y + 1,
            GRAPH_W - 2, GRAPH_H - 2, TFT_ST7789_BLACK);
        (void)TFT_ST7789_FillRect(&tft, 248, 200, 64, 16, TFT_ST7789_BLACK);
        DrawCurve(current_page == PAGE_GAIN ? sweep_gain_db : sweep_phase_deg,
            sweep_index, current_page == PAGE_GAIN ? TFT_ST7789_YELLOW : TFT_ST7789_CYAN);
    } else {
        (void)TFT_ST7789_FillRect(&tft, 136, 68, 176, 16, TFT_ST7789_BLACK);
        (void)TFT_ST7789_DrawFloat(&tft, 136, 68, sweep_index == 0U ? start_frequency_hz : sweep_frequency_hz[sweep_index - 1U], 1U, TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
        (void)TFT_ST7789_FillRect(&tft, 152, 100, 160, 16, TFT_ST7789_BLACK);
        (void)TFT_ST7789_DrawFloat(&tft, 152, 100, gain_ratio, 4U, TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
        (void)TFT_ST7789_DrawFloat(&tft, 240, 100, gain_db, 2U, TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK, false);
        (void)TFT_ST7789_FillRect(&tft, 152, 132, 160, 16, TFT_ST7789_BLACK);
        (void)TFT_ST7789_DrawFloat(&tft, 152, 132, phase_deg, 1U, TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN, TFT_ST7789_BLACK, false);
        (void)TFT_ST7789_FillRect(&tft, 152, 164, 160, 16, TFT_ST7789_BLACK);
        (void)TFT_ST7789_DrawInt32(&tft, 152, 164, (int32_t)sweep_index, TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    }
    (void)TFT_ST7789_FillRect(&tft, 0, 216, 320, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawFloat(&tft, 8, 216, start_frequency_hz, 0U, TFT_ST7789_FONT_8X16, selected_parameter == PARAM_START ? TFT_ST7789_RED : TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 80, 216, stop_frequency_hz, 0U, TFT_ST7789_FONT_8X16, selected_parameter == PARAM_STOP ? TFT_ST7789_RED : TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawInt32(&tft, 168, 216, (int32_t)sweep_points, TFT_ST7789_FONT_8X16, selected_parameter == PARAM_POINTS ? TFT_ST7789_RED : TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_DrawFloat(&tft, 224, 216, target_vpp_v, 1U, TFT_ST7789_FONT_8X16, selected_parameter == PARAM_VPP ? TFT_ST7789_RED : TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
    display_dirty = false;
}

/* [FUYONG_ADAPTED][moni01] SysTick 只生产按键事件，不执行扫频和 TFT。 */
static void QueueKey(char symbol)
{
    const uint8_t next = (uint8_t)((key_queue_head + 1U) % KEY_QUEUE_SIZE);
    if (next != key_queue_tail) {
        key_queue[key_queue_head] = symbol;
        key_queue_head = next;
    }
}

void SysTick_Handler(void)
{
    static uint8_t elapsed_ms;
    char symbol;
    ++elapsed_ms;
    if (elapsed_ms < KEYPAD_SCAN_MS) return;
    elapsed_ms = 0U;
    if (SignalMatrixKeypad4x4_ReadNewSymbol(&symbol) == SIGNAL_RESULT_OK) {
        QueueKey(symbol);
    }
}

static void App_Init(void)
{
    const signal_dual_adc_config_t adc_config = { SAMPLE_RATE_REQUEST_HZ,
        CPUCLK_FREQ, 65536U };
    const signal_dac_dma_mspm0_config_t dac_config = { DAC_UPDATE_RATE_HZ,
        CPUCLK_FREQ, 65536U };
    const signal_wave_output_config_t wave_config = { dds_wave_table,
        DDS_TABLE_COUNT, dds_output_buffer, DDS_OUTPUT_COUNT,
        dac_config, 12U, ADC_REFERENCE_V };
    SYSCFG_DL_init();
    if (SignalDualADC_Init(&adc_config) != SIGNAL_RESULT_OK) while (true) { }
    DL_DMA_enableInterrupt(DMA, DL_DMA_INTERRUPT_CHANNEL0 | DL_DMA_INTERRUPT_CHANNEL1);
    if (SignalWaveOutput_Init(&wave_config) != SIGNAL_RESULT_OK) while (true) { }
    if (SignalTFTST7789_MSPM0_Init(&tft, TFT_ST7789_ROTATION_270, 0U, 0U) != TFT_ST7789_OK) while (true) { }
    if (SysTick_Config(CPUCLK_FREQ / 1000U) != 0U) while (true) { }
}

int main(void)
{
    App_Init();
    while (true) {
        HandleKeypad();
        if (sweep_state == SWEEP_RUNNING) (void)RunSweepPoint();
        if (display_dirty) UpdateDisplay();
        if (sweep_state != SWEEP_RUNNING) __WFI();
    }
}
