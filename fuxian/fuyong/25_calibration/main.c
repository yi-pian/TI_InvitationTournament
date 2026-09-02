/* 25_calibration: two-point ADC voltage and fixed dual-channel delay calibration. */
#include <stdbool.h>
#include <stdint.h>
#include "signal_config.h"
#include "signal_adc_gain_offset_calibration.h"
#include "signal_channel_delay_calibration.h"
static float voltage_samples[SIGNAL_SAMPLE_COUNT], calibrated_samples[SIGNAL_SAMPLE_COUNT];
static float phase_deg, delay_s;
static signal_adc_gain_offset_calibration_t adc_calibration;
static signal_channel_delay_calibration_t delay_calibration;
/* [COPY START: ADC_GAIN_OFFSET_CALIBRATION]
 * [INPUT] two measured/reference voltages and voltage_samples[] V.
 * [OUTPUT] calibrated_samples[] V. Dependency: signal_adc_gain_offset_calibration. Single-frame unique: YES. */
static bool Calibration_ApplyADC(float measured_low_v,float true_low_v,float measured_high_v,float true_high_v)
{return SignalADCGainOffsetCalibration_Compute(measured_low_v,true_low_v,measured_high_v,true_high_v,&adc_calibration)==SIGNAL_ALGORITHM_OK&&SignalADCGainOffsetCalibration_Apply(voltage_samples,calibrated_samples,SIGNAL_SAMPLE_COUNT,&adc_calibration)==SIGNAL_ALGORITHM_OK;}
/* [COPY END: ADC_GAIN_OFFSET_CALIBRATION] */
/* [COPY START: CHANNEL_DELAY_CALIBRATION]
 * [INPUT] measured phase B-A, expected phase B-A, frequency_hz. [OUTPUT] phase_deg, delay_s.
 * Dependency: signal_channel_delay_calibration. Single-frame unique: NO; calibration coefficients may persist. */
static bool Calibration_ApplyDelay(float measured_phase_deg,float expected_phase_deg,float frequency_hz)
{if(SignalChannelDelayCalibration_Compute(measured_phase_deg,expected_phase_deg,frequency_hz,&delay_calibration)!=SIGNAL_ALGORITHM_OK)return false;delay_s=delay_calibration.delay_b_relative_to_a_s;return SignalChannelDelayCalibration_Apply(measured_phase_deg,frequency_hz,&delay_calibration,&phase_deg)==SIGNAL_ALGORITHM_OK;}
/* [COPY END: CHANNEL_DELAY_CALIBRATION] */
int main(void){while(true){/* Obtain known references, then call the required calibration function. */
    if(false){(void)Calibration_ApplyADC(0.0F,0.0F,1.0F,1.0F);
        (void)Calibration_ApplyDelay(0.0F,0.0F,1000.0F);}}}
