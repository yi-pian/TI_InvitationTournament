/* 21_waveform_display: dual-channel time/XY drawing only. */
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "ti_msp_dl_config.h"
#include "signal_config.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_tft_st7789.h"
#include "signal_tft_st7789_mspm0g3507.h"
#define GRAPH_X 8
#define GRAPH_Y 48
#define GRAPH_W 304
#define GRAPH_H 160
static uint16_t adc_ch1_samples[SIGNAL_SAMPLE_COUNT], adc_ch2_samples[SIGNAL_SAMPLE_COUNT];
static float voltage_samples[SIGNAL_SAMPLE_COUNT], voltage_ch2_samples[SIGNAL_SAMPLE_COUNT];
static tft_st7789_t tft;
static const signal_dual_adc_config_t s_adc = { SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U };

static bool AcquireDualADCFrame(void) { if (SignalDualADC_Start(adc_ch1_samples, adc_ch2_samples, SIGNAL_SAMPLE_COUNT) != SIGNAL_RESULT_OK) return false; while (!SignalDualADC_IsFinished()) { __WFI(); } return true; }
/* [COPY START: WAVEFORM_PREPARE]
 * [INPUT] adc_ch1_samples[]/adc_ch2_samples[] uint16 ADC code.
 * [OUTPUT] voltage_samples[]/voltage_ch2_samples[] float V. Dependency: signal_config. Single-frame unique: YES. */
static void Waveform_ConvertToVoltage(void) { uint32_t i; for(i=0U;i<SIGNAL_SAMPLE_COUNT;++i){ voltage_samples[i]=(float)adc_ch1_samples[i]*SIGNAL_ADC_VREF_V/4095.0F; voltage_ch2_samples[i]=(float)adc_ch2_samples[i]*SIGNAL_ADC_VREF_V/4095.0F; } }
/* [COPY END: WAVEFORM_PREPARE] */
static int32_t Waveform_Y(float value, float center, float half_range) { return GRAPH_Y + GRAPH_H / 2 - (int32_t)((value-center)*(float)(GRAPH_H/2)/half_range); }
/* [COPY START: WAVEFORM_DRAW]
 * [INPUT] voltage_samples[] + voltage_ch2_samples[] V, tft. [OUTPUT] TFT time trace or XY trace.
 * [AUTO RANGE] 两路共用 min/max；center=(max+min)/2，half_range=1.25*(max-min)/2。
 * 绘图必须使用 value-center，不能直接按相对 0 V 的绝对值缩放，否则带 1.65 V
 * ADC 偏置的交流波形会挤在屏幕上半部。
 * Dependency: signal_tft_st7789. Single-frame unique: YES (drawing only; no FFT/filter). */
static void Waveform_Draw(bool xy_mode) { uint32_t x; float minimum=voltage_samples[0],maximum=voltage_samples[0],center,half_range; for(x=0U;x<SIGNAL_SAMPLE_COUNT;++x){if(voltage_samples[x]<minimum)minimum=voltage_samples[x];if(voltage_samples[x]>maximum)maximum=voltage_samples[x];if(voltage_ch2_samples[x]<minimum)minimum=voltage_ch2_samples[x];if(voltage_ch2_samples[x]>maximum)maximum=voltage_ch2_samples[x];} center=0.5F*(minimum+maximum);half_range=fmaxf(0.01F,0.625F*(maximum-minimum));
 (void)TFT_ST7789_FillRect(&tft,GRAPH_X,GRAPH_Y,GRAPH_W,GRAPH_H,TFT_ST7789_BLACK);
 for(x=1U;x<(uint32_t)GRAPH_W;++x){uint32_t i0=(x-1U)*SIGNAL_SAMPLE_COUNT/GRAPH_W,i1=x*SIGNAL_SAMPLE_COUNT/GRAPH_W;int32_t x0,x1,y0,y1;
  if(xy_mode){x0=GRAPH_X+GRAPH_W/2+(int32_t)((voltage_samples[i0]-center)*(GRAPH_W/2)/half_range);x1=GRAPH_X+GRAPH_W/2+(int32_t)((voltage_samples[i1]-center)*(GRAPH_W/2)/half_range);y0=Waveform_Y(voltage_ch2_samples[i0],center,half_range);y1=Waveform_Y(voltage_ch2_samples[i1],center,half_range);(void)TFT_ST7789_DrawLine(&tft,x0,y0,x1,y1,TFT_ST7789_CYAN);}
  else {x0=GRAPH_X+(int32_t)x-1;x1=GRAPH_X+(int32_t)x;y0=Waveform_Y(voltage_samples[i0],center,half_range);y1=Waveform_Y(voltage_samples[i1],center,half_range);(void)TFT_ST7789_DrawLine(&tft,x0,y0,x1,y1,TFT_ST7789_YELLOW);y0=Waveform_Y(voltage_ch2_samples[i0],center,half_range);y1=Waveform_Y(voltage_ch2_samples[i1],center,half_range);(void)TFT_ST7789_DrawLine(&tft,x0,y0,x1,y1,TFT_ST7789_CYAN);}}
 }
/* [COPY END: WAVEFORM_DRAW] */
int main(void) { SYSCFG_DL_init(); if(SignalDualADC_Init(&s_adc)!=SIGNAL_RESULT_OK||SignalTFTST7789_MSPM0_Init(&tft,TFT_ST7789_ROTATION_270,0U,0U)!=TFT_ST7789_OK)while(true){} while(true)if(AcquireDualADCFrame()){Waveform_ConvertToVoltage();Waveform_Draw(false);} }
