#include "mspm0_blocking_bus.h"

#ifndef MSPM0_EXT_BUS_POLL_LIMIT
#define MSPM0_EXT_BUS_POLL_LIMIT (1000000UL)
#endif

static bool i2c_has_error(const I2C_Regs *i2c)
{
    return (DL_I2C_getControllerStatus(i2c) &
               DL_I2C_CONTROLLER_STATUS_ERROR) != 0U;
}

static bool i2c_wait_idle(I2C_Regs *i2c)
{
    uint32_t remaining = MSPM0_EXT_BUS_POLL_LIMIT;

    while ((DL_I2C_getControllerStatus(i2c) &
               DL_I2C_CONTROLLER_STATUS_IDLE) == 0U) {
        if (i2c_has_error(i2c) || (--remaining == 0U)) {
            return false;
        }
    }
    return true;
}

static bool i2c_wait_raw(I2C_Regs *i2c, uint32_t mask)
{
    uint32_t remaining = MSPM0_EXT_BUS_POLL_LIMIT;

    while ((DL_I2C_getRawInterruptStatus(i2c, mask) & mask) == 0U) {
        if (i2c_has_error(i2c) || (--remaining == 0U)) {
            return false;
        }
    }
    return true;
}

bool MSPM0_EXT_SPI_Transfer8(SPI_Regs *spi,
    GPIO_Regs *cs_port,
    uint32_t cs_pin,
    const uint8_t *tx,
    uint8_t *rx,
    size_t count)
{
    size_t i;

    if ((spi == NULL) || (cs_port == NULL) || (cs_pin == 0U) ||
        (count == 0U)) {
        return false;
    }

    while (!DL_SPI_isRXFIFOEmpty(spi)) {
        (void) DL_SPI_receiveData8(spi);
    }

    DL_GPIO_clearPins(cs_port, cs_pin);
    for (i = 0U; i < count; ++i) {
        const uint8_t outgoing = (tx != NULL) ? tx[i] : 0U;
        DL_SPI_transmitDataBlocking8(spi, outgoing);
        if (rx != NULL) {
            rx[i] = DL_SPI_receiveDataBlocking8(spi);
        } else {
            (void) DL_SPI_receiveDataBlocking8(spi);
        }
    }
    DL_GPIO_setPins(cs_port, cs_pin);
    return true;
}

bool MSPM0_EXT_SPI_Write16MSB(SPI_Regs *spi,
    GPIO_Regs *cs_port,
    uint32_t cs_pin,
    uint16_t word)
{
    const uint8_t bytes[2] = {
        (uint8_t) (word >> 8U),
        (uint8_t) word
    };

    return MSPM0_EXT_SPI_Transfer8(
        spi, cs_port, cs_pin, bytes, NULL, sizeof(bytes));
}

bool MSPM0_EXT_I2C_Write(I2C_Regs *i2c,
    uint8_t address_7bit,
    const uint8_t *data,
    uint16_t count)
{
    const uint32_t done = DL_I2C_INTERRUPT_CONTROLLER_TX_DONE;

    if ((i2c == NULL) || (data == NULL) || (count == 0U) ||
        (count > 8U) || (address_7bit > 0x7FU) || !i2c_wait_idle(i2c)) {
        return false;
    }

    DL_I2C_flushControllerTXFIFO(i2c);
    DL_I2C_clearInterruptStatus(i2c, done);
    if (DL_I2C_fillControllerTXFIFO(i2c, data, count) != count) {
        return false;
    }
    DL_I2C_startControllerTransfer(i2c, address_7bit,
        DL_I2C_CONTROLLER_DIRECTION_TX, count);
    if (!i2c_wait_raw(i2c, done)) {
        return false;
    }
    DL_I2C_clearInterruptStatus(i2c, done);
    return !i2c_has_error(i2c) && i2c_wait_idle(i2c);
}

