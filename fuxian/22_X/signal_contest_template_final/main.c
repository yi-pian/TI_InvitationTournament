/*
 * 22_X 总流程：双路同步 ADC/DMA 采样 -> 李萨如坐标映射 -> ILI9341 绘图 ->
 * 矩阵键盘控制 PLL/YV 倍频 -> 双路过零与相位差显示。
 * signal_* 的驱动、算法和调用格式来自集成库 README；本文件自己编写的是
 * 参数状态机、按键分发、坐标映射、页面局部刷新和 main 流程编排。硬件引脚、
 * DMA 和定时器要改 SysConfig；显示范围、颜色和刷新周期改 APP_* 宏。
 */
#include <stdint.h>
#include "ti_msp_dl_config.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_dual_adc_phase.h"
#include "signal_matrix_keypad_4x4.h"
#include "signal_tft_ili9341.h"
#include "signal_tft_ili9341_mspm0g3507.h"

// ============================================================
// 用户通常只需要修改这里
// ============================================================

// 每一路 ADC 每秒采样 500000 次，即 500 kSPS。
// 最高输入频率变化时，优先修改它；观察波形通常取最高频率的 10~50 倍。
#define SIGNAL_SAMPLE_RATE_HZ  (500000U)

// 每轮每一路采集 1024 个点。N 增大，观察时间更长、RAM 也更多。
// 两路 raw 缓冲区共占 4 * N 字节；1024 点共占 4096 字节。
#define SIGNAL_SAMPLE_COUNT    (1024U)

#define LISSAJOUS_PLOT_X       (20)
#define LISSAJOUS_PLOT_Y       (30)
#define LISSAJOUS_PLOT_WIDTH   (180)
#define LISSAJOUS_PLOT_HEIGHT  (180)
#define LISSAJOUS_POINT_COUNT  (280U)
#define ADC12_FULL_SCALE       (4095U)
#define PHASE_HYSTERESIS_CODE  (16U)
#define PHASE_MIN_AMPLITUDE    (64U)
#define LISSAJOUS_INNER_X      (LISSAJOUS_PLOT_X + 1)
#define LISSAJOUS_INNER_Y      (LISSAJOUS_PLOT_Y + 1)
#define LISSAJOUS_INNER_WIDTH  (LISSAJOUS_PLOT_WIDTH - 2)
#define LISSAJOUS_INNER_HEIGHT (LISSAJOUS_PLOT_HEIGHT - 2)

/* g_raw_a/g_raw_b：双 ADC DMA 原始码；g_tft：屏幕句柄；g_*_revision：数值变化标志，
 * 用于只刷新变化字段；g_phase_config：相位算法门限/采样率配置。 */
static uint16_t g_raw_a[SIGNAL_SAMPLE_COUNT];
static uint16_t g_raw_b[SIGNAL_SAMPLE_COUNT];
static tft_ili9341_t g_tft;
volatile signal_result_t g_adc_status;
volatile tft_ili9341_status_t g_tft_status;

#define APP_KEYPAD_SCAN_PERIOD_MS  (5U)

static volatile signal_result_t g_keypad_status;
static volatile uint8_t g_pll_multiplier = 1U;
static volatile uint8_t g_pll_display_revision = 1U;
static uint8_t g_pll_displayed_revision = 1U;
static volatile uint8_t g_yv_multiplier = 1U;
static volatile uint8_t g_yv_display_revision = 1U;
static uint8_t g_yv_displayed_revision = 1U;
static volatile uint8_t g_wave_mode = 0U;
static volatile uint8_t g_wave_display_revision = 1U;
static uint8_t g_wave_displayed_revision = 1U;

static volatile int16_t g_phase_degrees = 0;
static volatile uint8_t g_phase_valid = 0U;
static volatile uint8_t g_phase_display_revision = 1U;
static uint8_t g_phase_displayed_revision = 1U;

static signal_dual_adc_phase_config_t g_phase_config = {
    .hysteresis_code = PHASE_HYSTERESIS_CODE,
    .min_amplitude_code = PHASE_MIN_AMPLITUDE,
    .frequency_ratio = 1U,
    .max_x_crossings = 16U,
    .max_y_crossings = 64U
};

