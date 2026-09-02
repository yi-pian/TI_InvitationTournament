param(
    [string]$Compiler = 'D:\ti\CCS\ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\bin\tiarmclang.exe',
    [string]$SdkRoot = 'C:\TI\mspm0_sdk_2_11_00_07',
    [string]$Gcc = 'gcc',
    [int]$FftBackend = 0,
    [string]$OutputDirectoryName = 'build'
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$algorithm = $repo
$sdkSource = Join-Path $SdkRoot 'source'
$build = Join-Path $repo (Join-Path '10_tests\integration' $OutputDirectoryName)

foreach ($required in @($Compiler, $sdkSource, $algorithm)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required path not found: $required"
    }
}
New-Item -ItemType Directory -Force -Path $build | Out-Null

$algorithmIncludes = @(
    foreach ($algorithmDirectory in @('03_measurement', '04_dsp', '05_precision')) {
        Get-ChildItem -LiteralPath (Join-Path $algorithm $algorithmDirectory) `
            -Recurse -Filter '*.h' -File |
            ForEach-Object { '-I' + $_.Directory.FullName }
    }
) | Sort-Object -Unique

$pcSources = @(
    (Join-Path $repo '10_tests\integration\test_integration_round1.c'),
    (Join-Path $repo '08_applications\common\signal_integration.c'),
    (Join-Path $algorithm '03_measurement\adc_to_voltage\signal_adc_to_voltage.c'),
    (Join-Path $algorithm '03_measurement\mean\signal_mean.c'),
    (Join-Path $algorithm '03_measurement\minmax\signal_minmax.c'),
    (Join-Path $algorithm '03_measurement\vpp\signal_vpp.c'),
    (Join-Path $algorithm '03_measurement\rms\signal_rms.c'),
    (Join-Path $algorithm '03_measurement\ac_rms\signal_ac_rms.c'),
    (Join-Path $algorithm '03_measurement\frequency_zero_cross\signal_zero_cross.c'),
    (Join-Path $algorithm '03_measurement\phase\signal_phase.c'),
    (Join-Path $algorithm '04_dsp\remove_dc\signal_remove_dc.c'),
    (Join-Path $algorithm '04_dsp\window\signal_window.c'),
    (Join-Path $algorithm '04_dsp\window\hann\signal_hann.c'),
    (Join-Path $algorithm '04_dsp\fft\signal_fft.c'),
    (Join-Path $algorithm '04_dsp\fft_magnitude\signal_fft_magnitude.c'),
    (Join-Path $algorithm '04_dsp\peak_detect\signal_peak_detect.c'),
    (Join-Path $algorithm '04_dsp\correlation\signal_correlation.c'),
    (Join-Path $algorithm '04_dsp\harmonic\signal_harmonic.c'),
    (Join-Path $algorithm '04_dsp\thd\signal_thd.c'),
    (Join-Path $algorithm '05_precision\zero_cross_interpolation\signal_zero_cross_interpolation.c'),
    (Join-Path $algorithm '05_precision\multi_cycle_average\signal_multi_cycle_average.c'),
    (Join-Path $algorithm '05_precision\fft_parabolic_interpolation\signal_fft_parabolic_interpolation.c'),
    (Join-Path $algorithm '05_precision\window_gain_correction\signal_window_gain_correction.c'),
    (Join-Path $algorithm '05_precision\multi_bin_energy\signal_multi_bin_energy.c')
)
$pcExe = Join-Path $build 'test_integration_round1.exe'
$pcArgs = @('-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-pedantic')
if ($FftBackend -ne 0) {
    $pcArgs += @(
        '-fno-strict-aliasing', '-DARM_MATH_CM0',
        ('-DSIGNAL_FFT_BACKEND=' + $FftBackend),
        ('-I' + (Join-Path $sdkSource 'third_party\CMSIS\DSP\Include')),
        ('-I' + (Join-Path $sdkSource 'third_party\CMSIS\Core\Include'))
    )
}
$pcArgs += $algorithmIncludes
$pcArgs += '-I' + (Join-Path $repo '08_applications\common')
$pcArgs += $pcSources
if ($FftBackend -ne 0) {
    $cmsisDspSource = Join-Path $sdkSource 'third_party\CMSIS\DSP\Source'
    $pcArgs += @(
        (Join-Path $cmsisDspSource 'CommonTables\arm_common_tables.c'),
        (Join-Path $cmsisDspSource 'CommonTables\arm_const_structs.c'),
        (Join-Path $cmsisDspSource 'TransformFunctions\arm_bitreversal.c'),
        (Join-Path $cmsisDspSource 'TransformFunctions\arm_bitreversal2.c'),
        (Join-Path $cmsisDspSource 'TransformFunctions\arm_cfft_q15.c'),
        (Join-Path $cmsisDspSource 'TransformFunctions\arm_cfft_radix4_q15.c'),
        (Join-Path $cmsisDspSource 'TransformFunctions\arm_cfft_q31.c'),
        (Join-Path $cmsisDspSource 'TransformFunctions\arm_cfft_radix4_q31.c'),
        (Join-Path $cmsisDspSource 'TransformFunctions\arm_cfft_f32.c'),
        (Join-Path $cmsisDspSource 'TransformFunctions\arm_cfft_radix8_f32.c')
    )
}
$pcArgs += @('-o', $pcExe, '-lm')
& $Gcc @pcArgs
if ($LASTEXITCODE -ne 0) { throw 'Round-1 PC compile failed' }
$pcOutput = & $pcExe
if ($LASTEXITCODE -ne 0) { throw 'Round-1 PC tests failed' }
$pcOutput | ForEach-Object { Write-Output $_ }

$tiBase = @(
    '-D__MSPM0G3507__', '-D__USE_SYSCONFIG__',
    '-march=thumbv6m', '-mcpu=cortex-m0plus', '-mfloat-abi=soft',
    '-mthumb', '-std=c11', '-Wall', '-Werror', '-fsyntax-only',
    ('-I' + $sdkSource),
    ('-I' + (Join-Path $sdkSource 'third_party\CMSIS\Core\Include')),
    ('-I' + (Join-Path $repo '08_applications\common')),
    ('-I' + (Join-Path $repo '08_applications\common\mspm0g3507')),
    ('-I' + (Join-Path $repo '01_bsp\common')),
    ('-I' + (Join-Path $repo '02_acquisition\adc_dma')),
    ('-I' + (Join-Path $repo '02_acquisition\timer_capture')),
    ('-I' + (Join-Path $repo '06_generator\dac_dma')),
    ('-I' + (Join-Path $repo '06_generator\dac_wave_table')),
    ('-I' + (Join-Path $repo '06_generator\dds')),
    ('-I' + (Join-Path $repo '06_generator\sine'))
)
if ($FftBackend -ne 0) {
    $tiBase += @(
        '-DARM_MATH_CM0', ('-DSIGNAL_FFT_BACKEND=' + $FftBackend),
        '-fno-strict-aliasing',
        ('-I' + (Join-Path $sdkSource 'third_party\CMSIS\DSP\Include'))
    )
}

$checks = @()
function Invoke-TiSyntax {
    param(
        [string]$Name,
        [string]$Source,
        [string]$Profile,
        [string]$AppDirectory,
        [string[]]$Defines = @()
    )
    $arguments = $tiBase + $Defines + $algorithmIncludes
    if ($Profile) {
        $arguments += '-I' + (Join-Path $repo "10_tests\peripheral_profiles\generated\$Profile")
    }
    if ($AppDirectory) {
        $arguments += '-I' + (Join-Path $repo "08_applications\$AppDirectory")
    }
    $arguments += (Join-Path $repo $Source)
    & $Compiler @arguments
    if ($LASTEXITCODE -ne 0) { throw "$Name TI syntax check failed" }
    Write-Output "$Name TI_SOURCE_COMPILE PASS"
    $script:checks += [pscustomobject]@{
        target = $Name
        ti_source_compile = 'PASS'
    }
}

Invoke-TiSyntax 'integration_core' '08_applications\common\signal_integration.c' '' ''
Invoke-TiSyntax 'dual_adc_adapter' '08_applications\common\signal_dual_adc_platform.c' 'PROFILE_02_DUAL_ADC' ''
Invoke-TiSyntax 'dac_dma_adapter' '08_applications\common\signal_dac_dma_platform.c' 'PROFILE_03_DAC_GENERATOR' ''
Invoke-TiSyntax 'signal_meter' '08_applications\signal_meter\main.c' 'PROFILE_01_ADC_CAPTURE' 'signal_meter'
Invoke-TiSyntax 'frequency_method_a' '08_applications\frequency_meter\main.c' 'PROFILE_05_FREQUENCY' 'frequency_meter' @('-DSIGNAL_FREQUENCY_METHOD=1')
Invoke-TiSyntax 'frequency_method_b' '08_applications\frequency_meter\main.c' 'PROFILE_01_ADC_CAPTURE' 'frequency_meter' @('-DSIGNAL_FREQUENCY_METHOD=2')
Invoke-TiSyntax 'frequency_method_c' '08_applications\frequency_meter\main.c' 'PROFILE_01_ADC_CAPTURE' 'frequency_meter' @('-DSIGNAL_FREQUENCY_METHOD=3')
Invoke-TiSyntax 'spectrum_analyzer' '08_applications\spectrum_analyzer\main.c' 'PROFILE_01_ADC_CAPTURE' 'spectrum_analyzer'
Invoke-TiSyntax 'harmonic_thd_analyzer' '08_applications\harmonic_thd_analyzer\main.c' 'PROFILE_01_ADC_CAPTURE' 'harmonic_thd_analyzer'
Invoke-TiSyntax 'dual_channel_phase_meter' '08_applications\dual_channel_phase_meter\main.c' 'PROFILE_02_DUAL_ADC' 'dual_channel_phase_meter'
Invoke-TiSyntax 'dds_generator' '08_applications\dds_generator\main.c' 'PROFILE_03_DAC_GENERATOR' 'dds_generator'

$result = [pscustomobject]@{
    date = (Get-Date -Format 'yyyy-MM-dd')
    pc_truth_tests = '4/4 PASS'
    fft_backend = $FftBackend
    ti_source_checks = "$($checks.Count)/$($checks.Count) PASS"
    compile_link = 'NOT_RUN'
    board = 'NOT_RUN'
    targets = $checks
}
$result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $build 'round1_results.json') -Encoding utf8
Write-Output "round1 validation: PASS"
