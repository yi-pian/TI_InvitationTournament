/*
 * Copyright (c) 2023, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.c =============
 *  Configured MSPM0 DriverLib module definitions
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */

#include "ti_msp_dl_config.h"

DL_SPI_backupConfig gSPI_TFTBackup;

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform any initialization needed before using any board APIs
 */
SYSCONFIG_WEAK void SYSCFG_DL_init(void)
{
    SYSCFG_DL_initPower();
    SYSCFG_DL_GPIO_init();
    /* Module-Specific Initializations*/
    SYSCFG_DL_SYSCTL_init();
    SYSCFG_DL_SIGNAL_SAMPLE_TIMER_init();
    SYSCFG_DL_SPI_TFT_init();
    SYSCFG_DL_SIGNAL_ADC_init();
    SYSCFG_DL_DMA_init();
    /* Ensure backup structures have no valid state */

	gSPI_TFTBackup.backupRdy 	= false;

}
/*
 * User should take care to save and restore register configuration in application.
 * See Retention Configuration section for more details.
 */
SYSCONFIG_WEAK bool SYSCFG_DL_saveConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_SPI_saveConfiguration(SPI_TFT_INST, &gSPI_TFTBackup);

    return retStatus;
}


SYSCONFIG_WEAK bool SYSCFG_DL_restoreConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_SPI_restoreConfiguration(SPI_TFT_INST, &gSPI_TFTBackup);

    return retStatus;
}

SYSCONFIG_WEAK void SYSCFG_DL_initPower(void)
{
    DL_GPIO_reset(GPIOA);
    DL_GPIO_reset(GPIOB);
    DL_TimerG_reset(SIGNAL_SAMPLE_TIMER_INST);
    DL_SPI_reset(SPI_TFT_INST);
    DL_ADC12_reset(SIGNAL_ADC_INST);


    DL_GPIO_enablePower(GPIOA);
    DL_GPIO_enablePower(GPIOB);
    DL_TimerG_enablePower(SIGNAL_SAMPLE_TIMER_INST);
    DL_SPI_enablePower(SPI_TFT_INST);
    DL_ADC12_enablePower(SIGNAL_ADC_INST);

    delay_cycles(POWER_STARTUP_DELAY);
}

