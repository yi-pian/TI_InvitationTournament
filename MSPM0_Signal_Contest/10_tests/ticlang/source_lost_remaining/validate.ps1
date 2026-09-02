param(
    [string]$Compiler = 'D:\TI\CCS\ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\bin\tiarmclang.exe',
    [string]$SdkRoot = 'C:\TI\mspm0_sdk_2_11_00_07'
)

$ErrorActionPreference = 'Stop'
$contestRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$project = Join-Path $contestRoot '09_examples\adc_dma_onboard_selftest\ticlang\adc_dma_onboard_selftest_LP_MSPM0G3507_nortos_ticlang'
$debug = Join-Path $project 'Debug'
$build = Join-Path $PSScriptRoot 'build'

if (-not (Test-Path -LiteralPath $Compiler)) { throw "Compiler not found: $Compiler" }
if (-not (Test-Path -LiteralPath (Join-Path $debug 'device.opt'))) {
    throw 'Existing SysConfig/CCS device.opt is missing; target link cannot run.'
}
New-Item -ItemType Directory -Force -Path $build | Out-Null

$responseArg = '@' + (Join-Path $debug 'device.opt')
$includeRelative = @(
    '03_measurement\common', '03_measurement\adc_to_voltage',
    '03_measurement\mean', '03_measurement\dc_measure',
    '04_dsp\peak_detect', '04_dsp\fft_peak',
    '05_precision\jacobsen_interpolation',
    '05_precision\quinn_interpolation',
    '05_precision\macleod_interpolation',
    '05_precision\coherent_sampling',
    '05_precision\frequency_response_correction',
    '05_precision\czt'
)
$includes = @()
foreach ($relative in $includeRelative) { $includes += '-I' + (Join-Path $contestRoot $relative) }
$includes += '-I' + $project
$includes += '-I' + $debug
$includes += '-I' + (Join-Path $SdkRoot 'source')
$includes += '-I' + (Join-Path $SdkRoot 'source\third_party\CMSIS\Core\Include')

$sourceRelative = @(
    '03_measurement\mean\signal_mean.c',
    '03_measurement\dc_measure\signal_dc_measure.c',
    '04_dsp\peak_detect\signal_peak_detect.c',
    '04_dsp\fft_peak\signal_fft_peak.c',
    '05_precision\jacobsen_interpolation\signal_jacobsen_interpolation.c',
    '05_precision\quinn_interpolation\signal_quinn_interpolation.c',
    '05_precision\macleod_interpolation\signal_macleod_interpolation.c',
    '05_precision\coherent_sampling\signal_coherent_sampling.c',
    '05_precision\frequency_response_correction\signal_frequency_response_correction.c',
    '05_precision\czt\signal_czt.c'
)
$compile = @(
    '-march=thumbv6m', '-mcpu=cortex-m0plus', '-mfloat-abi=soft',
    '-mlittle-endian', '-mthumb', '-O2', '-std=c11', '-Wall', '-Werror',
    '-c', $responseArg
)
$objects = @()
foreach ($relative in $sourceRelative) {
    $source = Join-Path $contestRoot $relative
    $objectName = ([IO.Path]::GetFileNameWithoutExtension($source)) + '.o'
    $object = Join-Path $build $objectName
    & $Compiler @compile @includes '-o' $object $source
    if ($LASTEXITCODE -ne 0) { throw "TI Arm Clang compile failed: $relative" }
    $objects += $object
}
$mainObject = Join-Path $build 'main.o'
& $Compiler @compile @includes '-o' $mainObject (Join-Path $PSScriptRoot 'main.c')
if ($LASTEXITCODE -ne 0) { throw 'TI Arm Clang main compile failed.' }
$objects += $mainObject

$output = Join-Path $build 'source_lost_remaining.out'
$map = Join-Path $build 'source_lost_remaining.map'
$link = @(
    $responseArg,
    '-march=thumbv6m', '-mcpu=cortex-m0plus', '-mfloat-abi=soft',
    '-mlittle-endian', '-mthumb', '-O2', '-g', '-Wall', '-Werror',
    ('-Wl,-m' + $map),
    ('-Wl,-i' + (Join-Path $SdkRoot 'source')),
    ('-Wl,-i' + $project),
    ('-Wl,-i' + (Join-Path $debug 'syscfg')),
    '-Wl,-iD:\TI\CCS\ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\lib',
    '-Wl,--diag_wrap=off', '-Wl,--display_error_number',
    '-Wl,--warn_sections', '-Wl,--rom_model', '-o', $output,
    (Join-Path $debug 'ti_msp_dl_config.o'),
    (Join-Path $debug 'startup_mspm0g350x_ticlang.o')
)
$link += $objects
$link += '-Wl,-l' + (Join-Path $debug 'device_linker.cmd')
$link += '-Wl,-l' + (Join-Path $debug 'device.cmd.genlibs')
$link += '-Wl,-llibc.a'
& $Compiler @link
if ($LASTEXITCODE -ne 0) { throw 'TI Arm Clang full target link failed.' }

Write-Output 'module_compile=PASS main_compile=PASS full_target_link=PASS'
Write-Output "output=$output"
Write-Output "map=$map"