/* 函数索引：App_SetPLLMultiplier 写 ABCD 倍频控制；App_SetYVMultiplier 写波形档位；
 * App_SetWaveMode 控制模拟开关；App_Process*Key 把 symbol 分发到功能；SysTick_Handler
 * 定时扫描键盘；App_Draw* 绘制局部数字；Lissajous_Map* 把 ADC 码换成像素；
 * Lissajous_DrawStaticFrame 只画边框/轴，Lissajous_DrawFrame 只更新轨迹；main 负责初始化、
 * DMA 采样、相位算法和循环调度。multiplier 是 1~5 倍频，mode 是 0/1 波形档位，symbol
 * 是键盘得到的字符，sample 是 12 位 ADC 采样码。 */
/* 自写逻辑：设置 PLL 倍频状态。multiplier=1~5；同时更新显示版本号。
 * 要修改倍频映射，改本函数中的 GPIO ABCD 输出，不改 PLL 模块。 */
static void App_SetPLLMultiplier(uint8_t multiplier)
{
    uint32_t set_pins = 0U;
    const uint32_t all_pins = GPIO_PLL_CTRL_PLL_A_PIN |
        GPIO_PLL_CTRL_PLL_B_PIN | GPIO_PLL_CTRL_PLL_C_PIN |
        GPIO_PLL_CTRL_PLL_D_PIN;

    switch (multiplier) {
    case 2U:
        set_pins = GPIO_PLL_CTRL_PLL_B_PIN | GPIO_PLL_CTRL_PLL_C_PIN |
            GPIO_PLL_CTRL_PLL_D_PIN;
        break;
    case 3U:
        set_pins = GPIO_PLL_CTRL_PLL_A_PIN | GPIO_PLL_CTRL_PLL_C_PIN |
            GPIO_PLL_CTRL_PLL_D_PIN;
        break;
    case 4U:
        set_pins = GPIO_PLL_CTRL_PLL_C_PIN | GPIO_PLL_CTRL_PLL_D_PIN;
        break;
    case 5U:
        set_pins = GPIO_PLL_CTRL_PLL_A_PIN | GPIO_PLL_CTRL_PLL_B_PIN |
            GPIO_PLL_CTRL_PLL_D_PIN;
        break;
    default:
        multiplier = 1U;
        break;
    }

    DL_GPIO_clearPins(GPIO_PLL_CTRL_PORT, all_pins & ~set_pins);
    DL_GPIO_setPins(GPIO_PLL_CTRL_PORT, set_pins);
    g_pll_multiplier = multiplier;
    ++g_pll_display_revision;
}

/* 自写逻辑：设置 YV 波形/倍频档位。multiplier 是屏幕显示值和控制码的逻辑状态。 */
static void App_SetYVMultiplier(uint8_t multiplier)
{
    uint32_t set_pins = 0U;
    const uint32_t all_pins = GPIO_YV_CTRL_YV_A_PIN |
        GPIO_YV_CTRL_YV_B_PIN ;

    switch (multiplier) {
    case 2U:
        set_pins = GPIO_YV_CTRL_YV_A_PIN ;
        break;
    case 3U:
        set_pins = GPIO_YV_CTRL_YV_B_PIN ;
        break;
    default:
        multiplier = 1U;
        break;
    }

    DL_GPIO_clearPins(GPIO_YV_CTRL_PORT, all_pins & ~set_pins);
    DL_GPIO_setPins(GPIO_YV_CTRL_PORT, set_pins);
    g_yv_multiplier = multiplier;
    ++g_yv_display_revision;
}

/* 自写逻辑：控制模拟开关波形。mode=0/1 对应正弦/三角波，并置位局部刷新标志。 */
static void App_SetWaveMode(uint8_t mode)
{
    mode = (mode != 0U) ? 1U : 0U;

    if (mode == 0U) {
        DL_GPIO_clearPins(
            GPIO_WAVE_CTRL_PORT,
            GPIO_WAVE_CTRL_WAVE_SELECT_PIN);
    } else {
        DL_GPIO_setPins(
            GPIO_WAVE_CTRL_PORT,
            GPIO_WAVE_CTRL_WAVE_SELECT_PIN);
    }

    g_wave_mode = mode;
    ++g_wave_display_revision;
}

