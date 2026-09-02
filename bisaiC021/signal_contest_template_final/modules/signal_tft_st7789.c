#include "signal_tft_st7789.h"

#include <string.h>

#define ST7789_SWRESET 0x01U
#define ST7789_SLPOUT  0x11U
#define ST7789_COLMOD  0x3AU
#define ST7789_MADCTL  0x36U
#define ST7789_CASET   0x2AU
#define ST7789_RASET   0x2BU
#define ST7789_RAMWR   0x2CU
#define ST7789_DISPON  0x29U

static bool valid(const tft_st7789_t *tft)
{
    return tft != NULL && tft->config.write != NULL &&
           tft->config.set_dc != NULL;
}

static void cs(tft_st7789_t *tft, bool high)
{
    if (tft->config.set_cs != NULL) {
        tft->config.set_cs(tft->config.context, high);
    }
}

static tft_st7789_status_t transfer_unlocked(tft_st7789_t *tft,
    bool data_mode, const uint8_t *data, size_t length)
{
    int result;
    if (!valid(tft) || (data == NULL && length != 0U)) {
        return TFT_ST7789_ERROR_ARGUMENT;
    }
    if (length == 0U) {
        return TFT_ST7789_OK;
    }
    tft->config.set_dc(tft->config.context, data_mode);
    cs(tft, false);
    result = tft->config.write(tft->config.context, data, length);
    cs(tft, true);
    return result == 0 ? TFT_ST7789_OK : TFT_ST7789_ERROR_IO;
}

static tft_st7789_status_t transfer(tft_st7789_t *tft, bool data_mode,
    const uint8_t *data, size_t length)
{
    tft_st7789_status_t status;
    if (!valid(tft)) return TFT_ST7789_ERROR_ARGUMENT;
    if (tft->config.lock != NULL) tft->config.lock(tft->config.context);
    status = transfer_unlocked(tft, data_mode, data, length);
    if (tft->config.unlock != NULL) tft->config.unlock(tft->config.context);
    return status;
}

static tft_st7789_status_t command_data_unlocked(tft_st7789_t *tft,
    uint8_t command, const uint8_t *data, size_t length)
{
    tft_st7789_status_t status = transfer_unlocked(tft, false, &command, 1U);
    if (status == TFT_ST7789_OK && length != 0U) {
        status = transfer_unlocked(tft, true, data, length);
    }
    return status;
}

static tft_st7789_status_t command_data(tft_st7789_t *tft, uint8_t command,
    const uint8_t *data, size_t length)
{
    tft_st7789_status_t status;
    if (!valid(tft)) return TFT_ST7789_ERROR_ARGUMENT;
    if (tft->config.lock != NULL) tft->config.lock(tft->config.context);
    status = command_data_unlocked(tft, command, data, length);
    if (tft->config.unlock != NULL) tft->config.unlock(tft->config.context);
    return status;
}

tft_st7789_status_t TFT_ST7789_WriteCommand(tft_st7789_t *tft, uint8_t command)
{
    return transfer(tft, false, &command, 1U);
}

tft_st7789_status_t TFT_ST7789_WriteData(tft_st7789_t *tft,
    const uint8_t *data, size_t length)
{
    return transfer(tft, true, data, length);
}

static tft_st7789_status_t set_rotation_raw(tft_st7789_t *tft,
    tft_st7789_rotation_t rotation)
{
    static const uint8_t madctl[] = {0x00U, 0x60U, 0xC0U, 0xA0U};
    tft_st7789_status_t status;
    status = command_data(tft, ST7789_MADCTL, &madctl[rotation], 1U);
    if (status == TFT_ST7789_OK) {
        tft->rotation = rotation;
        tft->width = (rotation == TFT_ST7789_ROTATION_90 ||
                      rotation == TFT_ST7789_ROTATION_270) ?
            TFT_ST7789_NATIVE_HEIGHT : TFT_ST7789_NATIVE_WIDTH;
        tft->height = (rotation == TFT_ST7789_ROTATION_90 ||
                       rotation == TFT_ST7789_ROTATION_270) ?
            TFT_ST7789_NATIVE_WIDTH : TFT_ST7789_NATIVE_HEIGHT;
    }
    return status;
}

