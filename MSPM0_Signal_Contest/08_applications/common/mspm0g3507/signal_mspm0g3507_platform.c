#include "signal_mspm0g3507_platform.h"

#include <limits.h>

signal_result_t SignalMSPM0G3507_ADC_Bind(
    signal_adc_t *adc,
    signal_mspm0g3507_adc_context_t *context,
    uint8_t channel,
    uint8_t resolution_bits,
    float reference_voltage_v,
    uint32_t clock_hz)
{
    signal_adc_config_t config;
    if ((adc == NULL) || (context == NULL) || (context->instance == NULL) ||
        (context->result_interrupt_mask == 0U) ||
        (context->timeout_iterations == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    config.channel = channel;
    config.resolution_bits = resolution_bits;
    config.reference_voltage_v = reference_voltage_v;
    config.clock_hz = clock_hz;
    if (SignalADC_ValidateConfig(&config) != SIGNAL_RESULT_OK) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    adc->context = context;
    adc->read = SignalMSPM0G3507_ADC_Read;
    adc->enable = SignalMSPM0G3507_ADC_Enable;
    adc->disable = SignalMSPM0G3507_ADC_Disable;
    adc->config = config;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_ADC_Read(void *context, uint16_t *raw)
{
    signal_mspm0g3507_adc_context_t *adc_context =
        (signal_mspm0g3507_adc_context_t *) context;
    uint32_t remaining;
    if ((adc_context == NULL) || (adc_context->instance == NULL) ||
        (raw == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    DL_ADC12_clearInterruptStatus(adc_context->instance,
        adc_context->result_interrupt_mask);
    if (!DL_ADC12_isConversionsEnabled(adc_context->instance)) {
        DL_ADC12_enableConversions(adc_context->instance);
    }
    DL_ADC12_startConversion(adc_context->instance);
    remaining = adc_context->timeout_iterations;
    while (DL_ADC12_getRawInterruptStatus(adc_context->instance,
               adc_context->result_interrupt_mask) == 0U) {
        if (remaining == 0U) {
            DL_ADC12_stopConversion(adc_context->instance);
            return SIGNAL_RESULT_HARDWARE_ERROR;
        }
        --remaining;
    }
    *raw = DL_ADC12_getMemResult(adc_context->instance,
        adc_context->memory_index);
    DL_ADC12_clearInterruptStatus(adc_context->instance,
        adc_context->result_interrupt_mask);
    /* MSPM0 single-conversion mode clears ENC; arm the next call. */
    DL_ADC12_enableConversions(adc_context->instance);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_ADC_Enable(void *context)
{
    signal_mspm0g3507_adc_context_t *adc_context =
        (signal_mspm0g3507_adc_context_t *) context;
    if ((adc_context == NULL) || (adc_context->instance == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    DL_ADC12_enableConversions(adc_context->instance);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_ADC_Disable(void *context)
{
    signal_mspm0g3507_adc_context_t *adc_context =
        (signal_mspm0g3507_adc_context_t *) context;
    if ((adc_context == NULL) || (adc_context->instance == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    DL_ADC12_stopConversion(adc_context->instance);
    DL_ADC12_disableConversions(adc_context->instance);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_DAC_Bind(signal_dac_t *dac,
    DAC12_Regs *instance, float reference_voltage_v)
{
    if ((dac == NULL) || (instance == NULL) ||
        !(reference_voltage_v > 0.0f)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    dac->context = instance;
    dac->write = SignalMSPM0G3507_DAC_Write;
    dac->resolution_bits = 12U;
    dac->reference_voltage_v = reference_voltage_v;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_DAC_Write(void *context, uint16_t raw)
{
    DAC12_Regs *instance = (DAC12_Regs *) context;
    if (instance == NULL) return SIGNAL_RESULT_INVALID_ARGUMENT;
    if (raw > UINT16_C(4095)) return SIGNAL_RESULT_OUT_OF_RANGE;
    DL_DAC12_output12(instance, raw);
    if (!DL_DAC12_isEnabled(instance)) DL_DAC12_enable(instance);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_GPIO_Bind(signal_gpio_port_t *port,
    GPIO_Regs *instance)
{
    if ((port == NULL) || (instance == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    port->context = instance;
    port->write = SignalMSPM0G3507_GPIO_Write;
    port->read = SignalMSPM0G3507_GPIO_Read;
    port->toggle = SignalMSPM0G3507_GPIO_Toggle;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_GPIO_Write(void *context, uint32_t pin,
    bool high)
{
    GPIO_Regs *port = (GPIO_Regs *) context;
    if ((port == NULL) || (pin == 0U)) return SIGNAL_RESULT_INVALID_ARGUMENT;
    if (high) DL_GPIO_setPins(port, pin);
    else DL_GPIO_clearPins(port, pin);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_GPIO_Read(void *context, uint32_t pin,
    bool *high)
{
    GPIO_Regs *port = (GPIO_Regs *) context;
    if ((port == NULL) || (pin == 0U) || (high == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    *high = (DL_GPIO_readPins(port, pin) & pin) != 0U;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_GPIO_Toggle(void *context, uint32_t pin)
{
    GPIO_Regs *port = (GPIO_Regs *) context;
    if ((port == NULL) || (pin == 0U)) return SIGNAL_RESULT_INVALID_ARGUMENT;
    DL_GPIO_togglePins(port, pin);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_GPIO_ReadActive(void *context, bool *active)
{
    signal_mspm0g3507_gpio_input_t *input =
        (signal_mspm0g3507_gpio_input_t *) context;
    bool high;
    if ((input == NULL) || (input->port == NULL) || (active == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    high = (DL_GPIO_readPins(input->port, input->pin) & input->pin) != 0U;
    *active = input->active_low ? !high : high;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_KeypadDriveRow(void *context, uint8_t row,
    bool active)
{
    signal_mspm0g3507_keypad_context_t *keypad =
        (signal_mspm0g3507_keypad_context_t *) context;
    signal_mspm0g3507_gpio_pin_t *pin;
    if ((keypad == NULL) || (row >= SIGNAL_MATRIX_KEYPAD_4X4_ROWS)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    pin = &keypad->rows[row];
    if ((pin->port == NULL) || (pin->pin == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    /* Recommended matrix wiring is active-low. */
    if (active) DL_GPIO_clearPins(pin->port, pin->pin);
    else DL_GPIO_setPins(pin->port, pin->pin);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_KeypadReadColumn(void *context,
    uint8_t column, bool *active)
{
    signal_mspm0g3507_keypad_context_t *keypad =
        (signal_mspm0g3507_keypad_context_t *) context;
    signal_mspm0g3507_gpio_pin_t *pin;
    if ((keypad == NULL) || (active == NULL) ||
        (column >= SIGNAL_MATRIX_KEYPAD_4X4_COLUMNS)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    pin = &keypad->columns[column];
    if ((pin->port == NULL) || (pin->pin == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    *active = (DL_GPIO_readPins(pin->port, pin->pin) & pin->pin) == 0U;
    return SIGNAL_RESULT_OK;
}

void SignalMSPM0G3507_DelayUs(void *context, uint32_t microseconds)
{
    signal_mspm0g3507_keypad_context_t *keypad =
        (signal_mspm0g3507_keypad_context_t *) context;
    if ((keypad == NULL) || (keypad->cpu_clock_hz < 1000000U)) return;
    while (microseconds != 0U) {
        delay_cycles(keypad->cpu_clock_hz / 1000000U);
        --microseconds;
    }
}

signal_result_t SignalMSPM0G3507_UART_Bind(signal_uart_t *uart,
    UART_Regs *instance, uint32_t baud_rate)
{
    if ((uart == NULL) || (instance == NULL) || (baud_rate == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    uart->context = instance;
    uart->write = SignalMSPM0G3507_UART_Write;
    uart->read = SignalMSPM0G3507_UART_Read;
    uart->baud_rate = baud_rate;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_UART_Write(void *context,
    const uint8_t *data, size_t count)
{
    UART_Regs *uart = (UART_Regs *) context;
    size_t index;
    if ((uart == NULL) || ((data == NULL) && (count != 0U))) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    for (index = 0U; index < count; ++index) {
        DL_UART_Main_transmitDataBlocking(uart, data[index]);
    }
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_UART_Read(void *context, uint8_t *data,
    size_t capacity, size_t *received)
{
    UART_Regs *uart = (UART_Regs *) context;
    size_t count = 0U;
    if ((uart == NULL) || (data == NULL) || (capacity == 0U) ||
        (received == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    while ((count < capacity) &&
           DL_UART_Main_receiveDataCheck(uart, &data[count])) {
        ++count;
    }
    *received = count;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_Timer_Bind(signal_timer_t *timer,
    GPTIMER_Regs *instance, uint32_t clock_hz, uint32_t max_count)
{
    if ((timer == NULL) || (instance == NULL) || (clock_hz == 0U) ||
        (max_count == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    timer->context = instance;
    timer->set_period_count = SignalMSPM0G3507_TimerSetPeriod;
    timer->start = SignalMSPM0G3507_TimerStart;
    timer->stop = SignalMSPM0G3507_TimerStop;
    timer->read_count = SignalMSPM0G3507_TimerRead;
    timer->clock_hz = clock_hz;
    timer->max_count = max_count;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_TimerSetPeriod(void *context,
    uint32_t count)
{
    GPTIMER_Regs *timer = (GPTIMER_Regs *) context;
    if ((timer == NULL) || (count == 0U)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    DL_TimerG_stopCounter(timer);
    DL_TimerG_setLoadValue(timer, count - 1U);
    DL_TimerG_setTimerCount(timer, count - 1U);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_TimerStart(void *context)
{
    GPTIMER_Regs *timer = (GPTIMER_Regs *) context;
    if (timer == NULL) return SIGNAL_RESULT_INVALID_ARGUMENT;
    DL_TimerG_startCounter(timer);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_TimerStop(void *context)
{
    GPTIMER_Regs *timer = (GPTIMER_Regs *) context;
    if (timer == NULL) return SIGNAL_RESULT_INVALID_ARGUMENT;
    DL_TimerG_stopCounter(timer);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_TimerRead(void *context, uint32_t *count)
{
    GPTIMER_Regs *timer = (GPTIMER_Regs *) context;
    if ((timer == NULL) || (count == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    *count = DL_TimerG_getTimerCount(timer);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_DMA_Bind(signal_dma_t *dma,
    signal_mspm0g3507_dma_context_t *context)
{
    if ((dma == NULL) || (context == NULL) || (context->instance == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    dma->context = context;
    dma->configure = SignalMSPM0G3507_DMA_Configure;
    dma->start = SignalMSPM0G3507_DMA_Start;
    dma->stop = SignalMSPM0G3507_DMA_Stop;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_DMA_Configure(void *context,
    const signal_dma_transfer_t *transfer)
{
    signal_mspm0g3507_dma_context_t *dma_context =
        (signal_mspm0g3507_dma_context_t *) context;
    DL_DMA_WIDTH width;
    DL_DMA_INCREMENT source_increment;
    DL_DMA_INCREMENT destination_increment;
    if ((dma_context == NULL) || (dma_context->instance == NULL) ||
        (SignalDMA_ValidateTransfer(transfer) != SIGNAL_RESULT_OK)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    if (transfer->transfer_count > UINT16_MAX) {
        return SIGNAL_RESULT_OUT_OF_RANGE;
    }
    if (transfer->width == SIGNAL_DMA_WIDTH_8) width = DL_DMA_WIDTH_BYTE;
    else if (transfer->width == SIGNAL_DMA_WIDTH_16) {
        width = DL_DMA_WIDTH_HALF_WORD;
    } else if (transfer->width == SIGNAL_DMA_WIDTH_32) {
        width = DL_DMA_WIDTH_WORD;
    } else return SIGNAL_RESULT_OUT_OF_RANGE;

    if (transfer->source_increment == 0) {
        source_increment = DL_DMA_ADDR_UNCHANGED;
    } else if (transfer->source_increment == 1) {
        source_increment = DL_DMA_ADDR_INCREMENT;
    } else if (transfer->source_increment == -1) {
        source_increment = DL_DMA_ADDR_DECREMENT;
    } else return SIGNAL_RESULT_OUT_OF_RANGE;
    if (transfer->destination_increment == 0) {
        destination_increment = DL_DMA_ADDR_UNCHANGED;
    } else if (transfer->destination_increment == 1) {
        destination_increment = DL_DMA_ADDR_INCREMENT;
    } else if (transfer->destination_increment == -1) {
        destination_increment = DL_DMA_ADDR_DECREMENT;
    } else return SIGNAL_RESULT_OUT_OF_RANGE;

    DL_DMA_disableChannel(dma_context->instance, dma_context->channel);
    DL_DMA_setSrcAddr(dma_context->instance, dma_context->channel,
        (uint32_t) transfer->source);
    DL_DMA_setDestAddr(dma_context->instance, dma_context->channel,
        (uint32_t) transfer->destination);
    DL_DMA_setTransferSize(dma_context->instance, dma_context->channel,
        (uint16_t) transfer->transfer_count);
    DL_DMA_setSrcWidth(dma_context->instance, dma_context->channel, width);
    DL_DMA_setDestWidth(dma_context->instance, dma_context->channel, width);
    DL_DMA_setSrcIncrement(dma_context->instance, dma_context->channel,
        source_increment);
    DL_DMA_setDestIncrement(dma_context->instance, dma_context->channel,
        destination_increment);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_DMA_Start(void *context)
{
    signal_mspm0g3507_dma_context_t *dma_context =
        (signal_mspm0g3507_dma_context_t *) context;
    if ((dma_context == NULL) || (dma_context->instance == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    DL_DMA_enableChannel(dma_context->instance, dma_context->channel);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_DMA_Stop(void *context)
{
    signal_mspm0g3507_dma_context_t *dma_context =
        (signal_mspm0g3507_dma_context_t *) context;
    if ((dma_context == NULL) || (dma_context->instance == NULL)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    DL_DMA_disableChannel(dma_context->instance, dma_context->channel);
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_Comparator_Bind(
    signal_comparator_t *comparator,
    signal_mspm0g3507_comparator_context_t *context)
{
    if ((comparator == NULL) || (context == NULL) ||
        (context->instance == NULL) ||
        !(context->reference_voltage_v > 0.0f)) {
        return SIGNAL_RESULT_INVALID_ARGUMENT;
    }
    comparator->context = context;
    comparator->apply = SignalMSPM0G3507_Comparator_Apply;
    return SIGNAL_RESULT_OK;
}

signal_result_t SignalMSPM0G3507_Comparator_Apply(void *context,
    const signal_comparator_config_t *config)
{
    signal_mspm0g3507_comparator_context_t *comp_context =
        (signal_mspm0g3507_comparator_context_t *) context;
    float scaled;
    uint32_t dac_code;
    DL_COMP_HYSTERESIS hysteresis;
    if ((comp_context == NULL) || (comp_context->instance == NULL) ||
        (config == NULL) || !(comp_context->reference_voltage_v > 0.0f) ||
        (config->threshold_v < 0.0f) ||
        (config->threshold_v > comp_context->reference_voltage_v) ||
        (config->hysteresis_v < 0.0f) ||
        (config->hysteresis_v > 0.030f)) {
        return SIGNAL_RESULT_OUT_OF_RANGE;
    }
    scaled = config->threshold_v * 256.0f /
        comp_context->reference_voltage_v;
    dac_code = (uint32_t) (scaled + 0.5f);
    if (dac_code > 255U) dac_code = 255U;
    if (config->hysteresis_v < 0.005f) {
        hysteresis = DL_COMP_HYSTERESIS_NONE;
    } else if (config->hysteresis_v < 0.015f) {
        hysteresis = DL_COMP_HYSTERESIS_10;
    } else if (config->hysteresis_v < 0.025f) {
        hysteresis = DL_COMP_HYSTERESIS_20;
    } else {
        hysteresis = DL_COMP_HYSTERESIS_30;
    }
    DL_COMP_setDACCode0(comp_context->instance, dac_code);
    DL_COMP_setHysteresis(comp_context->instance, hysteresis);
    DL_COMP_setOutputPolarity(comp_context->instance,
        config->invert_output ? DL_COMP_POLARITY_INV :
                                DL_COMP_POLARITY_NON_INV);
    DL_COMP_enable(comp_context->instance);
    return SIGNAL_RESULT_OK;
}
