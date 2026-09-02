
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "arm_math.h"
#include "ti_msp_dl_config.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_dual_adc_phase.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_font.h"
#include "signal_tft_st7789_mspm0g3507.h"


#define SAMPLE_COUNT             (1024U)
#define ADC_REFERENCE_V          (3.3f)
#define REQUEST_SAMPLE_RATE_HZ   (100000U)
#define OFFSET_I                 (0.07f)
#define OFFSET_Q                 (0.07f)
#define DISPLAY_PERIOD_MS        (250U)
#define FRQ_GAIN                 (1.013731f)

#define VECTOR_PLOT_X       (116)
#define VECTOR_PLOT_Y       (24)
#define VECTOR_PLOT_WIDTH   (180)
#define VECTOR_PLOT_HEIGHT  (180)
#define VECTOR_INNER_X      (VECTOR_PLOT_X + 1)
#define VECTOR_INNER_Y      (VECTOR_PLOT_Y + 1)
#define VECTOR_INNER_WIDTH  (VECTOR_PLOT_WIDTH - 2)
#define VECTOR_INNER_HEIGHT (VECTOR_PLOT_HEIGHT - 2)
#define VECTOR_PLOT_MIDX    (VECTOR_PLOT_X + VECTOR_PLOT_WIDTH / 2)
#define VECTOR_PLOT_MIDY    (VECTOR_PLOT_Y + VECTOR_PLOT_HEIGHT / 2)
#define VECTOR_INNER_WIDTH_HALF   (VECTOR_INNER_WIDTH / 2)
#define VECTOR_INNER_HEIGHT_HALF  (VECTOR_INNER_HEIGHT / 2)
#define VECTOR_VMAX         (1.2f)

typedef struct {
    float mean_v;

} basic_result_t;


static uint16_t adc_ch1_samples[SAMPLE_COUNT];
static uint16_t adc_ch2_samples[SAMPLE_COUNT];
static float voltage_ch1_samples[SAMPLE_COUNT];
static float voltage_ch2_samples[SAMPLE_COUNT];


static float sample_rate_hz = (float)REQUEST_SAMPLE_RATE_HZ;
static float ch1_I_v, ch2_Q_v;
static float ch1_rms_v, ch1_vpp_v;
static float CH1_I = 0.0f, CH2_Q = 0.0f;
static float PF;

static float phase_deg;
float frequency_hz;


static tft_st7789_t tft;

static volatile uint16_t display_elapsed_ms;
static volatile bool display_due = true;

uint32_t last_counter;
uint32_t edge_counter;

static uint32_t set_pins = 0U;
static float v_gain_i = 1.0f;
static float v_gain_q = 1.0f;


static void DrawText(int32_t x, int32_t y, const char *text, uint16_t color)
{
    (void)TFT_ST7789_DrawString(&tft, x, y, text, TFT_ST7789_FONT_8X16,
        color, TFT_ST7789_BLACK, false, false);
}

static int32_t App_MapX(float vol)
{
    return VECTOR_PLOT_MIDX + (int32_t)((vol *
        (VECTOR_INNER_WIDTH_HALF)) / (VECTOR_VMAX));
}

static int32_t App_MapY(float vol)
{
    return VECTOR_PLOT_MIDY - (int32_t)((vol *
        (VECTOR_INNER_HEIGHT_HALF)) / (VECTOR_VMAX));
}

static void App_DrawVector(float voli, float volq)
{
    if(voli > VECTOR_VMAX) voli = VECTOR_VMAX;
    if(volq > VECTOR_VMAX) volq = VECTOR_VMAX;
    (void)TFT_ST7789_DrawLine(&tft, VECTOR_PLOT_MIDX,
            VECTOR_PLOT_MIDY, App_MapX(voli),
            App_MapY(volq), TFT_ST7789_YELLOW);

}

static void App_DrawGrid(void)
{
    uint8_t division;
    int32_t x;
    int32_t y;
    const uint16_t grid_color = TFT_ST7789_RGB565(55U, 75U, 85U);
    for (division = 1U; division < 6U; ++division) {
        x = VECTOR_PLOT_X + 1 + (int32_t)(((uint32_t)division *
            (VECTOR_PLOT_WIDTH - 3U)) / 6U);
        (void)TFT_ST7789_DrawLine(&tft, x, VECTOR_PLOT_Y + 1,
            x, VECTOR_PLOT_Y + VECTOR_PLOT_HEIGHT - 2, grid_color);
    }
    for (division = 1U; division < 6U; ++division) {
        y = VECTOR_PLOT_Y + 1 + (int32_t)(((uint32_t)division *
            (VECTOR_PLOT_HEIGHT - 3U)) / 6U);
        (void)TFT_ST7789_DrawLine(&tft, VECTOR_PLOT_X + 1, y,
            VECTOR_PLOT_X + VECTOR_PLOT_WIDTH - 2, y, grid_color);
    }
    (void)TFT_ST7789_DrawCircle(&tft, VECTOR_PLOT_MIDX, 
        VECTOR_PLOT_MIDY, 90, grid_color);
    (void)TFT_ST7789_DrawCircle(&tft, VECTOR_PLOT_MIDX, 
        VECTOR_PLOT_MIDY, 60, grid_color);
    (void)TFT_ST7789_DrawCircle(&tft, VECTOR_PLOT_MIDX, 
        VECTOR_PLOT_MIDY, 30, grid_color);

}

