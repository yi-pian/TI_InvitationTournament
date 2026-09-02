#include "ti_msp_dl_config.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_font.h"
#include "signal_tft_st7789_mspm0g3507.h"

static tft_st7789_t g_tft;
volatile tft_st7789_status_t g_status;

int main(void)
{
    SYSCFG_DL_init();
    g_status = SignalTFTST7789_MSPM0_Init(
        &g_tft, TFT_ST7789_ROTATION_90, 0U, 0U);
    if (g_status != TFT_ST7789_OK) while (1) { }
    g_status = TFT_ST7789_FillScreen(&g_tft, TFT_ST7789_BLACK);
    if (g_status == TFT_ST7789_OK)
        g_status = TFT_ST7789_DrawRect(&g_tft, 10, 10, 100, 60, TFT_ST7789_GREEN);
    if (g_status == TFT_ST7789_OK)
        g_status = TFT_ST7789_DrawString(&g_tft, 16, 80, "ST7789 OK",
            TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN, TFT_ST7789_BLACK,
            false, false);
    if (g_status == TFT_ST7789_OK)
        g_status = TFT_ST7789_DrawFloat(&g_tft, 16, 102, 10000.08f, 2U,
            TFT_ST7789_FONT_6X12, TFT_ST7789_WHITE, TFT_ST7789_BLACK, false);
    if (g_status == TFT_ST7789_OK)
        g_status = TFT_ST7789_DrawMonoBitmap(&g_tft, 16, 120, 16U, 16U,
            TFT_ST7789_GLYPH_CN_DIAN_16X16,
            TFT_ST7789_GLYPH_16X16_BYTES, TFT_ST7789_YELLOW,
            TFT_ST7789_BLACK, false);
    while (1) { __WFI(); }
}
