/* 工程：80_tft_usage。教学流程：初始化 TFT → 画固定标题 → 更新数值与页面。 */
#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_mspm0g3507.h"
#include "signal_tft_st7789_font.h"

/* 已初始化的 ST7789 显示对象；全部绘图函数读取它。 */
static tft_st7789_t tft;
/* 当前页号；外部键盘/页面状态机写入，DrawPage() 读取。 */
static uint8_t current_page;
/* 要显示的测量值；本例单位 Hz，UpdateLiveValue() 读取。 */
static float frequency_hz = 1000.0f;

/* ============================================================
 * 函数：InitTFTDemo
 * [功能] 按本工程既有旋转角度和偏移初始化 ST7789 平台适配层。
 * [输入] SysConfig 已初始化的 SPI/GPIO；[输出] tft 可供绘图。
 * [返回值] true：初始化成功；false：TFT 平台初始化失败。
 * [复用] 需要 signal_tft_st7789、signal_tft_st7789_mspm0g3507 和相同 TFT
 * 硬件/SysConfig；不要在别的屏幕型号上盲用旋转与偏移参数。
 * ============================================================ */
static bool InitTFTDemo(void)
{
    return SignalTFTST7789_MSPM0_Init(&tft, TFT_ST7789_ROTATION_270,
        0U, 0U) == TFT_ST7789_OK;
}

/* ============================================================
 * [COPY START: TFT_TEXT]
 * 函数：DrawStaticText
 * [功能] 清屏并绘制不随每帧数据变化的标题。
 * [输入] 已初始化 tft；[输出] 黑底白字 "MSPM0 SIGNAL"。
 * [为什么单独执行] 固定内容只需初始化后画一次，避免在实时循环反复刷整屏。
 * [复用] 需要 InitTFTDemo() 和 ST7789 字库模块。
 * ============================================================ */
static void DrawStaticText(void)
{
    (void)TFT_ST7789_FillScreen(&tft, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawString(&tft, 8, 8, "MSPM0 SIGNAL",
        TFT_ST7789_FONT_8X16, TFT_ST7789_WHITE, TFT_ST7789_BLACK,
        false, false);
}
/* [COPY END: TFT_TEXT] */

/* ============================================================
 * [COPY START: TFT_VARIABLE]
 * 函数：DisplayVariable
 * [功能] 显示 frequency_hz 的整数部分，作为最简单的变量显示示例。
 * [输入] frequency_hz：float，Hz。
 * [输出] 坐标 (8,32) 的青色数值。
 * [复用] 若显示 V、deg、dB，必须保持变量的真实单位并调整标签/格式。
 * ============================================================ */
static void DisplayVariable(void)
{
    (void)TFT_ST7789_DrawInt32(&tft, 8, 32, (int32_t)frequency_hz,
        TFT_ST7789_FONT_8X16, TFT_ST7789_CYAN, TFT_ST7789_BLACK,
        false);
}
/* [COPY END: TFT_VARIABLE] */

/* ============================================================
 * [COPY START: TFT_LIVE_VALUE]
 * 函数：UpdateLiveValue
 * [功能] 先局部清除旧数值区域，再显示新的 frequency_hz，避免数字缩短时残留。
 * [输入] frequency_hz：float，Hz；[输出] 坐标 (8,56) 的黄色数值。
 * [为什么局部刷新] 比整屏重绘快，且不破坏标题和其他页面元素。
 * [复用] 需为最大可能位数预留 FillRect 区域；不能假定新数值总比旧数值长。
 * ============================================================ */
static void UpdateLiveValue(void)
{
    (void)TFT_ST7789_FillRect(&tft, 8, 56, 120, 16, TFT_ST7789_BLACK);
    (void)TFT_ST7789_DrawInt32(&tft, 8, 56, (int32_t)frequency_hz,
        TFT_ST7789_FONT_8X16, TFT_ST7789_YELLOW, TFT_ST7789_BLACK,
        false);
}
/* [COPY END: TFT_LIVE_VALUE] */

/* ============================================================
 * [COPY START: TFT_TWO_PAGES]
 * 函数：DrawPage
 * [功能] 根据 current_page 绘制两页示例的页标识。
 * [输入] current_page：uint8_t；[输出] 坐标 (8,80) 的 PAGE 0/PAGE 1。
 * [复用] 综合工程可替换为自己的页面状态机，但保留“数据更新”和“页面选择”
 * 两个职责边界，避免 main() 充满 TFT 单句调用。
 * ============================================================ */
static void DrawPage(void)
{
    const char *page_text = (current_page == 0U) ? "PAGE 0" : "PAGE 1";

    (void)TFT_ST7789_DrawString(&tft, 8, 80, page_text,
        TFT_ST7789_FONT_8X16, TFT_ST7789_GREEN, TFT_ST7789_BLACK,
        false, false);
}
/* [COPY END: TFT_TWO_PAGES] */

int main(void)
{
    SYSCFG_DL_init();
    if (!InitTFTDemo()) {
        while (true) {
        }
    }
    DrawStaticText();

    while (true) {
        DisplayVariable();
        UpdateLiveValue();
        DrawPage();
        __WFI();
    }
}