tft_st7789_status_t TFT_ST7789_Init(tft_st7789_t *tft,
    const tft_st7789_config_t *config, tft_st7789_rotation_t rotation)
{
    static const uint8_t init[] = {
        ST7789_COLMOD, 1U, 0x05U,
        0xC5U, 1U, 0x1AU,
        ST7789_MADCTL, 1U, 0x00U,
        0xB2U, 5U, 0x05U, 0x05U, 0x00U, 0x33U, 0x33U,
        0xB7U, 1U, 0x05U,
        0xBBU, 1U, 0x3FU,
        0xC0U, 1U, 0x2CU,
        0xC2U, 1U, 0x01U,
        0xC3U, 1U, 0x0FU,
        0xC4U, 1U, 0x20U,
        0xC6U, 1U, 0x01U,
        0xD0U, 2U, 0xA4U, 0xA1U,
        0xE8U, 1U, 0x03U,
        0xE9U, 3U, 0x09U, 0x09U, 0x08U,
        0xE0U, 14U, 0xD0U, 0x05U, 0x09U, 0x09U, 0x08U, 0x14U,
            0x28U, 0x33U, 0x3FU, 0x07U, 0x13U, 0x14U, 0x28U, 0x30U,
        0xE1U, 14U, 0xD0U, 0x05U, 0x09U, 0x09U, 0x08U, 0x03U,
            0x24U, 0x32U, 0x32U, 0x3BU, 0x14U, 0x13U, 0x28U, 0x2FU
    };
    size_t i = 0U;
    tft_st7789_status_t status;
    if (tft == NULL || config == NULL || config->write == NULL ||
        config->set_dc == NULL || config->delay_ms == NULL ||
        (config->lock == NULL) != (config->unlock == NULL) ||
        rotation > TFT_ST7789_ROTATION_270) {
        return TFT_ST7789_ERROR_ARGUMENT;
    }
    (void)memset(tft, 0, sizeof(*tft));
    tft->config = *config;
    tft->width = TFT_ST7789_NATIVE_WIDTH;
    tft->height = TFT_ST7789_NATIVE_HEIGHT;
    if (config->set_cs != NULL) config->set_cs(config->context, true);
    config->set_dc(config->context, true);
    if (config->set_backlight != NULL) config->set_backlight(config->context, false);
    if (config->set_reset != NULL) {
        config->set_reset(config->context, false);
        config->delay_ms(config->context, 10U);
        config->set_reset(config->context, true);
        config->delay_ms(config->context, 120U);
    } else {
        config->delay_ms(config->context, 120U);
    }
    status = TFT_ST7789_WriteCommand(tft, ST7789_SWRESET);
    if (status != TFT_ST7789_OK) return status;
    config->delay_ms(config->context, 120U);
    status = TFT_ST7789_WriteCommand(tft, ST7789_SLPOUT);
    if (status != TFT_ST7789_OK) return status;
    config->delay_ms(config->context, 120U);
    while (i < sizeof(init)) {
        uint8_t command = init[i++];
        uint8_t count = init[i++];
        status = command_data(tft, command, &init[i], count);
        if (status != TFT_ST7789_OK) return status;
        i += count;
    }
    status = TFT_ST7789_WriteCommand(tft, ST7789_DISPON);
    if (status != TFT_ST7789_OK) return status;
    config->delay_ms(config->context, 20U);
    tft->initialized = true;
    status = TFT_ST7789_SetRotation(tft, rotation);
    if (status == TFT_ST7789_OK) TFT_ST7789_SetBacklight(tft, true);
    return status;
}

tft_st7789_status_t TFT_ST7789_SetRotation(tft_st7789_t *tft,
    tft_st7789_rotation_t rotation)
{
    if (!valid(tft) || rotation > TFT_ST7789_ROTATION_270) return TFT_ST7789_ERROR_ARGUMENT;
    if (!tft->initialized) return TFT_ST7789_ERROR_NOT_INITIALIZED;
    return set_rotation_raw(tft, rotation);
}

void TFT_ST7789_SetBacklight(tft_st7789_t *tft, bool on)
{
    if (tft != NULL && tft->config.set_backlight != NULL)
        tft->config.set_backlight(tft->config.context, on);
}

uint16_t TFT_ST7789_GetWidth(const tft_st7789_t *tft) { return tft ? tft->width : 0U; }
uint16_t TFT_ST7789_GetHeight(const tft_st7789_t *tft) { return tft ? tft->height : 0U; }

