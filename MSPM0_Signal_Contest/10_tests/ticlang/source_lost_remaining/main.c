#include <stdint.h>

#include "signal_coherent_sampling.h"
#include "signal_czt.h"
#include "signal_dc_measure.h"
#include "signal_fft_peak.h"
#include "signal_frequency_response_correction.h"
#include "signal_jacobsen_interpolation.h"
#include "signal_macleod_interpolation.h"
#include "signal_quinn_interpolation.h"

static signal_complex_f32_t s_spectrum[11];
static signal_complex_f32_t s_czt_output[3];
static float s_samples[8] = {0.0F, 1.0F, 0.0F, -1.0F, 0.0F, 1.0F, 0.0F, -1.0F};
static float s_magnitude[4] = {0.0F, 1.0F, 3.0F, 2.0F};
static uint16_t s_raw[4] = {1000U, 1100U, 1200U, 1300U};

int main(void)
{
    signal_algorithm_status_t status;
    signal_jacobsen_result_t jacobsen;
    signal_quinn_result_t quinn;
    signal_macleod_result_t macleod;
    signal_coherent_sampling_result_t coherent;
    signal_frequency_response_correction_result_t correction;
    signal_dc_measure_result_t dc;
    signal_fft_peak_result_t peak;
    signal_adc_to_voltage_config_t adc = {4095U, 3.3F, 1.0F, 0.0F};
    static const signal_frequency_response_correction_point_t table[2] = {
        {100.0F, 1.1F, 2.0F}, {1000.0F, 0.9F, -3.0F}
    };

    s_spectrum[8].real = -10.940022F;
    s_spectrum[8].imag = 27.591763F;
    s_spectrum[9].real = 20.908211F;
    s_spectrum[9].imag = -46.005161F;
    s_spectrum[10].real = 6.253964F;
    s_spectrum[10].imag = -12.141195F;

    status = SignalJacobsen_Process(s_spectrum, 11U, 9U, 48000.0F, 64U,
                                    &jacobsen);
    if (status == SIGNAL_ALGORITHM_OK)
    {
        status = SignalQuinnSecond_Process(s_spectrum, 11U, 9U, 48000.0F,
                                           64U, &quinn);
    }
    if (status == SIGNAL_ALGORITHM_OK)
    {
        status = SignalMacleod_Process(s_spectrum, 11U, 9U, 48000.0F, 64U,
                                       &macleod);
    }
    if (status == SIGNAL_ALGORITHM_OK)
    {
        status = SignalCoherentSampling_FindNearest(1000.0F, 48000.0F, 1024U,
                                                     1U, 511U, true, &coherent);
    }
    if (status == SIGNAL_ALGORITHM_OK)
    {
        status = SignalFrequencyResponseCorrection_Process(
            table, 2U, 500.0F, 2.0F, 10.0F, SIGNAL_FRC_INTERPOLATE_LOG_HZ,
            SIGNAL_FRC_RANGE_REJECT, &correction);
    }
    if (status == SIGNAL_ALGORITHM_OK)
    {
        status = SignalCZT_UnitCircleRealDirect(s_samples, 8U, 8000.0F,
                                                500.0F, 100.0F, s_czt_output,
                                                3U, 3U);
    }
    if (status == SIGNAL_ALGORITHM_OK)
    {
        status = SignalDCMeasure_FromRawLinear(s_raw, 4U, &adc, &dc);
    }
    if (status == SIGNAL_ALGORITHM_OK)
    {
        status = SignalFFTPeak_Process(s_magnitude, 4U, 1U, 3U, 8000.0F,
                                       8U, &peak);
    }
    return (status == SIGNAL_ALGORITHM_OK) ? 0 : 1;
}
