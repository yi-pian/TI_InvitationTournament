/*
 * Copyright (c) 2021, Texas Instruments Incorporated
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

#include "ti_msp_dl_config.h"
#include "signal_tft_ili9341.h"
#include "signal_mspm0g3507_tft_platform.h"

static tft_ili9341_t g_tft;
static signal_mspm0g3507_tft_context_t g_tft_platform = {
    .spi = SPI_TFT_INST,
    .cs_port = NULL,
    .cs_pin = 0U,
    .dc_port = GPIO_TFT_CTRL_PORT,
    .dc_pin = GPIO_TFT_CTRL_TFT_DC_PIN,
    .reset_port = NULL,
    .reset_pin = 0U,
    .backlight_port = GPIO_TFT_CTRL_PORT,
    .backlight_pin = GPIO_TFT_CTRL_TFT_BLK_PIN,
    .cpu_clock_hz = CPUCLK_FREQ,
};
static volatile uint8_t g_pin_test_mode = 0U;

static void tft_pin_test_start(void)
{
    /* Temporarily take the SPI pins back as GPIO outputs. */
    DL_GPIO_initDigitalOutput(GPIO_SPI_TFT_IOMUX_PICO);
    DL_GPIO_initDigitalOutput(GPIO_SPI_TFT_IOMUX_SCLK);
    DL_GPIO_initDigitalOutput(GPIO_SPI_TFT_IOMUX_CS0);
    DL_GPIO_initDigitalOutput(GPIO_TFT_CTRL_TFT_DC_IOMUX);
    DL_GPIO_initDigitalOutput(GPIO_TFT_CTRL_TFT_BLK_IOMUX);

    DL_GPIO_enableOutput(GPIO_SPI_TFT_PICO_PORT, GPIO_SPI_TFT_PICO_PIN);
    DL_GPIO_enableOutput(GPIO_SPI_TFT_SCLK_PORT, GPIO_SPI_TFT_SCLK_PIN);
    DL_GPIO_enableOutput(GPIO_SPI_TFT_CS0_PORT, GPIO_SPI_TFT_CS0_PIN);
    DL_GPIO_enableOutput(GPIO_TFT_CTRL_PORT,
                         GPIO_TFT_CTRL_TFT_DC_PIN | GPIO_TFT_CTRL_TFT_BLK_PIN);

    /* All five TFT signal pins must measure about 3.3 V at the TFT header. */
    DL_GPIO_setPins(GPIO_SPI_TFT_PICO_PORT, GPIO_SPI_TFT_PICO_PIN);
    DL_GPIO_setPins(GPIO_SPI_TFT_SCLK_PORT, GPIO_SPI_TFT_SCLK_PIN);
    DL_GPIO_setPins(GPIO_SPI_TFT_CS0_PORT, GPIO_SPI_TFT_CS0_PIN);
    DL_GPIO_setPins(GPIO_TFT_CTRL_PORT,
                    GPIO_TFT_CTRL_TFT_DC_PIN | GPIO_TFT_CTRL_TFT_BLK_PIN);
}

static void tft_halt(void)
{
    TFT_ILI9341_SetBacklight(&g_tft, false);

    while (1) {
    }
}

int main(void)
{
    tft_ili9341_config_t tft_config;

    SYSCFG_DL_init();

    /* SPI1 hardware owns PB6/CS0 in this example. */
    if (SignalMSPM0G3507_TFT_Bind(&tft_config, &g_tft_platform, true) !=
        SIGNAL_RESULT_OK) {
        tft_halt();
    }

    if (g_pin_test_mode != 0U) {
        tft_pin_test_start();
        while (1) {
        }
    }

    SignalMSPM0G3507_TFT_DelayMs(&g_tft_platform, 350U);
    if (TFT_ILI9341_Init(&g_tft, &tft_config,
                         TFT_ILI9341_ROTATION_90) != TFT_ILI9341_OK) {
        tft_halt();
    }

    if (TFT_ILI9341_FillScreen(&g_tft, TFT_ILI9341_BLACK) != TFT_ILI9341_OK) {
        tft_halt();
    }

    if (TFT_ILI9341_FillRect(&g_tft, 10, 10, 90, 220,
                             TFT_ILI9341_RED) != TFT_ILI9341_OK) {
        tft_halt();
    }

    if (TFT_ILI9341_FillRect(&g_tft, 115, 10, 90, 220,
                             TFT_ILI9341_GREEN) != TFT_ILI9341_OK) {
        tft_halt();
    }

    if (TFT_ILI9341_FillRect(&g_tft, 220, 10, 90, 220,
                             TFT_ILI9341_BLUE) != TFT_ILI9341_OK) {
        tft_halt();
    }

    if (TFT_ILI9341_DrawRect(&g_tft, 0, 0,
                             TFT_ILI9341_GetWidth(&g_tft),
                             TFT_ILI9341_GetHeight(&g_tft),
                             TFT_ILI9341_WHITE) != TFT_ILI9341_OK) {
        tft_halt();
    }

    if (TFT_ILI9341_DrawString(&g_tft, 8, 8, "MSPM0 SIGNAL",
                               TFT_ILI9341_FONT_8X16,
                               TFT_ILI9341_WHITE, TFT_ILI9341_RED,
                               false, false) != TFT_ILI9341_OK) {
        tft_halt();
    }

    if (TFT_ILI9341_DrawString(&g_tft, 8, 32, "VPP=",
                               TFT_ILI9341_FONT_8X16,
                               TFT_ILI9341_YELLOW, TFT_ILI9341_RED,
                               false, false) != TFT_ILI9341_OK) {
        tft_halt();
    }

    if (TFT_ILI9341_DrawFloat(&g_tft, 40, 32, 1.25F, 2U,
                              TFT_ILI9341_FONT_8X16,
                              TFT_ILI9341_YELLOW, TFT_ILI9341_RED,
                              false) != TFT_ILI9341_OK) {
        tft_halt();
    }

    if (TFT_ILI9341_DrawMonoBitmap(
            &g_tft, 8, 56, 16U, 16U,
            TFT_ILI9341_GLYPH_CN_DIAN_16X16,
            TFT_ILI9341_GLYPH_16X16_BYTES,
            TFT_ILI9341_WHITE, TFT_ILI9341_RED, false) != TFT_ILI9341_OK) {
        tft_halt();
    }

    if (TFT_ILI9341_DrawMonoBitmap(
            &g_tft, 24, 56, 16U, 16U,
            TFT_ILI9341_GLYPH_CN_ZI_16X16,
            TFT_ILI9341_GLYPH_16X16_BYTES,
            TFT_ILI9341_WHITE, TFT_ILI9341_RED, false) != TFT_ILI9341_OK) {
        tft_halt();
    }

    while (1) {
    }
}