tft_st7789_status_t TFT_ST7789_SetAddressWindow(tft_st7789_t *tft,
    uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t data[4];
    tft_st7789_status_t status;
    if (!valid(tft)) return TFT_ST7789_ERROR_ARGUMENT;
    if (!tft->initialized) return TFT_ST7789_ERROR_NOT_INITIALIZED;
    if (x0 > x1 || y0 > y1 || x1 >= tft->width || y1 >= tft->height)
        return TFT_ST7789_ERROR_RANGE;
    x0 = (uint16_t)(x0 + tft->config.x_offset); x1 = (uint16_t)(x1 + tft->config.x_offset);
    y0 = (uint16_t)(y0 + tft->config.y_offset); y1 = (uint16_t)(y1 + tft->config.y_offset);
    data[0]=(uint8_t)(x0>>8U); data[1]=(uint8_t)x0; data[2]=(uint8_t)(x1>>8U); data[3]=(uint8_t)x1;
    status = command_data(tft, ST7789_CASET, data, sizeof(data));
    if (status != TFT_ST7789_OK) return status;
    data[0]=(uint8_t)(y0>>8U); data[1]=(uint8_t)y0; data[2]=(uint8_t)(y1>>8U); data[3]=(uint8_t)y1;
    status = command_data(tft, ST7789_RASET, data, sizeof(data));
    if (status != TFT_ST7789_OK) return status;
    return TFT_ST7789_WriteCommand(tft, ST7789_RAMWR);
}

static tft_st7789_status_t draw_pixels(tft_st7789_t *tft,
    const uint16_t *pixels, size_t count)
{
    uint8_t bytes[128U];
    while (count != 0U) {
        size_t n = count > 64U ? 64U : count;
        size_t i;
        for (i=0U; i<n; ++i) { bytes[2U*i]=(uint8_t)(pixels[i]>>8U); bytes[2U*i+1U]=(uint8_t)pixels[i]; }
        if (TFT_ST7789_WriteData(tft, bytes, 2U*n) != TFT_ST7789_OK) return TFT_ST7789_ERROR_IO;
        pixels += n; count -= n;
    }
    return TFT_ST7789_OK;
}

tft_st7789_status_t TFT_ST7789_DrawPixel(tft_st7789_t *tft, int32_t x, int32_t y, uint16_t color)
{
    if (!valid(tft)) return TFT_ST7789_ERROR_ARGUMENT;
    if (!tft->initialized) return TFT_ST7789_ERROR_NOT_INITIALIZED;
    if (x < 0 || y < 0 || x >= (int32_t)tft->width || y >= (int32_t)tft->height) return TFT_ST7789_ERROR_RANGE;
    if (TFT_ST7789_SetAddressWindow(tft,(uint16_t)x,(uint16_t)y,(uint16_t)x,(uint16_t)y)!=TFT_ST7789_OK) return TFT_ST7789_ERROR_IO;
    return draw_pixels(tft,&color,1U);
}

tft_st7789_status_t TFT_ST7789_FillRect(tft_st7789_t *tft, int32_t x, int32_t y, int32_t width, int32_t height, uint16_t color)
{
    uint16_t row[64]; int32_t end_x, end_y, row_x, row_y;
    size_t i;
    if (!valid(tft) || !tft->initialized || width <= 0 || height <= 0) return TFT_ST7789_ERROR_ARGUMENT;
    end_x=x+width-1; end_y=y+height-1;
    if (x < 0 || y < 0 || end_x >= tft->width || end_y >= tft->height) return TFT_ST7789_ERROR_RANGE;
    for (i=0U;i<64U;++i) row[i]=color;
    if (TFT_ST7789_SetAddressWindow(tft,(uint16_t)x,(uint16_t)y,(uint16_t)end_x,(uint16_t)end_y)!=TFT_ST7789_OK) return TFT_ST7789_ERROR_IO;
    for (row_y=y;row_y<=end_y;++row_y) for (row_x=x;row_x<=end_x;) { size_t n=(size_t)(end_x-row_x+1); if(n>64U)n=64U; if(draw_pixels(tft,row,n)!=TFT_ST7789_OK)return TFT_ST7789_ERROR_IO; row_x+=(int32_t)n; }
    return TFT_ST7789_OK;
}

