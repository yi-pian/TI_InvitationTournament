#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "ssd1306.h"
#include "ssd1306_mspm0g3507.h"

static uint8_t g_framebuffer[SSD1306_FRAMEBUFFER_SIZE];
static ssd1306_t g_display;
static ssd1306_mspm0_i2c_t g_display_bus;

int main(void)
{
    ssd1306_status_t status;

    SYSCFG_DL_init();
    SSD1306_ClearBuffer(g_framebuffer);
    (void) SSD1306_DrawString6x8(
        g_framebuffer, 0U, 0U, "MSPM0 OLED", true);
    SSD1306_DrawLine(g_framebuffer, 0, 15, 127, 15, true);

    status = SignalSSD1306_MSPM0_Init(&g_display, &g_display_bus,
        SSD1306_I2C_ADDRESS_DEFAULT, false);
    if (status == SSD1306_STATUS_OK) {
        status = SSD1306_Update(&g_display, g_framebuffer);
    }
    if (status != SSD1306_STATUS_OK) {
        __BKPT(0);
    }

    while (1) {
        __WFI();
    }
}
