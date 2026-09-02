/* 23_trigger_capture: software edge trigger and pre/post-trigger extraction. */
#include <stdbool.h>
#include <stdint.h>
#include "ti_msp_dl_config.h"
#include "signal_config.h"
#include "signal_dual_adc_mspm0g3507.h"
#include "signal_trigger_capture.h"
#define CAPTURE_COUNT SIGNAL_SAMPLE_COUNT
#define PRETRIGGER_COUNT (CAPTURE_COUNT/4U)
static uint16_t adc_samples[CAPTURE_COUNT], adc_unused_samples[CAPTURE_COUNT];
static uint16_t captured_samples[CAPTURE_COUNT];
static uint32_t trigger_index;
static const signal_dual_adc_config_t s_adc={SIGNAL_SAMPLE_RATE_HZ,CPUCLK_FREQ,65536U};
static bool AcquireADCFrame(void){if(SignalDualADC_Start(adc_samples,adc_unused_samples,CAPTURE_COUNT)!=SIGNAL_RESULT_OK)return false;while(!SignalDualADC_IsFinished()){__WFI();}return true;}
/* [COPY START: TRIGGER_CAPTURE]
 * [INPUT] adc_samples[] uint16 ADC code; level/hysteresis code; PRETRIGGER_COUNT.
 * [OUTPUT] captured_samples[] containing pretrigger + posttrigger data; trigger_index.
 * Dependency: signal_trigger_capture. Single-frame unique: YES. */
static bool Trigger_Capture(uint16_t level,uint16_t hysteresis)
{signal_trigger_config_t cfg={level,hysteresis,SIGNAL_TRIGGER_RISING};size_t index;if(SignalTrigger_Find(adc_samples,CAPTURE_COUNT,&cfg,0U,&index)!=SIGNAL_RESULT_OK)return false;if(SignalTrigger_Extract(adc_samples,CAPTURE_COUNT,index,PRETRIGGER_COUNT,captured_samples,CAPTURE_COUNT)!=SIGNAL_RESULT_OK)return false;trigger_index=(uint32_t)index;return true;}
/* [COPY END: TRIGGER_CAPTURE] */
int main(void){SYSCFG_DL_init();if(SignalDualADC_Init(&s_adc)!=SIGNAL_RESULT_OK)while(true){}while(true)if(AcquireADCFrame())(void)Trigger_Capture(2048U,32U);}
