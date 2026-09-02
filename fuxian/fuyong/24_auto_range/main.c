/* 24_auto_range: software display range; hardware gain hook is intentionally empty. */
#include <stdint.h>
#include <math.h>
#include "signal_config.h"
static float voltage_samples[SIGNAL_SAMPLE_COUNT];
static float display_center_v=1.65F;
static float display_half_range_v=1.0F;
static uint8_t hardware_gain_index;
/* [COPY START: AUTO_RANGE]
 * [INPUT] voltage_samples[] float V.
 * [OUTPUT] display_center_v V、display_half_range_v V、hardware_gain_index。
 * [DRAW] y 映射必须使用 (value-display_center_v)/display_half_range_v。
 * [WHY] 以 min/max 中点为中心，避免 1.65 V ADC 偏置占掉大部分显示量程。
 * Dependency: math only. Single-frame unique: YES. */
static void AutoRange_Update(void){
    uint32_t i;
    float minimum=voltage_samples[0];
    float maximum=voltage_samples[0];
    for(i=1U;i<SIGNAL_SAMPLE_COUNT;++i){
        if(voltage_samples[i]<minimum)minimum=voltage_samples[i];
        if(voltage_samples[i]>maximum)maximum=voltage_samples[i];
    }
    display_center_v=0.5F*(minimum+maximum);
    display_half_range_v=fmaxf(0.01F,0.625F*(maximum-minimum));
    if(display_half_range_v<0.20F)
        hardware_gain_index=2U;
    else if(display_half_range_v<1.0F)
        hardware_gain_index=1U;
    else hardware_gain_index=0U;
}
/* [COPY END: AUTO_RANGE] */
int main(void){while(1){/* ADC conversion fills voltage_samples[] first. */AutoRange_Update();}}