/* 自写逻辑：把矩阵键盘字符 1~5 转成 PLL 倍频设置；其他键忽略。 */
static void App_ProcessPLLKey(char symbol)
{
    if ((symbol >= '1') && (symbol <= '5')) {
        App_SetPLLMultiplier((uint8_t)(symbol - '0'));
    }
}

/* 自写逻辑：把 A/B/C/D 转成 YV 档位或波形切换；symbol 是键盘模块输出的字符。 */
static void App_ProcessYVKey(char symbol)
{
    if (symbol == 'A') {
        App_SetYVMultiplier(1U);       
    }
    else if (symbol == 'B') {
        App_SetYVMultiplier(2U); 
    }
    else if (symbol == 'C') {
        App_SetYVMultiplier(3U); 
    }
    else if (symbol == 'D') {
        App_SetWaveMode((uint8_t)(g_wave_mode == 0U));
    }
}

/* 自写调度逻辑：SysTick 每 1 ms 进入，累计到 5 ms 后调用键盘模块的稳定扫描接口。
 * 中断只做扫描和状态更新，不在这里执行 SPI 绘图或浮点计算。 */
void SysTick_Handler(void)
{
    static uint8_t milliseconds;
    char symbol;

    ++milliseconds;
    if (milliseconds < APP_KEYPAD_SCAN_PERIOD_MS) return;
    milliseconds = 0U;
    /* 【矩阵键盘模块】一次调用完成固定 GPIO 行列扫描、3 次消抖和鬼键过滤；
     * 只有出现“新按下”的稳定按键才返回 SIGNAL_RESULT_OK。 */
    g_keypad_status = SignalMatrixKeypad4x4_ReadNewSymbol(&symbol);
    if (g_keypad_status == SIGNAL_RESULT_OK) {
        App_ProcessPLLKey(symbol);
        App_ProcessYVKey(symbol);
    }
}

/* 自写显示逻辑：只清除 PLL 数字区域并用 ILI9341 8x16 字库重画 multiplier。 */
static tft_ili9341_status_t App_DrawPLLMultiplier(uint8_t multiplier)
{
    tft_ili9341_status_t status;

    /* 【ILI9341 模块】先擦除旧数字的 8x16 区域，边框和其他文字不刷新。 */
    status = TFT_ILI9341_FillRect(
        &g_tft, 258, 30, 8, 16, TFT_ILI9341_BLACK);
    if (status != TFT_ILI9341_OK) return status;
    return TFT_ILI9341_DrawInt32(
        &g_tft, 258, 30, multiplier, TFT_ILI9341_FONT_8X16,
        TFT_ILI9341_WHITE, TFT_ILI9341_BLACK, false);
}

/* 自写显示逻辑：只刷新 YV 数字区域；底层 DrawInt32 来自 ILI9341 README。 */
static tft_ili9341_status_t App_DrawYVMultiplier(uint8_t multiplier)
{
    tft_ili9341_status_t status;

    /* 【ILI9341 模块】局部清除 YV 数值字段。 */
    status = TFT_ILI9341_FillRect(
        &g_tft, 258, 60, 8, 16, TFT_ILI9341_BLACK);
    if (status != TFT_ILI9341_OK) return status;
    return TFT_ILI9341_DrawInt32(
        &g_tft, 258, 60, multiplier, TFT_ILI9341_FONT_8X16,
        TFT_ILI9341_WHITE, TFT_ILI9341_BLACK, false);
}

/* 自写显示逻辑：刷新波形模式文字；mode=0 显示正弦，mode=1 显示三角。 */
static tft_ili9341_status_t App_DrawWaveMode(uint8_t mode)
{
    tft_ili9341_status_t status;

    /* 【ILI9341 模块】波形名称长度可能不同，因此先清完整文字区域。 */
    status = TFT_ILI9341_FillRect(
        &g_tft, 234, 90, 48, 16, TFT_ILI9341_BLACK);
    if (status != TFT_ILI9341_OK) return status;

    return TFT_ILI9341_DrawString(
        &g_tft, 234, 90, (mode == 0U) ? "SIN" : "TRI",
        TFT_ILI9341_FONT_8X16, TFT_ILI9341_WHITE,
        TFT_ILI9341_BLACK, false, false);
}

