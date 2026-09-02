/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
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
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)



#define CPUCLK_FREQ                                                     32000000



/* Defines for SIGNAL_SAMPLE_TIMER */
#define SIGNAL_SAMPLE_TIMER_INST                                         (TIMG0)
#define SIGNAL_SAMPLE_TIMER_INST_IRQHandler                        TIMG0_IRQHandler
#define SIGNAL_SAMPLE_TIMER_INST_INT_IRQN                        (TIMG0_INT_IRQn)
#define SIGNAL_SAMPLE_TIMER_INST_LOAD_VALUE                                   (63U)
#define SIGNAL_SAMPLE_TIMER_INST_PUB_0_CH                                     (1)



/* Defines for SPI_TFT */
#define SPI_TFT_INST                                                       SPI1
#define SPI_TFT_INST_IRQHandler                                 SPI1_IRQHandler
#define SPI_TFT_INST_INT_IRQN                                     SPI1_INT_IRQn
#define GPIO_SPI_TFT_PICO_PORT                                            GPIOB
#define GPIO_SPI_TFT_PICO_PIN                                     DL_GPIO_PIN_8
#define GPIO_SPI_TFT_IOMUX_PICO                                 (IOMUX_PINCM25)
#define GPIO_SPI_TFT_IOMUX_PICO_FUNC                 IOMUX_PINCM25_PF_SPI1_PICO
#define GPIO_SPI_TFT_POCI_PORT                                            GPIOA
#define GPIO_SPI_TFT_POCI_PIN                                    DL_GPIO_PIN_16
#define GPIO_SPI_TFT_IOMUX_POCI                                 (IOMUX_PINCM38)
#define GPIO_SPI_TFT_IOMUX_POCI_FUNC                 IOMUX_PINCM38_PF_SPI1_POCI
/* GPIO configuration for SPI_TFT */
#define GPIO_SPI_TFT_SCLK_PORT                                            GPIOB
#define GPIO_SPI_TFT_SCLK_PIN                                     DL_GPIO_PIN_9
#define GPIO_SPI_TFT_IOMUX_SCLK                                 (IOMUX_PINCM26)
#define GPIO_SPI_TFT_IOMUX_SCLK_FUNC                 IOMUX_PINCM26_PF_SPI1_SCLK
#define GPIO_SPI_TFT_CS0_PORT                                             GPIOB
#define GPIO_SPI_TFT_CS0_PIN                                      DL_GPIO_PIN_6
#define GPIO_SPI_TFT_IOMUX_CS0                                  (IOMUX_PINCM23)
#define GPIO_SPI_TFT_IOMUX_CS0_FUNC                   IOMUX_PINCM23_PF_SPI1_CS0



/* Defines for SIGNAL_ADC */
#define SIGNAL_ADC_INST                                                     ADC0
#define SIGNAL_ADC_INST_IRQHandler                               ADC0_IRQHandler
#define SIGNAL_ADC_INST_INT_IRQN                                 (ADC0_INT_IRQn)
#define SIGNAL_ADC_ADCMEM_0                                   DL_ADC12_MEM_IDX_0
#define SIGNAL_ADC_ADCMEM_0_REF                  DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define SIGNAL_ADC_ADCMEM_0_REF_VOLTAGE_V                                     3.3
#define SIGNAL_ADC_INST_SUB_CH                                               (1)
#define GPIO_SIGNAL_ADC_C2_PORT                                            GPIOA
#define GPIO_SIGNAL_ADC_C2_PIN                                    DL_GPIO_PIN_25
#define GPIO_SIGNAL_ADC_IOMUX_C2                                 (IOMUX_PINCM55)
#define GPIO_SIGNAL_ADC_IOMUX_C2_FUNC             (IOMUX_PINCM55_PF_UNCONNECTED)



/* Defines for SIGNAL_ADC_DMA */
#define SIGNAL_ADC_DMA_CHAN_ID                                               (0)
#define SIGNAL_ADC_INST_DMA_TRIGGER                   (DMA_ADC0_EVT_GEN_BD_TRIG)