static bool AcquireDualADCFrame(void)
{
    if (SignalDualADC_Start(adc_ch1_samples, adc_ch2_samples,
            SAMPLE_COUNT) != SIGNAL_RESULT_OK) return false;
    while (!SignalDualADC_IsFinished()) __WFI();
    sample_rate_hz = (float)SignalDualADC_GetConfiguredRate();
    return sample_rate_hz > 0.0f;
}

static void ConvertADCToVoltage(const uint16_t *input_samples,
    float *output_samples, uint32_t sample_count)
{
    uint32_t index;
    for (index = 0U; index < sample_count; ++index) {
        output_samples[index] = (float)input_samples[index] *
            ADC_REFERENCE_V / 4095.0f;
    }
}

static void MeasureBasicParameters(const float *voltage_input,
    uint32_t sample_count, basic_result_t *result)
{
    arm_mean_f32(voltage_input, sample_count, &result->mean_v);
}

static void RunMeasurement(void)
{
    basic_result_t ch1, ch2;


    ConvertADCToVoltage(adc_ch1_samples, voltage_ch1_samples, SAMPLE_COUNT);
    ConvertADCToVoltage(adc_ch2_samples, voltage_ch2_samples, SAMPLE_COUNT);
    MeasureBasicParameters(voltage_ch1_samples, SAMPLE_COUNT, &ch1);
    MeasureBasicParameters(voltage_ch2_samples, SAMPLE_COUNT, &ch2);
    ch1_I_v = ch1.mean_v; ch2_Q_v = ch2.mean_v;
    CH1_I = (ch1_I_v * v_gain_i) - OFFSET_I; CH2_Q = (ch2_Q_v * v_gain_q) - OFFSET_Q;
    phase_deg = atan2f(CH2_Q, CH1_I) * 180.0 / 3.1415926;
    ch1_vpp_v = sqrt(CH1_I * CH1_I + CH2_Q * CH2_Q) * 8;
    ch1_rms_v = ch1_vpp_v / sqrt(2.0);
    PF = cosf(atan2f(CH2_Q, CH1_I));

}

static void DrawStaticUi(void)
{
    /* 整屏只在上电或翻页清一次，固定标签随后保持不动。 */
    (void)TFT_ST7789_FillScreen(&tft, TFT_ST7789_BLACK);
    
    DrawText(8, 4, "VECTOR VOLTMETER", TFT_ST7789_CYAN);
    DrawText(8, 24, "I/V:", TFT_ST7789_YELLOW);
    DrawText(8, 44, "Q/V:", TFT_ST7789_YELLOW);
    DrawText(8, 64, "VPP:", TFT_ST7789_YELLOW);
    DrawText(8, 84, "RMS:", TFT_ST7789_YELLOW);
    
    DrawText(8, 104, "PHS:", TFT_ST7789_YELLOW);
    DrawText(8, 124, "FRQ:", TFT_ST7789_YELLOW);
    DrawText(8, 144, "PF :", TFT_ST7789_YELLOW);
    TFT_ST7789_DrawRect(
        &tft, VECTOR_PLOT_X, VECTOR_PLOT_Y,
        VECTOR_PLOT_WIDTH, VECTOR_PLOT_HEIGHT,
        TFT_ST7789_BLUE);
    DrawText(116, 208, "X/I=0.4V Y/Q=0.4V R=0.4V", TFT_ST7789_YELLOW);
    
}