SYSCONFIG_WEAK void SYSCFG_DL_GPIO_init(void)
{

    DL_GPIO_initPeripheralOutputFunction(
        GPIO_SPI_TFT_IOMUX_SCLK, GPIO_SPI_TFT_IOMUX_SCLK_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_SPI_TFT_IOMUX_PICO, GPIO_SPI_TFT_IOMUX_PICO_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_SPI_TFT_IOMUX_POCI, GPIO_SPI_TFT_IOMUX_POCI_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_SPI_TFT_IOMUX_CS0, GPIO_SPI_TFT_IOMUX_CS0_FUNC);

    DL_GPIO_initDigitalOutput(GPIO_TFT_CTRL_TFT_DC_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_TFT_CTRL_TFT_BLK_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_KEYPAD_KEYPAD_R1_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_KEYPAD_KEYPAD_R2_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_KEYPAD_KEYPAD_R3_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_KEYPAD_KEYPAD_R4_IOMUX);

    DL_GPIO_initDigitalInputFeatures(GPIO_KEYPAD_KEYPAD_C1_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(GPIO_KEYPAD_KEYPAD_C2_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(GPIO_KEYPAD_KEYPAD_C3_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(GPIO_KEYPAD_KEYPAD_C4_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_clearPins(GPIOB, GPIO_TFT_CTRL_TFT_DC_PIN);
    DL_GPIO_setPins(GPIOB, GPIO_TFT_CTRL_TFT_BLK_PIN |
		GPIO_KEYPAD_KEYPAD_R1_PIN |
		GPIO_KEYPAD_KEYPAD_R2_PIN |
		GPIO_KEYPAD_KEYPAD_R3_PIN |
		GPIO_KEYPAD_KEYPAD_R4_PIN);
    DL_GPIO_enableOutput(GPIOB, GPIO_TFT_CTRL_TFT_DC_PIN |
		GPIO_TFT_CTRL_TFT_BLK_PIN |
		GPIO_KEYPAD_KEYPAD_R1_PIN |
		GPIO_KEYPAD_KEYPAD_R2_PIN |
		GPIO_KEYPAD_KEYPAD_R3_PIN |
		GPIO_KEYPAD_KEYPAD_R4_PIN);

}



SYSCONFIG_WEAK void SYSCFG_DL_SYSCTL_init(void)
{

	//Low Power Mode is configured to be SLEEP0
    DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);

    
	DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);
	/* Set default configuration */
	DL_SYSCTL_disableHFXT();
	DL_SYSCTL_disableSYSPLL();

}



/*
 * Timer clock configuration to be sourced by BUSCLK /  (32000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   32000000 Hz = 32000000 Hz / (1 * (0 + 1))
 */
static const DL_TimerG_ClockConfig gSIGNAL_SAMPLE_TIMERClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale    = 0U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * SIGNAL_SAMPLE_TIMER_INST_LOAD_VALUE = (2us * 32000000 Hz) - 1
 */
static const DL_TimerG_TimerConfig gSIGNAL_SAMPLE_TIMERTimerConfig = {
    .period     = SIGNAL_SAMPLE_TIMER_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC,
    .startTimer = DL_TIMER_STOP,
};

SYSCONFIG_WEAK void SYSCFG_DL_SIGNAL_SAMPLE_TIMER_init(void) {

    DL_TimerG_setClockConfig(SIGNAL_SAMPLE_TIMER_INST,
        (DL_TimerG_ClockConfig *) &gSIGNAL_SAMPLE_TIMERClockConfig);

    DL_TimerG_initTimerMode(SIGNAL_SAMPLE_TIMER_INST,
        (DL_TimerG_TimerConfig *) &gSIGNAL_SAMPLE_TIMERTimerConfig);
    DL_TimerG_enableClock(SIGNAL_SAMPLE_TIMER_INST);


    DL_TimerG_enableEvent(SIGNAL_SAMPLE_TIMER_INST, DL_TIMERG_EVENT_ROUTE_1, (DL_TIMERG_EVENT_ZERO_EVENT));

    DL_TimerG_setPublisherChanID(SIGNAL_SAMPLE_TIMER_INST, DL_TIMERG_PUBLISHER_INDEX_0, SIGNAL_SAMPLE_TIMER_INST_PUB_0_CH);



}


static const DL_SPI_Config gSPI_TFT_config = {
    .mode        = DL_SPI_MODE_CONTROLLER,
    .frameFormat = DL_SPI_FRAME_FORMAT_MOTO4_POL0_PHA0,
    .parity      = DL_SPI_PARITY_NONE,
    .dataSize    = DL_SPI_DATA_SIZE_8,
    .bitOrder    = DL_SPI_BIT_ORDER_MSB_FIRST,
    .chipSelectPin = DL_SPI_CHIP_SELECT_0,
};

static const DL_SPI_ClockConfig gSPI_TFT_clockConfig = {
    .clockSel    = DL_SPI_CLOCK_BUSCLK,
    .divideRatio = DL_SPI_CLOCK_DIVIDE_RATIO_1
};

SYSCONFIG_WEAK void SYSCFG_DL_SPI_TFT_init(void) {
    DL_SPI_setClockConfig(SPI_TFT_INST, (DL_SPI_ClockConfig *) &gSPI_TFT_clockConfig);

    DL_SPI_init(SPI_TFT_INST, (DL_SPI_Config *) &gSPI_TFT_config);

    /* Configure Controller mode */
    /*
     * Set the bit rate clock divider to generate the serial output clock
     *     outputBitRate = (spiInputClock) / ((1 + SCR) * 2)
     *     8000000 = (32000000)/((1 + 1) * 2)
     */
    DL_SPI_setBitRateSerialClockDivider(SPI_TFT_INST, 1);
    /* Set RX and TX FIFO threshold levels */
    DL_SPI_setFIFOThreshold(SPI_TFT_INST, DL_SPI_RX_FIFO_LEVEL_1_2_FULL, DL_SPI_TX_FIFO_LEVEL_1_2_EMPTY);

    /* Enable module */
    DL_SPI_enable(SPI_TFT_INST);
}

/* SIGNAL_ADC Initialization */
static const DL_ADC12_ClockConfig gSIGNAL_ADCClockConfig = {
    .clockSel       = DL_ADC12_CLOCK_ULPCLK,
    .divideRatio    = DL_ADC12_CLOCK_DIVIDE_1,
    .freqRange      = DL_ADC12_CLOCK_FREQ_RANGE_24_TO_32,
};
SYSCONFIG_WEAK void SYSCFG_DL_SIGNAL_ADC_init(void)
{
    DL_ADC12_setClockConfig(SIGNAL_ADC_INST, (DL_ADC12_ClockConfig *) &gSIGNAL_ADCClockConfig);
    DL_ADC12_initSingleSample(SIGNAL_ADC_INST,
        DL_ADC12_REPEAT_MODE_ENABLED, DL_ADC12_SAMPLING_SOURCE_AUTO, DL_ADC12_TRIG_SRC_EVENT,
        DL_ADC12_SAMP_CONV_RES_12_BIT, DL_ADC12_SAMP_CONV_DATA_FORMAT_UNSIGNED);
    DL_ADC12_configConversionMem(SIGNAL_ADC_INST, SIGNAL_ADC_ADCMEM_0,
        DL_ADC12_INPUT_CHAN_2, DL_ADC12_REFERENCE_VOLTAGE_VDDA, DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0, DL_ADC12_AVERAGING_MODE_DISABLED,
        DL_ADC12_BURN_OUT_SOURCE_DISABLED, DL_ADC12_TRIGGER_MODE_TRIGGER_NEXT, DL_ADC12_WINDOWS_COMP_MODE_DISABLED);
    DL_ADC12_setPowerDownMode(SIGNAL_ADC_INST,DL_ADC12_POWER_DOWN_MODE_MANUAL);
    DL_ADC12_setSampleTime0(SIGNAL_ADC_INST,2);
    DL_ADC12_enableDMA(SIGNAL_ADC_INST);
    DL_ADC12_setDMASamplesCnt(SIGNAL_ADC_INST,1);
    DL_ADC12_enableDMATrigger(SIGNAL_ADC_INST,(DL_ADC12_DMA_MEM0_RESULT_LOADED));
    DL_ADC12_setSubscriberChanID(SIGNAL_ADC_INST,SIGNAL_ADC_INST_SUB_CH);
    /* Enable ADC12 interrupt */
    DL_ADC12_clearInterruptStatus(SIGNAL_ADC_INST,(DL_ADC12_INTERRUPT_DMA_DONE));
    DL_ADC12_enableInterrupt(SIGNAL_ADC_INST,(DL_ADC12_INTERRUPT_DMA_DONE));
    DL_ADC12_enableConversions(SIGNAL_ADC_INST);
}

static const DL_DMA_Config gSIGNAL_ADC_DMAConfig = {
    .transferMode   = DL_DMA_SINGLE_TRANSFER_MODE,
    .extendedMode   = DL_DMA_NORMAL_MODE,
    .destIncrement  = DL_DMA_ADDR_INCREMENT,
    .srcIncrement   = DL_DMA_ADDR_UNCHANGED,
    .destWidth      = DL_DMA_WIDTH_HALF_WORD,
    .srcWidth       = DL_DMA_WIDTH_HALF_WORD,
    .trigger        = SIGNAL_ADC_INST_DMA_TRIGGER,
    .triggerType    = DL_DMA_TRIGGER_TYPE_EXTERNAL,
};

SYSCONFIG_WEAK void SYSCFG_DL_SIGNAL_ADC_DMA_init(void)
{
    DL_DMA_initChannel(DMA, SIGNAL_ADC_DMA_CHAN_ID , (DL_DMA_Config *) &gSIGNAL_ADC_DMAConfig);
}
SYSCONFIG_WEAK void SYSCFG_DL_DMA_init(void){
    SYSCFG_DL_SIGNAL_ADC_DMA_init();
}


