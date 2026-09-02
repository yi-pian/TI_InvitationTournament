/**
 * @file main.c
 * @brief TEST ONLY：采集一帧 TMP6131 原始码并经 XDS110 UART 输出 CSV。
 *
 * UART 只存在于本例程。signal_adc_dma.c/.h 不包含任何串口依赖。
 */

#include <stdbool.h>
#include <stdint.h>

#include "signal_adc_dma.h"
#include "signal_config.h"
#include "ti_msp_dl_config.h"

uint16_t g_adc_buffer[SIGNAL_UART_DUMP_SAMPLE_COUNT];

volatile uint16_t g_adc_raw_min;
volatile uint16_t g_adc_raw_max;
volatile uint32_t g_adc_raw_mean;
volatile uint32_t g_dumped_samples;
volatile bool g_uart_dump_complete;
volatile bool g_uart_dump_pass;
volatile signal_result_t g_last_result;
volatile signal_status_t g_module_status;

static void UARTDump_PutChar(char character)
{
    DL_UART_Main_transmitDataBlocking(UART_0_INST, (uint8_t) character);
}

static void UARTDump_PutString(const char *text)
{
    while (*text != '\0') {
        UARTDump_PutChar(*text);
        text++;
    }
}

static void UARTDump_PutUnsigned(uint32_t value)
{
    char digits[10];
    uint32_t length = 0U;

    do {
        digits[length] = (char) ('0' + (value % 10U));
        length++;
        value /= 10U;
    } while (value != 0U);

    while (length > 0U) {
        length--;
        UARTDump_PutChar(digits[length]);
    }
}

static bool UARTDump_CalculateStats(void)
{
    uint16_t i;
    uint16_t minimum = UINT16_MAX;
    uint16_t maximum = 0U;
    uint32_t sum = 0U;

    for (i = 0U; i < SIGNAL_UART_DUMP_SAMPLE_COUNT; i++) {
        const uint16_t sample = g_adc_buffer[i];

        if (sample > 4095U) {
            return false;
        }
        if (sample < minimum) {
            minimum = sample;
        }
        if (sample > maximum) {
            maximum = sample;
        }
        sum += sample;
    }

    g_adc_raw_min = minimum;
    g_adc_raw_max = maximum;
    g_adc_raw_mean =
        (sum + (SIGNAL_UART_DUMP_SAMPLE_COUNT / 2U)) /
        SIGNAL_UART_DUMP_SAMPLE_COUNT;
    return !(((minimum == 0U) && (maximum == 0U)) ||
             ((minimum == 4095U) && (maximum == 4095U)));
}

static void UARTDump_WriteCSV(void)
{
    uint16_t index;

    UARTDump_PutString("INDEX,ADC_RAW\r\n");
    for (index = 0U; index < SIGNAL_UART_DUMP_SAMPLE_COUNT; index++) {
        UARTDump_PutUnsigned(index);
        UARTDump_PutChar(',');
        UARTDump_PutUnsigned(g_adc_buffer[index]);
        UARTDump_PutString("\r\n");
        g_dumped_samples = (uint32_t) index + 1U;
    }
    while (DL_UART_Main_isBusy(UART_0_INST)) {
        /* 等待最后一个停止位离开 UART；不影响已经结束的 ADC 采集。 */
    }
}

int main(void)
{
    const signal_adc_dma_config_t adc_config = {
        .sample_rate_hz = SIGNAL_UART_DUMP_SAMPLE_RATE_HZ,
        .timer_clock_hz = CPUCLK_FREQ,
        .timer_max_count = 65536U,
    };

    SYSCFG_DL_init();
    g_uart_dump_complete = false;
    g_uart_dump_pass = false;
    g_dumped_samples = 0U;

    g_last_result = SignalADC_Init(&adc_config);
    if (g_last_result == SIGNAL_RESULT_OK) {
        g_last_result = SignalADC_Start(
            g_adc_buffer, SIGNAL_UART_DUMP_SAMPLE_COUNT);
    }
    if (g_last_result == SIGNAL_RESULT_OK) {
        while (!SignalADC_IsFinished()) {
            __WFE();
        }
    }

    g_module_status = SignalADC_GetStatus();
    if ((g_last_result == SIGNAL_RESULT_OK) &&
        (g_module_status == MODULE_DONE) && UARTDump_CalculateStats()) {
        UARTDump_WriteCSV();
        g_uart_dump_pass = true;
    }

    g_uart_dump_complete = true;
    __BKPT(0);

    while (1) {
        __WFI();
    }
}