static void UpdateDisplay(void)
{
    

    (void)TFT_ST7789_FillRect(&tft, 40, 24, 48, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawFloat(&tft, 40, 24, CH1_I, 3U, TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_FillRect(&tft, 40, 44, 48, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawFloat(&tft, 40, 44, CH2_Q, 3U, TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_FillRect(&tft, 40, 64, 40, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawFloat(&tft, 40, 64, ch1_vpp_v, 3U, TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_FillRect(&tft, 40, 84, 40, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawFloat(&tft, 40, 84, ch1_rms_v, 3U, TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_FillRect(&tft, 40, 104, 62, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawFloat(&tft, 40, 104, phase_deg, 1U, TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_FillRect(&tft, 40, 124, 56, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawFloat(&tft, 40, 124, frequency_hz / 1000, 1U, TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    (void)TFT_ST7789_FillRect(&tft, 40, 144, 56, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawFloat(&tft, 40, 144, PF, 2U, TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK, false);
    
    
    (void)TFT_ST7789_FillRect(&tft,VECTOR_INNER_X, VECTOR_INNER_Y, VECTOR_INNER_WIDTH, VECTOR_INNER_HEIGHT, TFT_ST7789_BLACK);
    App_DrawGrid();
    App_DrawVector(CH1_I * 4,CH2_Q * 4);

    display_due = false; display_elapsed_ms = 0U;
}

static void CTRL_GAIN(float freq)
{

    const uint32_t all_pins = GPIO_GAIN_A_PIN |
        GPIO_GAIN_B_PIN |
        GPIO_GAIN_C_PIN |
        GPIO_GAIN_D_PIN ;
    if(freq > 1399)
    {
        set_pins = GPIO_GAIN_A_PIN | GPIO_GAIN_B_PIN | GPIO_GAIN_C_PIN;
    }
    else if (freq > 1219)
    {
        set_pins = GPIO_GAIN_B_PIN | GPIO_GAIN_C_PIN;
    }
    else if (freq > 1063)
    {
        set_pins = GPIO_GAIN_A_PIN | GPIO_GAIN_C_PIN;
    }
    else if (freq > 926)
    {
        set_pins = GPIO_GAIN_C_PIN;
    }
    else if (freq > 807)
    {
        set_pins = GPIO_GAIN_A_PIN | GPIO_GAIN_B_PIN | GPIO_GAIN_D_PIN;
    }
    else if (freq > 704)
    {
        set_pins = GPIO_GAIN_B_PIN | GPIO_GAIN_D_PIN;
    }
    else if (freq > 614)
    {
        set_pins = GPIO_GAIN_A_PIN | GPIO_GAIN_D_PIN;
    }
    else 
    {
        set_pins = GPIO_GAIN_D_PIN;
    }
    DL_GPIO_clearPins(GPIO_GAIN_PORT, all_pins & ~set_pins);
    DL_GPIO_setPins(GPIO_GAIN_PORT, set_pins);
    
}


void SysTick_Handler(void)
{
    

    if (display_elapsed_ms < DISPLAY_PERIOD_MS) ++display_elapsed_ms;
    else display_due = true;
   

    }


static void App_Init(void)
{
    const signal_dual_adc_config_t config = {
        REQUEST_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U
    };
    SYSCFG_DL_init();
    if (SignalDualADC_Init(&config) != SIGNAL_RESULT_OK) while (true) { }
    DL_DMA_enableInterrupt(DMA, DL_DMA_INTERRUPT_CHANNEL0 |
        DL_DMA_INTERRUPT_CHANNEL1);
    if (SignalTFTST7789_MSPM0_Init(&tft, TFT_ST7789_ROTATION_270,
            0U, 0U) != TFT_ST7789_OK) while (true) { }
    if (SysTick_Config(CPUCLK_FREQ / 1000U) != 0U) while (true) { }
    
    DrawStaticUi();
    DL_TimerG_startCounter(COMPARE_0_INST);
    last_counter = DL_TimerG_getTimerCount(COMPARE_0_INST);
    DL_TimerG_startCounter(GATE_0_INST);
    NVIC_EnableIRQ(GATE_0_INST_INT_IRQN);

}

int main(void)
{
    App_Init();
    while (true) {
        
        if (!AcquireDualADCFrame()) continue;
        CTRL_GAIN(frequency_hz / 1000); 
        //App_ErasePrevious(CH1_I, CH2_Q);       
        RunMeasurement();

        if (display_due) UpdateDisplay();

        
    }
}

void GATE_0_INST_IRQHandler(void)
{
    volatile uint32_t now_counter = DL_TimerG_getTimerCount(COMPARE_0_INST);
    switch (DL_TimerG_getPendingInterrupt(GATE_0_INST)) {
        case DL_TIMERG_IIDX_LOAD:
            if(last_counter >= now_counter)
            {
                edge_counter = last_counter - now_counter;
            }
            else{
                edge_counter = 65535 - now_counter + last_counter;
            }
            frequency_hz = (edge_counter * 100U) * FRQ_GAIN;
            last_counter = now_counter;
            break;
        default:
            break;
    }
}