/* 自写显示逻辑：显示相位角和有效标志；相位计算本身由 phase 模块完成。 */
static tft_ili9341_status_t App_DrawPhaseDegrees(
    int16_t phase_degrees, uint8_t valid)
{
    tft_ili9341_status_t status;

    status = TFT_ILI9341_FillRect(
        &g_tft, 234, 120, 48, 16, TFT_ILI9341_BLACK);
    if (status != TFT_ILI9341_OK) return status;

    if (valid == 0U) {
        return TFT_ILI9341_DrawString(
            &g_tft, 234, 120, "----", TFT_ILI9341_FONT_8X16,
            TFT_ILI9341_WHITE, TFT_ILI9341_BLACK, false, false);
    }

    return TFT_ILI9341_DrawInt32(
        &g_tft, 234, 120, phase_degrees, TFT_ILI9341_FONT_8X16,
        TFT_ILI9341_WHITE, TFT_ILI9341_BLACK, false);
}

/* 自写坐标逻辑：把 X 路 ADC 原始码按 0~4095 映射到李萨如图 X 像素。 */
static int32_t Lissajous_MapX(uint16_t sample)
{
    return LISSAJOUS_INNER_X +
        (int32_t)(((uint32_t)sample * (LISSAJOUS_INNER_WIDTH - 1)) /
                  ADC12_FULL_SCALE);
}

/* 自写坐标逻辑：把 Y 路 ADC 原始码映射到屏幕 Y 像素，并反转屏幕坐标方向。 */
static int32_t Lissajous_MapY(uint16_t sample)
{
    return LISSAJOUS_INNER_Y + LISSAJOUS_INNER_HEIGHT - 1 -
        (int32_t)(((uint32_t)sample * (LISSAJOUS_INNER_HEIGHT - 1)) /
                  ADC12_FULL_SCALE);
}

/* 自写显示逻辑：只画一次蓝色边框、坐标轴和标题；页面切换时再次调用。 */
static tft_ili9341_status_t Lissajous_DrawStaticFrame(void)
{
    tft_ili9341_status_t status;

    status = TFT_ILI9341_FillRect(
        &g_tft, LISSAJOUS_PLOT_X, LISSAJOUS_PLOT_Y,
        LISSAJOUS_PLOT_WIDTH, LISSAJOUS_PLOT_HEIGHT,
        TFT_ILI9341_BLACK);
    if (status != TFT_ILI9341_OK) return status;

    return TFT_ILI9341_DrawRect(
        &g_tft, LISSAJOUS_PLOT_X, LISSAJOUS_PLOT_Y,
        LISSAJOUS_PLOT_WIDTH, LISSAJOUS_PLOT_HEIGHT,
        TFT_ILI9341_BLUE);
}

/* 自写显示逻辑：清除上一帧轨迹后，将双 ADC 同步样本逐点连线形成李萨如图。 */
static tft_ili9341_status_t Lissajous_DrawFrame(void)
{
    uint16_t point; 
    uint32_t index0;
    uint32_t index1;
    tft_ili9341_status_t status;

    status = TFT_ILI9341_FillRect(
        &g_tft, LISSAJOUS_INNER_X, LISSAJOUS_INNER_Y,
        LISSAJOUS_INNER_WIDTH, LISSAJOUS_INNER_HEIGHT,
        TFT_ILI9341_BLACK);
    if (status != TFT_ILI9341_OK) return status;

    for (point = 0U; point + 1U < LISSAJOUS_POINT_COUNT; ++point) {
        index0 = ((uint32_t)point * (SIGNAL_SAMPLE_COUNT - 1U)) /
                 (LISSAJOUS_POINT_COUNT - 1U);
        index1 = ((uint32_t)(point + 1U) * (SIGNAL_SAMPLE_COUNT - 1U)) /
                 (LISSAJOUS_POINT_COUNT - 1U);
        status = TFT_ILI9341_DrawLine(
            &g_tft, Lissajous_MapX(g_raw_a[index0]),
            Lissajous_MapY(g_raw_b[index0]),
            Lissajous_MapX(g_raw_a[index1]),
            Lissajous_MapY(g_raw_b[index1]), TFT_ILI9341_YELLOW);
        if (status != TFT_ILI9341_OK) return status;
    }
    return TFT_ILI9341_OK;
}

