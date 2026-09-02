#include "ti_msp_dl_config.h"
#include "signal_tft_ili9341.h"
#include "signal_tft_ili9341_mspm0g3507.h"

static tft_ili9341_t g_tft;
volatile tft_ili9341_status_t g_status;

int main(void)
{
    SYSCFG_DL_init();
    g_status = SignalTFTILI9341_MSPM0_Init(
        &g_tft, TFT_ILI9341_ROTATION_90);
    if (g_status != TFT_ILI9341_OK) while (1) { }
    g_status = TFT_ILI9341_FillScreen(&g_tft, TFT_ILI9341_BLACK);
    if (g_status == TFT_ILI9341_OK) {
        g_status = TFT_ILI9341_DrawString(&g_tft, 8, 8, "MSPM0",
            TFT_ILI9341_FONT_8X16, TFT_ILI9341_WHITE,
            TFT_ILI9341_BLACK, false, false);
    }

    while (1) {
        /* ===== 这里写你自己的逻辑：刷新数值或波形 ===== */
        __WFI();
    }
}
