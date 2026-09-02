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

DL_TimerG_backupConfig gSIGNAL_CAPTUREBackup;

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
    SYSCFG_DL_SIGNAL_CAPTURE_init();
    SYSCFG_DL_SIGNAL_UART_init();
    SYSCFG_DL_SIGNAL_COMP_init();
    /* Ensure backup structures have no valid state */
	gSIGNAL_CAPTUREBackup.backupRdy 	= false;


}
/*
 * User should take care to save and restore register configuration in application.
 * See Retention Configuration section for more details.
 */
SYSCONFIG_WEAK bool SYSCFG_DL_saveConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerG_saveConfiguration(SIGNAL_CAPTURE_INST, &gSIGNAL_CAPTUREBackup);

    return retStatus;
}


SYSCONFIG_WEAK bool SYSCFG_DL_restoreConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerG_restoreConfiguration(SIGNAL_CAPTURE_INST, &gSIGNAL_CAPTUREBackup, false);

    return retStatus;
}

SYSCONFIG_WEAK void SYSCFG_DL_initPower(void)
{
    DL_GPIO_reset(GPIOA);
    DL_GPIO_reset(GPIOB);
    DL_TimerG_reset(SIGNAL_CAPTURE_INST);
    DL_UART_Main_reset(SIGNAL_UART_INST);
    DL_COMP_reset(SIGNAL_COMP_INST);

    DL_GPIO_enablePower(GPIOA);
    DL_GPIO_enablePower(GPIOB);
    DL_TimerG_enablePower(SIGNAL_CAPTURE_INST);
    DL_UART_Main_enablePower(SIGNAL_UART_INST);
    DL_COMP_enablePower(SIGNAL_COMP_INST);
    delay_cycles(POWER_STARTUP_DELAY);
}

SYSCONFIG_WEAK void SYSCFG_DL_GPIO_init(void)
{

    DL_GPIO_initPeripheralOutputFunction(
        GPIO_SIGNAL_UART_IOMUX_TX, GPIO_SIGNAL_UART_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_SIGNAL_UART_IOMUX_RX, GPIO_SIGNAL_UART_IOMUX_RX_FUNC);

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
static const DL_TimerG_ClockConfig gSIGNAL_CAPTUREClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale = 0U
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * SIGNAL_CAPTURE_INST_LOAD_VALUE = (2ms * 32000000 Hz) - 1
 */
static const DL_TimerG_CaptureTriggerConfig gSIGNAL_CAPTURECaptureConfig = {
    .captureMode    = DL_TIMER_CAPTURE_MODE_EDGE_TIME,
    .period         = SIGNAL_CAPTURE_INST_LOAD_VALUE,
    .startTimer     = DL_TIMER_STOP,
};

SYSCONFIG_WEAK void SYSCFG_DL_SIGNAL_CAPTURE_init(void) {

    DL_TimerG_setClockConfig(SIGNAL_CAPTURE_INST,
        (DL_TimerG_ClockConfig *) &gSIGNAL_CAPTUREClockConfig);

    DL_TimerG_initCaptureTriggerMode(SIGNAL_CAPTURE_INST,
        (DL_TimerG_CaptureTriggerConfig *) &gSIGNAL_CAPTURECaptureConfig);
    DL_TimerG_enableInterrupt(SIGNAL_CAPTURE_INST , DL_TIMERG_INTERRUPT_CC0_DN_EVENT |
		DL_TIMERG_INTERRUPT_ZERO_EVENT);

    DL_TimerG_enableClock(SIGNAL_CAPTURE_INST);

    DL_TimerG_setExternalTriggerEvent(SIGNAL_CAPTURE_INST,
        DL_TIMER_EXT_TRIG_SEL_TRIG_SUB_0);

    DL_TimerG_enableExternalTrigger(SIGNAL_CAPTURE_INST);

    DL_TimerG_setSubscriberChanID(SIGNAL_CAPTURE_INST,
        DL_TIMER_SUBSCRIBER_INDEX_0, SIGNAL_CAPTURE_INST_SUB_0_CH);
}

static const DL_UART_Main_ClockConfig gSIGNAL_UARTClockConfig = {
    .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gSIGNAL_UARTConfig = {
    .mode        = DL_UART_MAIN_MODE_NORMAL,
    .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity      = DL_UART_MAIN_PARITY_NONE,
    .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
};

SYSCONFIG_WEAK void SYSCFG_DL_SIGNAL_UART_init(void)
{
    DL_UART_Main_setClockConfig(SIGNAL_UART_INST, (DL_UART_Main_ClockConfig *) &gSIGNAL_UARTClockConfig);

    DL_UART_Main_init(SIGNAL_UART_INST, (DL_UART_Main_Config *) &gSIGNAL_UARTConfig);
    /*
     * Configure baud rate by setting oversampling and baud rate divisors.
     *  Target baud rate: 115200
     *  Actual baud rate: 115211.52
     */
    DL_UART_Main_setOversampling(SIGNAL_UART_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(SIGNAL_UART_INST, SIGNAL_UART_IBRD_32_MHZ_115200_BAUD, SIGNAL_UART_FBRD_32_MHZ_115200_BAUD);


    /* Configure FIFOs */
    DL_UART_Main_enableFIFOs(SIGNAL_UART_INST);
    DL_UART_Main_setRXFIFOThreshold(SIGNAL_UART_INST, DL_UART_RX_FIFO_LEVEL_ONE_ENTRY);
    DL_UART_Main_setTXFIFOThreshold(SIGNAL_UART_INST, DL_UART_TX_FIFO_LEVEL_1_2_EMPTY);

    DL_UART_Main_enableLoopbackMode(SIGNAL_UART_INST);

    DL_UART_Main_enable(SIGNAL_UART_INST);
}

/* SIGNAL_COMP Initialization */
static const DL_COMP_Config gSIGNAL_COMPConfig = {
    .channelEnable = DL_COMP_ENABLE_CHANNEL_NEG,
    .mode          = DL_COMP_MODE_ULP,
    .negChannel    = DL_COMP_IMSEL_CHANNEL_0,
    .posChannel    = DL_COMP_IPSEL_CHANNEL_0,
    .hysteresis    = DL_COMP_HYSTERESIS_30,
    .polarity      = DL_COMP_POLARITY_NON_INV
};
static const DL_COMP_RefVoltageConfig gSIGNAL_COMPVRefConfig = {
    .mode           = DL_COMP_REF_MODE_STATIC,
    .source         = DL_COMP_REF_SOURCE_VDDA_DAC,
    .terminalSelect = DL_COMP_REF_TERMINAL_SELECT_POS,
    .controlSelect  = DL_COMP_DAC_CONTROL_SW,
    .inputSelect    = DL_COMP_DAC_INPUT_DACCODE0
};

SYSCONFIG_WEAK void SYSCFG_DL_SIGNAL_COMP_init(void)
{
    DL_COMP_init(SIGNAL_COMP_INST, (DL_COMP_Config *) &gSIGNAL_COMPConfig);
    DL_COMP_refVoltageInit(SIGNAL_COMP_INST, (DL_COMP_RefVoltageConfig *) &gSIGNAL_COMPVRefConfig);
    DL_COMP_setDACCode0(SIGNAL_COMP_INST, SIGNAL_COMP_DACCODE0);
    DL_COMP_enableEvent(SIGNAL_COMP_INST, (DL_COMP_EVENT_OUTPUT_EDGE));
    DL_COMP_setPublisherChanID(SIGNAL_COMP_INST, SIGNAL_COMP_INST_PUB_CH);

    DL_COMP_enable(SIGNAL_COMP_INST);

}


