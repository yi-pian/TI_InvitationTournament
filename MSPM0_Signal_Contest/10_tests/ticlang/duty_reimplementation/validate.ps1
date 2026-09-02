param(
    [string]$Compiler = 'D:\TI\CCS\ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\bin\tiarmclang.exe',
    [string]$SdkRoot = 'C:\TI\mspm0_sdk_2_11_00_07'
)

$ErrorActionPreference = 'Stop'
$contestRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$project = Join-Path $contestRoot '09_examples\adc_dma_onboard_selftest\ticlang\adc_dma_onboard_selftest_LP_MSPM0G3507_nortos_ticlang'
$debug = Join-Path $project 'Debug'
$build = Join-Path $PSScriptRoot 'build'
$duty = Join-Path $contestRoot '03_measurement\duty\signal_duty.c'
$main = Join-Path $PSScriptRoot 'main.c'

if (-not (Test-Path -LiteralPath $Compiler)) { throw "Compiler not found: $Compiler" }
if (-not (Test-Path -LiteralPath (Join-Path $debug 'device.opt'))) {
    throw 'Existing SysConfig/CCS device.opt is missing; target link cannot run.'
}
New-Item -ItemType Directory -Force -Path $build | Out-Null

$responseArg = '@' + (Join-Path $debug 'device.opt')
$includes = @(
    ('-I' + (Join-Path $contestRoot '03_measurement\duty')),
    ('-I' + (Join-Path $contestRoot '03_measurement\common')),
    ('-I' + $project),
    ('-I' + $debug),
    ('-I' + (Join-Path $SdkRoot 'source')),
    ('-I' + (Join-Path $SdkRoot 'source\third_party\CMSIS\Core\Include'))
)
$compile = @(
    '-march=thumbv6m', '-mcpu=cortex-m0plus', '-mfloat-abi=soft',
    '-mlittle-endian', '-mthumb', '-O2', '-std=c11', '-Wall', '-Werror',
    '-c', $responseArg
)
$dutyObject = Join-Path $build 'signal_duty.o'
$mainObject = Join-Path $build 'main.o'
& $Compiler @compile @includes '-o' $dutyObject $duty
if ($LASTEXITCODE -ne 0) { throw 'TI Arm Clang duty compile failed.' }
& $Compiler @compile @includes '-o' $mainObject $main
if ($LASTEXITCODE -ne 0) { throw 'TI Arm Clang duty main compile failed.' }

$output = Join-Path $build 'duty_reimplementation.out'
$map = Join-Path $build 'duty_reimplementation.map'
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
    (Join-Path $debug 'startup_mspm0g350x_ticlang.o'),
    $dutyObject,
    $mainObject,
    ('-Wl,-l' + (Join-Path $debug 'device_linker.cmd')),
    ('-Wl,-l' + (Join-Path $debug 'device.cmd.genlibs')),
    '-Wl,-llibc.a'
)
& $Compiler @link
if ($LASTEXITCODE -ne 0) { throw 'TI Arm Clang duty full target link failed.' }

Write-Output 'duty_compile=PASS main_compile=PASS full_target_link=PASS'
Write-Output "output=$output"
Write-Output "map=$map"
