#include "signal_integration.h"

#include <limits.h>
#include <math.h>

#include "signal_ac_rms.h"
#include "signal_adc_to_voltage.h"
#include "signal_correlation.h"
#include "signal_fft.h"
#include "signal_fft_magnitude.h"
#include "signal_fft_parabolic_interpolation.h"
#include "signal_hann.h"
#include "signal_mean.h"
#include "signal_minmax.h"
#include "signal_multi_cycle_average.h"
#include "signal_peak_detect.h"
#include "signal_phase.h"
#include "signal_remove_dc.h"
#include "signal_rms.h"
#include "signal_thd.h"
#include "signal_vpp.h"
#include "signal_window_gain_correction.h"
#include "signal_zero_cross_interpolation.h"

static signal_algorithm_status_t Integration_CountToU32(
    size_t count, uint32_t *count_u32)
{
    if ((count_u32 == NULL) || (count == 0U) || (count > UINT32_MAX)) {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    *count_u32 = (uint32_t) count;
    return SIGNAL_ALGORITHM_OK;
}

static uint32_t Integration_ClampBin(float frequency_hz,
    float sample_rate_hz, uint32_t fft_size, uint32_t maximum_bin)
{
    float exact_bin = frequency_hz * (float) fft_size / sample_rate_hz;
    uint32_t bin;

    if (exact_bin <= 0.0f) {
        return 0U;
    }
    bin = (uint32_t) exact_bin;
    return (bin > maximum_bin) ? maximum_bin : bin;
}

signal_algorithm_status_t SignalIntegration_RawToVoltage(
    const uint16_t *raw, size_t count, uint8_t adc_bits,
    float reference_voltage_v, float input_scale, float offset_voltage_v,
    float *voltage_v, size_t voltage_capacity)
{
    signal_adc_to_voltage_config_t config;
    uint32_t count_u32;

    if ((raw == NULL) || (voltage_v == NULL) ||
        (voltage_capacity < count) || (adc_bits == 0U) ||
        (adc_bits > 16U)) {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (Integration_CountToU32(count, &count_u32) != SIGNAL_ALGORITHM_OK) {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }

    config.adc_max_code = (1UL << adc_bits) - 1UL;
    config.reference_voltage_v = reference_voltage_v;
    config.input_scale = input_scale;
    config.offset_voltage_v = offset_voltage_v;
    return SignalADCToVoltage_Process(raw, voltage_v, count_u32, &config);
}

signal_algorithm_status_t SignalIntegration_FrequencyTime(
    float *voltage_v, size_t count, float sample_rate_hz,
    float hysteresis_v, signal_zero_cross_event_t *events,
    size_t event_capacity, float *crossing_positions,
    size_t position_capacity, float *frequency_hz)
{
    signal_remove_dc_result_t dc_result;
    signal_zero_cross_config_t crossing_config;
    signal_zero_cross_result_t crossing_result;
    signal_zero_cross_interpolation_result_t interpolation_result;
    signal_multi_cycle_average_result_t average_result;
    signal_algorithm_status_t status;
    uint32_t count_u32;

    if ((frequency_hz == NULL) || (events == NULL) ||
        (crossing_positions == NULL) || (event_capacity == 0U) ||
        (event_capacity > UINT32_MAX) || (position_capacity > UINT32_MAX) ||
        !(sample_rate_hz > 0.0f)) {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    status = Integration_CountToU32(count, &count_u32);
    if (status != SIGNAL_ALGORITHM_OK) {
        return status;
    }

    status = SignalRemoveDC_Process(
        voltage_v, voltage_v, count_u32, &dc_result);
    if (status != SIGNAL_ALGORITHM_OK) {
        return status;
    }

    crossing_config.threshold_v = 0.0f;
    crossing_config.hysteresis_v = hysteresis_v;
    crossing_config.direction = SIGNAL_ZERO_CROSS_RISING;
    status = SignalZeroCross_Process(voltage_v, count_u32, &crossing_config,
        events, (uint32_t) event_capacity, &crossing_result);
    if (status != SIGNAL_ALGORITHM_OK) {
        return status;
    }

    status = SignalZeroCrossInterpolation_Process(voltage_v, count_u32,
        crossing_config.threshold_v, events, crossing_result.event_count,
        crossing_positions, (uint32_t) position_capacity,
        &interpolation_result);
    if (status != SIGNAL_ALGORITHM_OK) {
        return status;
    }

    status = SignalMultiCycleAverage_Process(crossing_positions,
        interpolation_result.position_count, sample_rate_hz, &average_result);
    if (status == SIGNAL_ALGORITHM_OK) {
        *frequency_hz = average_result.frequency_hz;
    }
    return status;
}

signal_algorithm_status_t SignalIntegration_SignalMeter(
    const uint16_t *raw, size_t count, uint8_t adc_bits,
    float reference_voltage_v, float input_scale, float offset_voltage_v,
    float sample_rate_hz, uint32_t measurement_mask,
    float crossing_hysteresis_v, float *voltage_workspace,
    size_t voltage_capacity, signal_zero_cross_event_t *events,
    size_t event_capacity, float *crossing_positions,
    size_t position_capacity, signal_meter_result_t *result)
{
    signal_mean_result_t mean_result;
    signal_minmax_result_t minmax_result;
    signal_vpp_result_t vpp_result;
    signal_rms_result_t rms_result;
    signal_ac_rms_result_t ac_rms_result;
    signal_algorithm_status_t status;
    uint32_t count_u32;

    if (result == NULL) {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    status = Integration_CountToU32(count, &count_u32);
    if (status != SIGNAL_ALGORITHM_OK) {
        return status;
    }
    status = SignalIntegration_RawToVoltage(raw, count, adc_bits,
        reference_voltage_v, input_scale, offset_voltage_v,
        voltage_workspace, voltage_capacity);
    if (status != SIGNAL_ALGORITHM_OK) {
        return status;
    }
    result->dc_v = 0.0f;
    result->minimum_v = 0.0f;
    result->maximum_v = 0.0f;
    result->vpp_v = 0.0f;
    result->rms_v = 0.0f;
    result->ac_rms_v = 0.0f;
    result->frequency_hz = 0.0f;
    result->frequency_valid = 0U;

    if ((measurement_mask & SIGNAL_METER_MEASURE_DC) != 0U) {
        status = SignalMean_Process(voltage_workspace, count_u32, &mean_result);
        if (status != SIGNAL_ALGORITHM_OK) return status;
        result->dc_v = mean_result.mean_value;
    }
    if ((measurement_mask & SIGNAL_METER_MEASURE_MIN_MAX) != 0U) {
        status = SignalMinMax_Process(
            voltage_workspace, count_u32, &minmax_result);
        if (status != SIGNAL_ALGORITHM_OK) return status;
        result->minimum_v = minmax_result.min_value;
        result->maximum_v = minmax_result.max_value;
    }
    if ((measurement_mask & SIGNAL_METER_MEASURE_VPP) != 0U) {
        status = SignalVPP_Process(voltage_workspace, count_u32, &vpp_result);
        if (status != SIGNAL_ALGORITHM_OK) return status;
        result->vpp_v = vpp_result.amplitude_vpp;
    }
    if ((measurement_mask & SIGNAL_METER_MEASURE_RMS) != 0U) {
        status = SignalRMS_Process(voltage_workspace, count_u32, &rms_result);
        if (status != SIGNAL_ALGORITHM_OK) return status;
        result->rms_v = rms_result.rms_v;
    }
    if ((measurement_mask & SIGNAL_METER_MEASURE_AC_RMS) != 0U) {
        status = SignalACRMS_Process(
            voltage_workspace, count_u32, &ac_rms_result);
        if (status != SIGNAL_ALGORITHM_OK) return status;
        result->ac_rms_v = ac_rms_result.ac_rms_v;
    }
    if ((measurement_mask & SIGNAL_METER_MEASURE_FREQUENCY) != 0U) {
        status = SignalIntegration_FrequencyTime(voltage_workspace, count,
            sample_rate_hz, crossing_hysteresis_v, events, event_capacity,
            crossing_positions, position_capacity, &result->frequency_hz);
        if (status != SIGNAL_ALGORITHM_OK) {
            return status;
        }
        result->frequency_valid = 1U;
    }
    return SIGNAL_ALGORITHM_OK;
}

signal_algorithm_status_t SignalIntegration_Spectrum(
    float *voltage_workspace, size_t count, float sample_rate_hz,
    float expected_min_hz, float expected_max_hz,
    signal_complex_f32_t *fft_workspace, size_t fft_capacity,
    float *magnitude_workspace, size_t magnitude_capacity,
    uint32_t requested_peak_count,
    signal_spectrum_integration_result_t *result)
{
    signal_remove_dc_result_t dc_result;
    signal_window_result_t window_result;
    signal_fft_magnitude_result_t magnitude_result;
    signal_peak_detect_result_t peak_result;
    signal_fft_parabolic_result_t interpolation;
    signal_algorithm_status_t status;
    uint32_t count_u32;
    uint32_t first_bin;
    uint32_t last_bin;
    uint32_t i;

    if ((result == NULL) || (fft_workspace == NULL) ||
        (magnitude_workspace == NULL) || (fft_capacity < count) ||
        !(sample_rate_hz > 0.0f) || !(expected_max_hz > expected_min_hz) ||
        (requested_peak_count > SIGNAL_INTEGRATION_MAX_PEAKS)) {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    status = Integration_CountToU32(count, &count_u32);
    if (status != SIGNAL_ALGORITHM_OK) return status;
    if (magnitude_capacity < ((count / 2U) + 1U)) {
        return SIGNAL_ALGORITHM_BUFFER_TOO_SMALL;
    }

    status = SignalRemoveDC_Process(
        voltage_workspace, voltage_workspace, count_u32, &dc_result);
    if (status != SIGNAL_ALGORITHM_OK) return status;
    status = SignalHann_Apply(voltage_workspace, voltage_workspace,
        count_u32, &window_result);
    if (status != SIGNAL_ALGORITHM_OK) return status;
    status = SignalFFT_ForwardReal(voltage_workspace, fft_workspace,
        count_u32, (uint32_t) fft_capacity);
    if (status != SIGNAL_ALGORITHM_OK) return status;
    status = SignalFFTMagnitude_Process(fft_workspace, count_u32,
        magnitude_workspace, (uint32_t) magnitude_capacity,
        &magnitude_result);
    if (status != SIGNAL_ALGORITHM_OK) return status;
    status = SignalWindowGainCorrection_Apply(magnitude_workspace,
        magnitude_workspace, magnitude_result.bin_count, count_u32,
        window_result.coherent_gain);
    if (status != SIGNAL_ALGORITHM_OK) return status;

    first_bin = Integration_ClampBin(
        expected_min_hz, sample_rate_hz, count_u32,
        magnitude_result.bin_count - 1U);
    if (first_bin < 1U) first_bin = 1U;
    last_bin = Integration_ClampBin(
        expected_max_hz, sample_rate_hz, count_u32,
        magnitude_result.bin_count - 2U);
    if (last_bin <= first_bin) return SIGNAL_ALGORITHM_OUT_OF_RANGE;

    status = SignalPeakDetect_Process(magnitude_workspace,
        magnitude_result.bin_count, first_bin, last_bin, &peak_result);
    if (status != SIGNAL_ALGORITHM_OK) return status;
    status = SignalFFTParabolicInterpolation_Process(magnitude_workspace,
        magnitude_result.bin_count, peak_result.peak_index, sample_rate_hz,
        count_u32, &interpolation);
    if (status != SIGNAL_ALGORITHM_OK) return status;

    result->frequency_hz = interpolation.frequency_hz;
    result->fractional_bin = interpolation.fractional_bin;
    result->main_peak_v = interpolation.interpolated_magnitude;
    result->coherent_gain = window_result.coherent_gain;
    result->main_peak_bin = peak_result.peak_index;
    result->peak_count = requested_peak_count;

    for (i = 0U; i < requested_peak_count; ++i) {
        uint32_t bin;
        uint32_t best_bin = first_bin;
        float best_value = -1.0f;
        for (bin = first_bin; bin <= last_bin; ++bin) {
            uint32_t j;
            uint8_t blocked = 0U;
            for (j = 0U; j < i; ++j) {
                uint32_t previous = result->peak_bins[j];
                if ((bin + 1U >= previous) && (bin <= previous + 1U)) {
                    blocked = 1U;
                    break;
                }
            }
            if ((blocked == 0U) && (magnitude_workspace[bin] > best_value)) {
                best_value = magnitude_workspace[bin];
                best_bin = bin;
            }
        }
        result->peak_bins[i] = best_bin;
        result->peak_frequencies_hz[i] =
            (float) best_bin * sample_rate_hz / (float) count_u32;
        result->peak_amplitudes_v[i] = best_value;
    }
    return SIGNAL_ALGORITHM_OK;
}

signal_algorithm_status_t SignalIntegration_THD(
    float *voltage_workspace, size_t count, float sample_rate_hz,
    float expected_min_hz, float expected_max_hz,
    uint32_t harmonic_bin_radius, signal_complex_f32_t *fft_workspace,
    size_t fft_capacity, float *magnitude_workspace,
    size_t magnitude_capacity, signal_thd_integration_result_t *result)
{
    signal_spectrum_integration_result_t spectrum;
    signal_harmonic_config_t harmonic_config;
    signal_harmonic_result_t harmonics;
    signal_thd_result_t thd;
    signal_algorithm_status_t status;
    uint32_t order;

    if (result == NULL) return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    status = SignalIntegration_Spectrum(voltage_workspace, count,
        sample_rate_hz, expected_min_hz, expected_max_hz, fft_workspace,
        fft_capacity, magnitude_workspace, magnitude_capacity, 1U, &spectrum);
    if (status != SIGNAL_ALGORITHM_OK) return status;

    harmonic_config.fundamental_frequency_hz = spectrum.frequency_hz;
    harmonic_config.first_order = 1U;
    harmonic_config.last_order = 5U;
    harmonic_config.radius_bins = harmonic_bin_radius;
    status = SignalHarmonic_Process(magnitude_workspace,
        (uint32_t) ((count / 2U) + 1U), sample_rate_hz, (uint32_t) count,
        &harmonic_config, &harmonics);
    if (status != SIGNAL_ALGORITHM_OK) return status;
    status = SignalTHD_Process(&harmonics, &thd);
    if (status != SIGNAL_ALGORITHM_OK) return status;

    result->fundamental_frequency_hz = spectrum.frequency_hz;
    result->fundamental_amplitude_v = harmonics.items[1U].root_sum_square;
    result->harmonic_amplitude_v[0U] = 0.0f;
    for (order = 1U; order <= 5U; ++order) {
        result->harmonic_amplitude_v[order] =
            harmonics.items[order].root_sum_square;
    }
    result->thd_percent = thd.thd_percent;
    return SIGNAL_ALGORITHM_OK;
}

signal_algorithm_status_t SignalIntegration_DualPhase(
    float *channel_a_v, float *channel_b_v, size_t count,
    float sample_rate_hz, float signal_frequency_hz,
    uint32_t maximum_lag_samples, signal_complex_f32_t *fft_a,
    signal_complex_f32_t *fft_b, size_t fft_capacity,
    float *correlation_workspace, size_t correlation_capacity,
    signal_phase_integration_result_t *result)
{
    signal_remove_dc_result_t dc_a;
    signal_remove_dc_result_t dc_b;
    signal_window_result_t window_a;
    signal_window_result_t window_b;
    signal_phase_result_t fft_phase;
    signal_phase_result_t correlation_phase;
    signal_correlation_result_t correlation;
    signal_algorithm_status_t status;
    uint32_t count_u32;
    uint32_t bin;

    if ((result == NULL) || (fft_a == NULL) || (fft_b == NULL) ||
        (correlation_workspace == NULL) || (fft_capacity < count) ||
        (correlation_capacity < (2U * maximum_lag_samples + 1U)) ||
        !(signal_frequency_hz > 0.0f) || !(sample_rate_hz > 0.0f)) {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    status = Integration_CountToU32(count, &count_u32);
    if (status != SIGNAL_ALGORITHM_OK) return status;

    status = SignalRemoveDC_Process(channel_a_v, channel_a_v,
        count_u32, &dc_a);
    if (status != SIGNAL_ALGORITHM_OK) return status;
    status = SignalRemoveDC_Process(channel_b_v, channel_b_v,
        count_u32, &dc_b);
    if (status != SIGNAL_ALGORITHM_OK) return status;
    status = SignalCorrelation_Process(channel_a_v, channel_b_v, count_u32,
        maximum_lag_samples, correlation_workspace,
        (uint32_t) correlation_capacity, &correlation);
    if (status != SIGNAL_ALGORITHM_OK) return status;
    status = SignalPhase_FromCorrelationLag(
        (float) correlation.best_lag_samples,
        sample_rate_hz / signal_frequency_hz, &correlation_phase);
    if (status != SIGNAL_ALGORITHM_OK) return status;

    status = SignalHann_Apply(channel_a_v, channel_a_v,
        count_u32, &window_a);
    if (status != SIGNAL_ALGORITHM_OK) return status;
    status = SignalHann_Apply(channel_b_v, channel_b_v,
        count_u32, &window_b);
    if (status != SIGNAL_ALGORITHM_OK) return status;
    status = SignalFFT_ForwardReal(
        channel_a_v, fft_a, count_u32, (uint32_t) fft_capacity);
    if (status != SIGNAL_ALGORITHM_OK) return status;
    status = SignalFFT_ForwardReal(
        channel_b_v, fft_b, count_u32, (uint32_t) fft_capacity);
    if (status != SIGNAL_ALGORITHM_OK) return status;

    bin = (uint32_t) lroundf(
        signal_frequency_hz * (float) count_u32 / sample_rate_hz);
    if ((bin == 0U) || (bin >= count_u32)) {
        return SIGNAL_ALGORITHM_OUT_OF_RANGE;
    }
    status = SignalPhase_FromFFTBin(
        fft_a, fft_b, count_u32, bin, &fft_phase);
    if (status != SIGNAL_ALGORITHM_OK) return status;

    result->fft_phase_deg = fft_phase.phase_difference_deg;
    result->correlation_phase_deg =
        correlation_phase.phase_difference_deg;
    result->correlation_coefficient = correlation.best_coefficient;
    result->correlation_lag_samples = correlation.best_lag_samples;
    return SIGNAL_ALGORITHM_OK;
}