/* main：先初始化 SysConfig 和各模块，再启动 DMA/SysTick；循环中读取 DMA 完成的
 * 双路数据，调用相位模块，按 revision 只更新变化的 PLL、YV、波形和相位数字，
 * 最后进入 WFI 等待中断。若要改采样率，看 adc_config；若要改键盘扫描周期，
 * 改 APP_KEYPAD_SCAN_PERIOD_MS。 */
int main(void)
{
    // timer_clock_hz 必须填写 SysConfig 中采样 Timer 的实际输入时钟。
    // PROFILE_02_DUAL_ADC 使用 BUSCLK/1/1，故这里可用 CPUCLK_FREQ。
    // 65536U 是 16 位 Timer 可表示的最大周期数，初学者通常不要修改。
    const signal_dual_adc_config_t config = {
        .sample_rate_hz = SIGNAL_SAMPLE_RATE_HZ,
        .timer_clock_hz = CPUCLK_FREQ,
        .timer_max_count = 65536U,
    };

    // 先由 SysConfig 初始化 Timer、两路 ADC、DMA 和 Event。
    SYSCFG_DL_init();

    // 模块初始化只需在上电后执行一次。
    /* 【双路同步 ADC 模块】config 指定采样率、CPU 时钟和定时器计数范围；
     * 模块内部配置 DMA 中断和同步启动，main 不直接操作 ADC 寄存器。 */
    g_adc_status = SignalDualADC_Init(&config);
    if (g_adc_status != SIGNAL_RESULT_OK) while (1) { }

    App_SetPLLMultiplier(1U);
    App_SetYVMultiplier(1U);
    App_SetWaveMode(0U);

    /* 【ILI9341 MSPM0 适配模块】绑定 SysConfig SPI/GPIO 并初始化屏幕方向。 */
    g_tft_status = SignalTFTILI9341_MSPM0_Init(
        &g_tft, TFT_ILI9341_ROTATION_270);
    if (g_tft_status != TFT_ILI9341_OK) while (1) { }
    g_tft_status = TFT_ILI9341_FillScreen(&g_tft, TFT_ILI9341_BLACK);
    if (g_tft_status == TFT_ILI9341_OK) {
        g_tft_status = TFT_ILI9341_DrawString(
            &g_tft, 8, 8, "LiSaRu",
            TFT_ILI9341_FONT_8X16, TFT_ILI9341_WHITE,
            TFT_ILI9341_BLACK, false, false);
        g_tft_status = TFT_ILI9341_DrawString(
            &g_tft, 210, 30, "PLL:",
            TFT_ILI9341_FONT_8X16, TFT_ILI9341_WHITE,
            TFT_ILI9341_BLACK, false, false);
        g_tft_status = TFT_ILI9341_DrawString(
            &g_tft, 210, 60, "YV:",
            TFT_ILI9341_FONT_8X16, TFT_ILI9341_WHITE,
            TFT_ILI9341_BLACK, false, false);
        g_tft_status = TFT_ILI9341_DrawString(
            &g_tft, 210, 90, "WV:",
            TFT_ILI9341_FONT_8X16, TFT_ILI9341_WHITE,
            TFT_ILI9341_BLACK, false, false);
        g_tft_status = TFT_ILI9341_DrawString(
            &g_tft, 210, 120, "PH:",
            TFT_ILI9341_FONT_8X16, TFT_ILI9341_WHITE,
            TFT_ILI9341_BLACK, false, false);
        g_tft_status = App_DrawPLLMultiplier(g_pll_multiplier);
        g_tft_status = App_DrawYVMultiplier(g_yv_multiplier);
        g_tft_status = App_DrawWaveMode(g_wave_mode);
        g_tft_status = App_DrawPhaseDegrees(g_phase_degrees, g_phase_valid);
        g_pll_displayed_revision = g_pll_display_revision;
        g_yv_displayed_revision = g_yv_display_revision;
        g_wave_displayed_revision = g_wave_display_revision;
        g_phase_displayed_revision = g_phase_display_revision;
    }
    if (g_tft_status == TFT_ILI9341_OK) {
        g_tft_status = Lissajous_DrawStaticFrame();
    }
    if (g_tft_status != TFT_ILI9341_OK) while (1) { }
    if (SysTick_Config(CPUCLK_FREQ / 1000U) != 0U) while (1) { }

    while (1) {
        // 启动本轮两路同步 ADC + DMA 采集。
        /* 【双路同步 ADC 模块】把两路 DMA 目标分别设为 g_raw_a/g_raw_b。 */
        g_adc_status = SignalDualADC_Start(
            g_raw_a, g_raw_b, SIGNAL_SAMPLE_COUNT);
        if (g_adc_status != SIGNAL_RESULT_OK) while (1) { }

        // 等待两路 DMA 都完成；等待期间 CPU 进入低功耗等待中断状态。
        /* DMA 完成标志由模块中断设置，WFI 避免等待时空转。 */
        while (!SignalDualADC_IsFinished()) { __WFI(); }

        // ===== 从这里开始写自己的信号处理逻辑 =====
        // g_raw_a[i] 与 g_raw_b[i] 对应同一次 Timer 触发的两个 ADC 原始码。
        // 这里将 X 映射到横轴、Y 映射到纵轴，绘制李萨如轨迹。
        signal_dual_adc_phase_result_t phase_result;
        signal_algorithm_status_t phase_status;
        g_phase_config.frequency_ratio = g_pll_multiplier;
        /* 【双路同步 ADC 相位模块】输入同步样本、采样率和门限配置；模块寻找 X 上升
         * 过零点及其附近 Y 过零点，并返回最小等效相位差。 */
        phase_status = SignalDualADCPhase_Process(
            g_raw_a, g_raw_b, SIGNAL_SAMPLE_COUNT,
            SIGNAL_SAMPLE_RATE_HZ, &g_phase_config, &phase_result);
        if ((phase_status == SIGNAL_ALGORITHM_OK) &&
            (phase_result.valid != 0U)) {
            g_phase_degrees = phase_result.phase_degrees;
            g_phase_valid = 1U;
        } else {
            g_phase_valid = 0U;
        }
        ++g_phase_display_revision;

        uint8_t revision = g_pll_display_revision;
        uint8_t revision2 = g_yv_display_revision;
        uint8_t revision3 = g_wave_display_revision;
        uint8_t phase_revision = g_phase_display_revision;
        if ((revision != g_pll_displayed_revision) ||
            (revision2 != g_yv_displayed_revision) ||
            (revision3 != g_wave_displayed_revision)) {
            g_tft_status = App_DrawPLLMultiplier(g_pll_multiplier);
            if (g_tft_status != TFT_ILI9341_OK) while (1) { }
            g_tft_status = App_DrawYVMultiplier(g_yv_multiplier);
            if (g_tft_status != TFT_ILI9341_OK) while (1) { }
            g_tft_status = App_DrawWaveMode(g_wave_mode);
            if (g_tft_status != TFT_ILI9341_OK) while (1) { }
            g_pll_displayed_revision = revision;
            g_yv_displayed_revision = revision2;
            g_wave_displayed_revision = revision3;
        }
        if (phase_revision != g_phase_displayed_revision) {
            g_tft_status = App_DrawPhaseDegrees(
                g_phase_degrees , g_phase_valid);
            if (g_tft_status != TFT_ILI9341_OK) while (1) { }
            g_phase_displayed_revision = phase_revision;
        }
        g_tft_status = Lissajous_DrawFrame();
        if (g_tft_status != TFT_ILI9341_OK) while (1) { }
    }
}
