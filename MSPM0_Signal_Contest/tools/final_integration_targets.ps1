if (-not (Get-Command Get-Round1IntegrationAlgorithmSources -ErrorAction SilentlyContinue)) {
    . (Join-Path $PSScriptRoot 'round1_integration_targets.ps1')
}

function Get-FinalIntegrationTargets {
    $integrationAlgorithms = Get-Round1IntegrationAlgorithmSources
    return @(
        [pscustomobject]@{
            Name = 'sweep_analyzer_final'
            DisplayName = 'Sweep Analyzer'
            AppDirectory = 'sweep_analyzer'
            Profile = 'PROFILE_04_ADC_DAC'
            Defines = @()
            FftBackend = 0
            MathBackend = 0
            ContestSources = @(
                '08_applications\sweep_analyzer\signal_sweep_analyzer.c',
                '08_applications\common\signal_dac_dma_platform.c',
                '02_acquisition\adc_dma\signal_adc_dma.c',
                '06_generator\dac_dma\signal_dac_dma.c',
                '06_generator\dac_wave_table\signal_dac_wave_table.c',
                '06_generator\dds\signal_dds.c',
                '06_generator\sine\signal_sine.c',
                '06_generator\frequency_sweep\signal_frequency_sweep.c'
            )
            AlgorithmSources = @(
                '03_measurement\adc_to_voltage\signal_adc_to_voltage.c',
                '05_precision\lock_in\signal_lock_in.c'
            )
        },
        [pscustomobject]@{
            Name = 'waveform_capture_replay_final'
            DisplayName = 'Wave Capture Replay'
            AppDirectory = 'waveform_capture_replay'
            Profile = 'PROFILE_04_ADC_DAC'
            Defines = @()
            FftBackend = 0
            MathBackend = 0
            ContestSources = @(
                '08_applications\waveform_capture_replay\signal_waveform_capture_replay.c',
                '08_applications\common\signal_dac_dma_platform.c',
                '02_acquisition\adc_dma\signal_adc_dma.c',
                '02_acquisition\adc_ring_buffer\signal_adc_ring_buffer.c',
                '02_acquisition\trigger_capture\signal_trigger_capture.c',
                '06_generator\dac_dma\signal_dac_dma.c',
                '06_generator\arbitrary_wave\signal_arbitrary_wave.c'
            )
            AlgorithmSources = @()
        },
        [pscustomobject]@{
            Name = 'signal_analyzer_final'
            DisplayName = 'Signal Analyzer'
            AppDirectory = 'signal_analyzer'
            Profile = 'PROFILE_02_DUAL_ADC'
            Defines = @()
            FftBackend = 2
            MathBackend = 0
            ContestSources = @(
                '08_applications\signal_analyzer\signal_analyzer_pipeline.c',
                '08_applications\common\signal_integration.c',
                '08_applications\common\signal_dual_adc_platform.c'
            )
            AlgorithmSources = @($integrationAlgorithms) + @(
                '04_dsp\snr\signal_snr.c',
                '04_dsp\sfdr\signal_sfdr.c'
            )
        },
        [pscustomobject]@{
            Name = 'signal_contest_template_final'
            DisplayName = 'Signal Contest Template'
            AppDirectory = 'signal_contest_template'
            Profile = 'PROFILE_06_FULL_SIGNAL'
            Defines = @()
            FftBackend = 2
            MathBackend = 0
            ContestSources = @(
                '08_applications\signal_contest_template\signal_pipeline.c',
                '08_applications\common\signal_integration.c',
                '08_applications\common\signal_dual_adc_platform.c'
            )
            AlgorithmSources = @($integrationAlgorithms)
        }
    )
}
