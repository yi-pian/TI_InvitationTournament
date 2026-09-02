function Get-Round1IntegrationAlgorithmSources {
    return @(
        '11_legacy_compatibility\algorithms\03_measurement\adc_to_voltage\signal_adc_to_voltage.c',
        '11_legacy_compatibility\algorithms\03_measurement\mean\signal_mean.c',
        '11_legacy_compatibility\algorithms\03_measurement\minmax\signal_minmax.c',
        '11_legacy_compatibility\algorithms\03_measurement\vpp\signal_vpp.c',
        '11_legacy_compatibility\algorithms\03_measurement\rms\signal_rms.c',
        '11_legacy_compatibility\algorithms\03_measurement\ac_rms\signal_ac_rms.c',
        '03_measurement\frequency_zero_cross\signal_zero_cross.c',
        '03_measurement\phase\signal_phase.c',
        '11_legacy_compatibility\algorithms\04_dsp\remove_dc\signal_remove_dc.c',
        '04_dsp\window\signal_window.c',
        '04_dsp\window\hann\signal_hann.c',
        '11_legacy_compatibility\algorithms\04_dsp\fft\signal_fft.c',
        '11_legacy_compatibility\algorithms\04_dsp\fft_magnitude\signal_fft_magnitude.c',
        '11_legacy_compatibility\algorithms\04_dsp\peak_detect\signal_peak_detect.c',
        '04_dsp\correlation\signal_correlation.c',
        '04_dsp\harmonic\signal_harmonic.c',
        '04_dsp\thd\signal_thd.c',
        '05_precision\zero_cross_interpolation\signal_zero_cross_interpolation.c',
        '11_legacy_compatibility\algorithms\05_precision\multi_cycle_average\signal_multi_cycle_average.c',
        '05_precision\fft_parabolic_interpolation\signal_fft_parabolic_interpolation.c',
        '05_precision\window_gain_correction\signal_window_gain_correction.c',
        '05_precision\multi_bin_energy\signal_multi_bin_energy.c'
    )
}

function Get-Round1IntegrationTargets {
    $integrationAlgorithms = Get-Round1IntegrationAlgorithmSources
    return @(
        [pscustomobject]@{
            Name = 'signal_meter_round1'
            DisplayName = 'Signal Meter'
            AppDirectory = 'signal_meter'
            Profile = 'PROFILE_01_ADC_CAPTURE'
            Defines = @()
            ContestSources = @(
                '08_applications\common\signal_integration.c',
                '02_acquisition\adc_dma\signal_adc_dma.c'
            )
            AlgorithmSources = $integrationAlgorithms
        },
        [pscustomobject]@{
            Name = 'frequency_meter_a_round1'
            DisplayName = 'Frequency Meter A'
            AppDirectory = 'frequency_meter'
            Profile = 'PROFILE_05_FREQUENCY'
            Defines = @('SIGNAL_FREQUENCY_METHOD=1')
            ContestSources = @(
                '08_applications\common\mspm0g3507\signal_mspm0g3507_capture_platform.c',
                '02_acquisition\timer_capture\signal_timer_capture.c'
            )
            AlgorithmSources = @()
        },
        [pscustomobject]@{
            Name = 'frequency_meter_b_round1'
            DisplayName = 'Frequency Meter B'
            AppDirectory = 'frequency_meter'
            Profile = 'PROFILE_01_ADC_CAPTURE'
            Defines = @('SIGNAL_FREQUENCY_METHOD=2')
            ContestSources = @(
                '08_applications\common\signal_integration.c',
                '02_acquisition\adc_dma\signal_adc_dma.c'
            )
            AlgorithmSources = $integrationAlgorithms
        },
        [pscustomobject]@{
            Name = 'frequency_meter_c_round1'
            DisplayName = 'Frequency Meter C'
            AppDirectory = 'frequency_meter'
            Profile = 'PROFILE_01_ADC_CAPTURE'
            Defines = @('SIGNAL_FREQUENCY_METHOD=3')
            ContestSources = @(
                '08_applications\common\signal_integration.c',
                '02_acquisition\adc_dma\signal_adc_dma.c'
            )
            AlgorithmSources = $integrationAlgorithms
        },
        [pscustomobject]@{
            Name = 'spectrum_analyzer_round1'
            DisplayName = 'Spectrum Analyzer'
            AppDirectory = 'spectrum_analyzer'
            Profile = 'PROFILE_01_ADC_CAPTURE'
            Defines = @()
            ContestSources = @(
                '08_applications\common\signal_integration.c',
                '02_acquisition\adc_dma\signal_adc_dma.c'
            )
            AlgorithmSources = $integrationAlgorithms
        },
        [pscustomobject]@{
            Name = 'harmonic_thd_analyzer_round1'
            DisplayName = 'THD Analyzer'
            AppDirectory = 'harmonic_thd_analyzer'
            Profile = 'PROFILE_01_ADC_CAPTURE'
            Defines = @()
            ContestSources = @(
                '08_applications\common\signal_integration.c',
                '02_acquisition\adc_dma\signal_adc_dma.c'
            )
            AlgorithmSources = $integrationAlgorithms
        },
        [pscustomobject]@{
            Name = 'dual_channel_phase_meter_round1'
            DisplayName = 'Phase Meter'
            AppDirectory = 'dual_channel_phase_meter'
            Profile = 'PROFILE_02_DUAL_ADC'
            Defines = @()
            ContestSources = @(
                '08_applications\common\signal_integration.c',
                '08_applications\common\signal_dual_adc_platform.c'
            )
            AlgorithmSources = $integrationAlgorithms
        },
        [pscustomobject]@{
            Name = 'dds_generator_round1'
            DisplayName = 'DDS Generator'
            AppDirectory = 'dds_generator'
            Profile = 'PROFILE_03_DAC_GENERATOR'
            Defines = @()
            ContestSources = @(
                '08_applications\common\signal_dac_dma_platform.c',
                '06_generator\dac_dma\signal_dac_dma.c',
                '06_generator\dac_wave_table\signal_dac_wave_table.c',
                '06_generator\dds\signal_dds.c',
                '06_generator\sine\signal_sine.c'
            )
            AlgorithmSources = @()
        }
    )
}
