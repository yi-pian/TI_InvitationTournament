/* 22_spectrum_display: draw an already computed FFT magnitude frame. */
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "ti_msp_dl_config.h"
#include "signal_config.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_font.h"
#include "signal_tft_st7789_mspm0g3507.h"
#define SPEC_X 8
#define SPEC_Y 48
#define SPEC_W 304
#define SPEC_H 160
static float fft_magnitude[SIGNAL_SAMPLE_COUNT/2U+1U];
static tft_st7789_t tft;
/* [COPY START: SPECTRUM_DISPLAY]
 * [INPUT] fft_magnitude[] float linear amplitude, sample_rate_hz Hz.
 * [OUTPUT] dB spectrum, automatic vertical range and peak marker on TFT.
 * Dependency: signal_tft_st7789, math. Single-frame unique: YES; it must not run FFT. */
static void Spectrum_Draw(float sample_rate_hz)
{
    uint32_t k,x,peak=1U,peak_x;
    float top=-120.0F,bottom,peak_db,peak_frequency_hz;
    int32_t peak_y;
    if(!isfinite(sample_rate_hz)||sample_rate_hz<=0.0F)return;
    for(k=1U;k<=SIGNAL_SAMPLE_COUNT/2U;++k){float d=20.0F*log10f(fmaxf(fft_magnitude[k],1e-9F));if(d>top)top=d;if(fft_magnitude[k]>fft_magnitude[peak])peak=k;}
    top=ceilf(top/10.0F)*10.0F;bottom=top-60.0F;
    peak_db=20.0F*log10f(fmaxf(fft_magnitude[peak],1e-9F));
    peak_frequency_hz=(float)peak*sample_rate_hz/(float)SIGNAL_SAMPLE_COUNT;
    peak_x=(peak-1U)*(SPEC_W-1U)/(SIGNAL_SAMPLE_COUNT/2U-1U);
    peak_y=SPEC_Y+SPEC_H-1-(int32_t)((peak_db-bottom)*(SPEC_H-1)/(top-bottom));
    if(peak_y<SPEC_Y)peak_y=SPEC_Y;if(peak_y>=SPEC_Y+SPEC_H)peak_y=SPEC_Y+SPEC_H-1;
    (void)TFT_ST7789_FillRect(&tft,SPEC_X,SPEC_Y-16,SPEC_W,SPEC_H+32,TFT_ST7789_BLACK);
    for(x=0U;x<(uint32_t)SPEC_W;++x){k=1U+x*(SIGNAL_SAMPLE_COUNT/2U-1U)/(SPEC_W-1U);{float d=20.0F*log10f(fmaxf(fft_magnitude[k],1e-9F));int32_t y=SPEC_Y+SPEC_H-1-(int32_t)((d-bottom)*(SPEC_H-1)/(top-bottom));if(y<SPEC_Y)y=SPEC_Y;if(y>=SPEC_Y+SPEC_H)y=SPEC_Y+SPEC_H-1;(void)TFT_ST7789_DrawLine(&tft,SPEC_X+(int32_t)x,SPEC_Y+SPEC_H-1,SPEC_X+(int32_t)x,y,TFT_ST7789_GREEN);}}
    (void)TFT_ST7789_DrawLine(&tft,SPEC_X+(int32_t)peak_x,SPEC_Y,SPEC_X+(int32_t)peak_x,peak_y,TFT_ST7789_RED);
    (void)TFT_ST7789_DrawString(&tft,SPEC_X,SPEC_Y-14,"Pk",TFT_ST7789_FONT_6X12,TFT_ST7789_RED,TFT_ST7789_BLACK,false,false);
    (void)TFT_ST7789_DrawInt32(&tft,SPEC_X+18,SPEC_Y-14,(int32_t)(peak_frequency_hz+0.5F),TFT_ST7789_FONT_6X12,TFT_ST7789_WHITE,TFT_ST7789_BLACK,false);
    (void)TFT_ST7789_DrawString(&tft,SPEC_X+66,SPEC_Y-14,"Hz",TFT_ST7789_FONT_6X12,TFT_ST7789_WHITE,TFT_ST7789_BLACK,false,false);
    (void)TFT_ST7789_DrawFloat(&tft,SPEC_X+104,SPEC_Y-14,peak_db,1U,TFT_ST7789_FONT_6X12,TFT_ST7789_CYAN,TFT_ST7789_BLACK,false);
    (void)TFT_ST7789_DrawString(&tft,SPEC_X+152,SPEC_Y-14,"dB",TFT_ST7789_FONT_6X12,TFT_ST7789_CYAN,TFT_ST7789_BLACK,false,false);
    (void)TFT_ST7789_DrawString(&tft,SPEC_X,SPEC_Y+SPEC_H+2,"0Hz",TFT_ST7789_FONT_6X12,TFT_ST7789_WHITE,TFT_ST7789_BLACK,false,false);
    (void)TFT_ST7789_DrawInt32(&tft,SPEC_X+SPEC_W-54,SPEC_Y+SPEC_H+2,(int32_t)(0.5F*sample_rate_hz+0.5F),TFT_ST7789_FONT_6X12,TFT_ST7789_WHITE,TFT_ST7789_BLACK,false);
    (void)TFT_ST7789_DrawString(&tft,SPEC_X+SPEC_W-12,SPEC_Y+SPEC_H+2,"Hz",TFT_ST7789_FONT_6X12,TFT_ST7789_WHITE,TFT_ST7789_BLACK,false,false);
}
/* [COPY END: SPECTRUM_DISPLAY] */
int main(void){SYSCFG_DL_init();if(SignalTFTST7789_MSPM0_Init(&tft,TFT_ST7789_ROTATION_270,0U,0U)!=TFT_ST7789_OK)while(true){} while(true){/* Populate fft_magnitude[] using 20_fft_analysis, then call once. */Spectrum_Draw((float)SIGNAL_SAMPLE_RATE_HZ);}}