tft_st7789_status_t TFT_ST7789_FillScreen(tft_st7789_t *tft, uint16_t color)
{
    if (!valid(tft)) return TFT_ST7789_ERROR_ARGUMENT;
    return TFT_ST7789_FillRect(tft, 0, 0, (int32_t)tft->width,
        (int32_t)tft->height, color);
}

tft_st7789_status_t TFT_ST7789_DrawLine(tft_st7789_t *tft,int32_t x0,int32_t y0,int32_t x1,int32_t y1,uint16_t color)
{
    int32_t dx=x1>x0?x1-x0:x0-x1, sx=x0<x1?1:-1, dy=y1>y0?y1-y0:y0-y1, sy=y0<y1?1:-1, err=dx-dy, e2;
    tft_st7789_status_t status;
    if (!valid(tft)) return TFT_ST7789_ERROR_ARGUMENT;
    if (!tft->initialized) return TFT_ST7789_ERROR_NOT_INITIALIZED;
    do { if(x0>=0&&y0>=0&&x0<tft->width&&y0<tft->height){status=TFT_ST7789_DrawPixel(tft,x0,y0,color);if(status!=TFT_ST7789_OK)return status;} if(x0==x1&&y0==y1)break; e2=2*err; if(e2>-dy){err-=dy;x0+=sx;} if(e2<dx){err+=dx;y0+=sy;} } while(1);
    return TFT_ST7789_OK;
}

tft_st7789_status_t TFT_ST7789_DrawRect(tft_st7789_t *tft,int32_t x,int32_t y,int32_t width,int32_t height,uint16_t color)
{ if(width<=0||height<=0)return TFT_ST7789_ERROR_ARGUMENT; if(TFT_ST7789_DrawLine(tft,x,y,x+width-1,y,color)!=TFT_ST7789_OK)return TFT_ST7789_ERROR_IO; if(TFT_ST7789_DrawLine(tft,x,y+height-1,x+width-1,y+height-1,color)!=TFT_ST7789_OK)return TFT_ST7789_ERROR_IO; if(TFT_ST7789_DrawLine(tft,x,y,x,y+height-1,color)!=TFT_ST7789_OK)return TFT_ST7789_ERROR_IO; return TFT_ST7789_DrawLine(tft,x+width-1,y,x+width-1,y+height-1,color); }

tft_st7789_status_t TFT_ST7789_DrawRGB565(tft_st7789_t *tft,int32_t x,int32_t y,int32_t width,int32_t height,const uint16_t *pixels)
{
    int32_t row; if(!valid(tft)||!tft->initialized||pixels==NULL||width<=0||height<=0)return TFT_ST7789_ERROR_ARGUMENT; if(x<0||y<0||x+width>tft->width||y+height>tft->height)return TFT_ST7789_ERROR_RANGE; if(TFT_ST7789_SetAddressWindow(tft,x,y,x+width-1,y+height-1)!=TFT_ST7789_OK)return TFT_ST7789_ERROR_IO; for(row=0;row<height;++row){if(draw_pixels(tft,pixels+(size_t)row*(size_t)width,(size_t)width)!=TFT_ST7789_OK)return TFT_ST7789_ERROR_IO;} return TFT_ST7789_OK;
}