/* Port definition for Pin Group GPIO_TFT_CTRL */
#define GPIO_TFT_CTRL_PORT                                               (GPIOB)

/* Defines for TFT_DC: GPIOB.15 with pinCMx 32 on package pin 3 */
#define GPIO_TFT_CTRL_TFT_DC_PIN                                (DL_GPIO_PIN_15)
#define GPIO_TFT_CTRL_TFT_DC_IOMUX                               (IOMUX_PINCM32)
/* Defines for TFT_BLK: GPIOB.12 with pinCMx 29 on package pin 64 */
#define GPIO_TFT_CTRL_TFT_BLK_PIN                               (DL_GPIO_PIN_12)
#define GPIO_TFT_CTRL_TFT_BLK_IOMUX                              (IOMUX_PINCM29)
/* Port definition for Pin Group GPIO_KEYPAD */
#define GPIO_KEYPAD_PORT                                                 (GPIOB)

/* Defines for KEYPAD_R1: GPIOB.16 with pinCMx 33 on package pin 4 */
#define GPIO_KEYPAD_KEYPAD_R1_PIN                               (DL_GPIO_PIN_16)
#define GPIO_KEYPAD_KEYPAD_R1_IOMUX                              (IOMUX_PINCM33)
/* Defines for KEYPAD_R2: GPIOB.0 with pinCMx 12 on package pin 47 */
#define GPIO_KEYPAD_KEYPAD_R2_PIN                                (DL_GPIO_PIN_0)
#define GPIO_KEYPAD_KEYPAD_R2_IOMUX                              (IOMUX_PINCM12)
/* Defines for KEYPAD_R3: GPIOB.7 with pinCMx 24 on package pin 59 */
#define GPIO_KEYPAD_KEYPAD_R3_PIN                                (DL_GPIO_PIN_7)
#define GPIO_KEYPAD_KEYPAD_R3_IOMUX                              (IOMUX_PINCM24)
/* Defines for KEYPAD_R4: GPIOB.17 with pinCMx 43 on package pin 14 */
#define GPIO_KEYPAD_KEYPAD_R4_PIN                               (DL_GPIO_PIN_17)
#define GPIO_KEYPAD_KEYPAD_R4_IOMUX                              (IOMUX_PINCM43)
/* Defines for KEYPAD_C1: GPIOB.18 with pinCMx 44 on package pin 15 */
#define GPIO_KEYPAD_KEYPAD_C1_PIN                               (DL_GPIO_PIN_18)
#define GPIO_KEYPAD_KEYPAD_C1_IOMUX                              (IOMUX_PINCM44)
/* Defines for KEYPAD_C2: GPIOB.13 with pinCMx 30 on package pin 1 */
#define GPIO_KEYPAD_KEYPAD_C2_PIN                               (DL_GPIO_PIN_13)
#define GPIO_KEYPAD_KEYPAD_C2_IOMUX                              (IOMUX_PINCM30)
/* Defines for KEYPAD_C3: GPIOB.20 with pinCMx 48 on package pin 19 */
#define GPIO_KEYPAD_KEYPAD_C3_PIN                               (DL_GPIO_PIN_20)
#define GPIO_KEYPAD_KEYPAD_C3_IOMUX                              (IOMUX_PINCM48)
/* Defines for KEYPAD_C4: GPIOB.4 with pinCMx 17 on package pin 52 */
#define GPIO_KEYPAD_KEYPAD_C4_PIN                                (DL_GPIO_PIN_4)
#define GPIO_KEYPAD_KEYPAD_C4_IOMUX                              (IOMUX_PINCM17)


/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_SIGNAL_SAMPLE_TIMER_init(void);
void SYSCFG_DL_SPI_TFT_init(void);
void SYSCFG_DL_SIGNAL_ADC_init(void);
void SYSCFG_DL_DMA_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
