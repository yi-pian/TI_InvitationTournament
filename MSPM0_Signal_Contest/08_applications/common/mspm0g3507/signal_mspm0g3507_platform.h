#ifndef SIGNAL_MSPM0G3507_PLATFORM_H
#define SIGNAL_MSPM0G3507_PLATFORM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <ti/driverlib/driverlib.h>

#include "signal_adc.h"
#include "signal_button.h"
#include "signal_comparator.h"
#include "signal_dac.h"
#include "signal_dma.h"
#include "signal_gpio.h"
#include "signal_latching_button_switch.h"
#include "signal_matrix_keypad_4x4.h"
#include "signal_timer.h"
#include "signal_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Software-triggered, single-result ADC12 binding. */
typedef struct {
    ADC12_Regs *instance;
    DL_ADC12_MEM_IDX memory_index;
    uint32_t result_interrupt_mask;
    uint32_t timeout_iterations;
} signal_mspm0g3507_adc_context_t;

signal_result_t SignalMSPM0G3507_ADC_Bind(
    signal_adc_t *adc,
    signal_mspm0g3507_adc_context_t *context,
    uint8_t channel,
    uint8_t resolution_bits,
    float reference_voltage_v,
    uint32_t clock_hz);
signal_result_t SignalMSPM0G3507_ADC_Read(void *context, uint16_t *raw);
signal_result_t SignalMSPM0G3507_ADC_Enable(void *context);
signal_result_t SignalMSPM0G3507_ADC_Disable(void *context);

/* DAC12 single-code binding. The descriptor exposes 12-bit unsigned codes. */
signal_result_t SignalMSPM0G3507_DAC_Bind(signal_dac_t *dac,
    DAC12_Regs *instance, float reference_voltage_v);
signal_result_t SignalMSPM0G3507_DAC_Write(void *context, uint16_t raw);

/* Generic GPIO port binding. pin is a DL_GPIO_PIN_x bit mask. */
signal_result_t SignalMSPM0G3507_GPIO_Bind(signal_gpio_port_t *port,
    GPIO_Regs *instance);
signal_result_t SignalMSPM0G3507_GPIO_Write(void *context, uint32_t pin,
    bool high);
signal_result_t SignalMSPM0G3507_GPIO_Read(void *context, uint32_t pin,
    bool *high);
signal_result_t SignalMSPM0G3507_GPIO_Toggle(void *context, uint32_t pin);

/* Active-level GPIO input used by Button and Latching Button Switch. */
typedef struct {
    GPIO_Regs *port;
    uint32_t pin;
    bool active_low;
} signal_mspm0g3507_gpio_input_t;

signal_result_t SignalMSPM0G3507_GPIO_ReadActive(void *context, bool *active);

/* 4x4 keypad GPIO table. Each pin may be on a different GPIO port. */
typedef struct {
    GPIO_Regs *port;
    uint32_t pin;
} signal_mspm0g3507_gpio_pin_t;

typedef struct {
    signal_mspm0g3507_gpio_pin_t rows[SIGNAL_MATRIX_KEYPAD_4X4_ROWS];
    signal_mspm0g3507_gpio_pin_t columns[SIGNAL_MATRIX_KEYPAD_4X4_COLUMNS];
    uint32_t cpu_clock_hz;
} signal_mspm0g3507_keypad_context_t;

signal_result_t SignalMSPM0G3507_KeypadDriveRow(void *context, uint8_t row,
    bool active);
signal_result_t SignalMSPM0G3507_KeypadReadColumn(void *context,
    uint8_t column, bool *active);
void SignalMSPM0G3507_DelayUs(void *context, uint32_t microseconds);

/* UART Main blocking TX and bounded non-blocking RX binding. */
signal_result_t SignalMSPM0G3507_UART_Bind(signal_uart_t *uart,
    UART_Regs *instance, uint32_t baud_rate);
signal_result_t SignalMSPM0G3507_UART_Write(void *context,
    const uint8_t *data, size_t count);
signal_result_t SignalMSPM0G3507_UART_Read(void *context, uint8_t *data,
    size_t capacity, size_t *received);

/* TimerG binding. count means period ticks, so hardware LOAD is count - 1. */
signal_result_t SignalMSPM0G3507_Timer_Bind(signal_timer_t *timer,
    GPTIMER_Regs *instance, uint32_t clock_hz, uint32_t max_count);
signal_result_t SignalMSPM0G3507_TimerSetPeriod(void *context,
    uint32_t count);
signal_result_t SignalMSPM0G3507_TimerStart(void *context);
signal_result_t SignalMSPM0G3507_TimerStop(void *context);
signal_result_t SignalMSPM0G3507_TimerRead(void *context, uint32_t *count);

/* DMA binding. SysConfig remains responsible for channel trigger/mode. */
typedef struct {
    DMA_Regs *instance;
    uint8_t channel;
} signal_mspm0g3507_dma_context_t;

signal_result_t SignalMSPM0G3507_DMA_Bind(signal_dma_t *dma,
    signal_mspm0g3507_dma_context_t *context);
signal_result_t SignalMSPM0G3507_DMA_Configure(void *context,
    const signal_dma_transfer_t *transfer);
signal_result_t SignalMSPM0G3507_DMA_Start(void *context);
signal_result_t SignalMSPM0G3507_DMA_Stop(void *context);

/* Comparator internal 8-bit DAC threshold binding. */
typedef struct {
    COMP_Regs *instance;
    float reference_voltage_v;
} signal_mspm0g3507_comparator_context_t;

signal_result_t SignalMSPM0G3507_Comparator_Bind(
    signal_comparator_t *comparator,
    signal_mspm0g3507_comparator_context_t *context);
signal_result_t SignalMSPM0G3507_Comparator_Apply(void *context,
    const signal_comparator_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* SIGNAL_MSPM0G3507_PLATFORM_H */