tft_st7789_status_t TFT_ST7789_DrawCircle(tft_st7789_t *tft, int32_t x0, int32_t y0, int32_t radius, uint16_t color)
{
    int32_t x, y, d, ddF_x, ddF_y, status;
    
    // 1. 参数有效性检查 (仿照 DrawRGB565 和 DrawRect)
    if (!valid(tft) || !tft->initialized || radius <= 0) 
        return TFT_ST7789_ERROR_ARGUMENT;
    
    // 2. 边界检查：如果圆心完全在屏幕外且半径过小，可能无法绘制，但通常只要部分可见即可绘制
    // 为了严谨，我们检查中心点是否在合理范围内，或者至少允许部分绘制
    // 注意：中点算法本身不依赖 SetAddressWindow，它直接调用 DrawPixel
    // 因此需要确保 DrawPixel 内部能处理越界（通常 DrawPixel 会检查 x0/tft->width 等）
    
    // 初始化中点算法变量
    x = 0;
    y = radius;
    d = 3 - (2 * radius); // 初始决策参数
    ddF_x = 1;
    ddF_y = -2 * radius;
    
    // 3. 绘制 8 个对称点
    // 使用 do-while 循环处理从顶部开始的情况，或者 while 循环
    // 这里使用 while 循环，条件为 x <= y
    
    // 辅助宏或内联逻辑来绘制 8 个点
    // 点1: (x0+x, y0+y)
    status = TFT_ST7789_DrawPixel(tft, x0 + x, y0 + y, color); if(status != TFT_ST7789_OK) return status;
    // 点2: (x0-x, y0+y)
    status = TFT_ST7789_DrawPixel(tft, x0 - x, y0 + y, color); if(status != TFT_ST7789_OK) return status;
    // 点3: (x0+x, y0-y)
    status = TFT_ST7789_DrawPixel(tft, x0 + x, y0 - y, color); if(status != TFT_ST7789_OK) return status;
    // 点4: (x0-x, y0-y)
    status = TFT_ST7789_DrawPixel(tft, x0 - x, y0 - y, color); if(status != TFT_ST7789_OK) return status;
    // 点5: (x0+y, y0+x)
    status = TFT_ST7789_DrawPixel(tft, x0 + y, y0 + x, color); if(status != TFT_ST7789_OK) return status;
    // 点6: (x0-y, y0+x)
    status = TFT_ST7789_DrawPixel(tft, x0 - y, y0 + x, color); if(status != TFT_ST7789_OK) return status;
    // 点7: (x0+y, y0-x)
    status = TFT_ST7789_DrawPixel(tft, x0 + y, y0 - x, color); if(status != TFT_ST7789_OK) return status;
    // 点8: (x0-y, y0-x)
    status = TFT_ST7789_DrawPixel(tft, x0 - y, y0 - x, color); if(status != TFT_ST7789_OK) return status;
    
    // 循环计算剩余点
    while (x < y) {
        if (d < 0) {
            // 选择 E (East)
            d += ddF_x;
            ddF_x += 2;
            x++;
        } else {
            // 选择 SE (South-East)
            d += ddF_x + ddF_y;
            ddF_x += 2;
            ddF_y += 2;
            x++;
            y--;
        }
        
        // 重复绘制 8 个对称点
        status = TFT_ST7789_DrawPixel(tft, x0 + x, y0 + y, color); if(status != TFT_ST7789_OK) return status;
        status = TFT_ST7789_DrawPixel(tft, x0 - x, y0 + y, color); if(status != TFT_ST7789_OK) return status;
        status = TFT_ST7789_DrawPixel(tft, x0 + x, y0 - y, color); if(status != TFT_ST7789_OK) return status;
        status = TFT_ST7789_DrawPixel(tft, x0 - x, y0 - y, color); if(status != TFT_ST7789_OK) return status;
        status = TFT_ST7789_DrawPixel(tft, x0 + y, y0 + x, color); if(status != TFT_ST7789_OK) return status;
        status = TFT_ST7789_DrawPixel(tft, x0 - y, y0 + x, color); if(status != TFT_ST7789_OK) return status;
        status = TFT_ST7789_DrawPixel(tft, x0 + y, y0 - x, color); if(status != TFT_ST7789_OK) return status;
        status = TFT_ST7789_DrawPixel(tft, x0 - y, y0 - x, color); if(status != TFT_ST7789_OK) return status;
    }
    
    return TFT_ST7789_OK;
}


tft_st7789_status_t TFT_ST7789_DrawMonoBitmap(tft_st7789_t *tft,int32_t x,int32_t y,uint8_t width,uint8_t height,const uint8_t *bitmap,size_t bitmap_size,uint16_t fg,uint16_t bg,bool transparent)
{
    uint8_t bpr=(uint8_t)((width+7U)/8U); uint16_t color; uint8_t yy,xx;
    if(bitmap==NULL||width==0U||height==0U||bitmap_size<(size_t)bpr*height)return TFT_ST7789_ERROR_ARGUMENT;
    for(yy=0U;yy<height;++yy)for(xx=0U;xx<width;++xx){bool on=(bitmap[(size_t)yy*bpr+(xx>>3U)]&(uint8_t)(1U<<(xx&7U)))!=0U; if(!on&&transparent)continue; color=on?fg:bg; if(TFT_ST7789_DrawPixel(tft,x+xx,y+yy,color)!=TFT_ST7789_OK)return TFT_ST7789_ERROR_IO;}
    return TFT_ST7789_OK;
}

signal_module_status_t SignalTFTST7789_GetModuleStatus(void)
{
    return MODULE_STATUS_BUILD_VERIFIED;
}
