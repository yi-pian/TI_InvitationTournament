#ifndef SIGNAL_INTEGRATION_H
#define SIGNAL_INTEGRATION_H

#include <stddef.h>
#include <stdint.h>

#include "signal_algorithm_status.h"
#include "signal_complex.h"
#include "signal_harmonic.h"
#include "signal_zero_cross.h"

#define SIGNAL_INTEGRATION_MAX_PEAKS (8U)
#define SIGNAL_METER_MEASURE_DC        (1UL << 0)
#define SIGNAL_METER_MEASURE_MIN_MAX   (1UL << 1)
#define SIGNAL_METER_MEASURE_VPP       (1UL << 2)
#define SIGNAL_METER_MEASURE_RMS       (1UL << 3)
#define SIGNAL_METER_MEASURE_AC_RMS    (1UL << 4)
#define SIGNAL_METER_MEASURE_FREQUENCY (1UL << 5)

typedef struct {
    float dc_v;
    float minimum_v;
    float maximum_v;
    float vpp_v;
    float rms_v;
    float ac_rms_v;
    float frequency_hz;
    uint8_t frequency_valid;
} signal_meter_result_t;

typedef struct {
    float frequency_hz;
    float fractional_bin;
    float main_peak_v;
    float coherent_gain;
    uint32_t main_peak_bin;
    uint32_t peak_count;
    uint32_t peak_bins[SIGNAL_INTEGRATION_MAX_PEAKS];
    float peak_frequencies_hz[SIGNAL_INTEGRATION_MAX_PEAKS];
    float peak_amplitudes_v[SIGNAL_INTEGRATION_MAX_PEAKS];
} signal_spectrum_integration_result_t;

typedef struct {
    float fundamental_frequency_hz;
    float fundamental_amplitude_v;
    float harmonic_amplitude_v[6U];
    float thd_percent;
} signal_thd_integration_result_t;

typedef struct {
    float fft_phase_deg;
    float correlation_phase_deg;
    float correlation_coefficient;
    int32_t correlation_lag_samples;
} signal_phase_integration_result_t;

signal_algorithm_status_t SignalIntegration_RawToVoltage(
    const uint16_t *raw,
    size_t count,
    uint8_t adc_bits,
    float reference_voltage_v,
    float input_scale,
    float offset_voltage_v,
    float *voltage_v,
    size_t voltage_capacity);

signal_algorithm_status_t SignalIntegration_FrequencyTime(
    float *voltage_v,
    size_t count,
    float sample_rate_hz,
    float hysteresis_v,
    signal_zero_cross_event_t *events,
    size_t event_capacity,
    float *crossing_positions,
    size_t position_capacity,
    float *frequency_hz);

signal_algorithm_status_t SignalIntegration_SignalMeter(
    const uint16_t *raw,
    size_t count,
    uint8_t adc_bits,
    float reference_voltage_v,
    float input_scale,
    float offset_voltage_v,
    float sample_rate_hz,
    uint32_t measurement_mask,
    float crossing_hysteresis_v,
    float *voltage_workspace,
    size_t voltage_capacity,
    signal_zero_cross_event_t *events,
    size_t event_capacity,
    float *crossing_positions,
    size_t position_capacity,
    signal_meter_result_t *result);

signal_algorithm_status_t SignalIntegration_Spectrum(
    float *voltage_workspace,
    size_t count,
    float sample_rate_hz,
    float expected_min_hz,
    float expected_max_hz,
    signal_complex_f32_t *fft_workspace,
    size_t fft_capacity,
    float *magnitude_workspace,
    size_t magnitude_capacity,
    uint32_t requested_peak_count,
    signal_spectrum_integration_result_t *result);

signal_algorithm_status_t SignalIntegration_THD(
    float *voltage_workspace,
    size_t count,
    float sample_rate_hz,
    float expected_min_hz,
    float expected_max_hz,
    uint32_t harmonic_bin_radius,
    signal_complex_f32_t *fft_workspace,
    size_t fft_capacity,
    float *magnitude_workspace,
    size_t magnitude_capacity,
    signal_thd_integration_result_t *result);

signal_algorithm_status_t SignalIntegration_DualPhase(
    float *channel_a_v,
    float *channel_b_v,
    size_t count,
    float sample_rate_hz,
    float signal_frequency_hz,
    uint32_t maximum_lag_samples,
    signal_complex_f32_t *fft_a,
    signal_complex_f32_t *fft_b,
    size_t fft_capacity,
    float *correlation_workspace,
    size_t correlation_capacity,
    signal_phase_integration_result_t *result);

#endif /* SIGNAL_INTEGRATION_H */