bool MSPM0_EXT_I2C_Read(I2C_Regs *i2c,
    uint8_t address_7bit,
    uint8_t *data,
    uint16_t count)
{
    uint16_t i;
    uint32_t remaining;
    const uint32_t done = DL_I2C_INTERRUPT_CONTROLLER_RX_DONE;

    if ((i2c == NULL) || (data == NULL) || (count == 0U) ||
        (address_7bit > 0x7FU) || !i2c_wait_idle(i2c)) {
        return false;
    }

    DL_I2C_flushControllerRXFIFO(i2c);
    DL_I2C_clearInterruptStatus(i2c, done);
    DL_I2C_startControllerTransfer(i2c, address_7bit,
        DL_I2C_CONTROLLER_DIRECTION_RX, count);

    for (i = 0U; i < count; ++i) {
        remaining = MSPM0_EXT_BUS_POLL_LIMIT;
        while (DL_I2C_isControllerRXFIFOEmpty(i2c)) {
            if (i2c_has_error(i2c) || (--remaining == 0U)) {
                return false;
            }
        }
        data[i] = DL_I2C_receiveControllerData(i2c);
    }

    if (!i2c_wait_raw(i2c, done)) {
        return false;
    }
    DL_I2C_clearInterruptStatus(i2c, done);
    return !i2c_has_error(i2c) && i2c_wait_idle(i2c);
}

bool MSPM0_EXT_I2C_WriteRead(I2C_Regs *i2c,
    uint8_t address_7bit,
    const uint8_t *tx,
    uint16_t tx_count,
    uint8_t *rx,
    uint16_t rx_count)
{
    uint16_t i;
    uint32_t remaining;
    const uint32_t tx_done = DL_I2C_INTERRUPT_CONTROLLER_TX_DONE;
    const uint32_t rx_done = DL_I2C_INTERRUPT_CONTROLLER_RX_DONE;

    if ((i2c == NULL) || (tx == NULL) || (rx == NULL) ||
        (tx_count == 0U) || (tx_count > 8U) || (rx_count == 0U) ||
        (address_7bit > 0x7FU) || !i2c_wait_idle(i2c)) {
        return false;
    }

    DL_I2C_flushControllerTXFIFO(i2c);
    DL_I2C_flushControllerRXFIFO(i2c);
    DL_I2C_clearInterruptStatus(i2c, tx_done | rx_done);
    if (DL_I2C_fillControllerTXFIFO(i2c, tx, tx_count) != tx_count) {
        return false;
    }

    DL_I2C_startControllerTransferAdvanced(i2c, address_7bit,
        DL_I2C_CONTROLLER_DIRECTION_TX, tx_count,
        DL_I2C_CONTROLLER_START_ENABLE, DL_I2C_CONTROLLER_STOP_DISABLE,
        DL_I2C_CONTROLLER_ACK_DISABLE);
    if (!i2c_wait_raw(i2c, tx_done)) {
        return false;
    }
    DL_I2C_clearInterruptStatus(i2c, tx_done);

    DL_I2C_startControllerTransferAdvanced(i2c, address_7bit,
        DL_I2C_CONTROLLER_DIRECTION_RX, rx_count,
        DL_I2C_CONTROLLER_START_ENABLE, DL_I2C_CONTROLLER_STOP_ENABLE,
        DL_I2C_CONTROLLER_ACK_DISABLE);

    for (i = 0U; i < rx_count; ++i) {
        remaining = MSPM0_EXT_BUS_POLL_LIMIT;
        while (DL_I2C_isControllerRXFIFOEmpty(i2c)) {
            if (i2c_has_error(i2c) || (--remaining == 0U)) {
                return false;
            }
        }
        rx[i] = DL_I2C_receiveControllerData(i2c);
    }

    if (!i2c_wait_raw(i2c, rx_done)) {
        return false;
    }
    DL_I2C_clearInterruptStatus(i2c, rx_done);
    return !i2c_has_error(i2c) && i2c_wait_idle(i2c);
}
