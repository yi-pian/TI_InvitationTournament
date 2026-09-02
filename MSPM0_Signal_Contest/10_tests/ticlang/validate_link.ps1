param(
    [string]$Compiler = 'D:\TI\CCS\ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\bin\tiarmclang.exe',
    [string]$SdkRoot = 'C:\TI\mspm0_sdk_2_11_00_07'
)

$ErrorActionPreference = 'Stop'
$contestRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$project = Join-Path $contestRoot '09_examples\adc_dma_onboard_selftest\ticlang\adc_dma_onboard_selftest_LP_MSPM0G3507_nortos_ticlang'
$debug = Join-Path $project 'Debug'
$build = Join-Path $PSScriptRoot 'build'

if (-not (Test-Path -LiteralPath $Compiler)) { throw "Compiler not found: $Compiler" }
if (-not (Test-Path -LiteralPath (Join-Path $debug 'device.opt'))) {
    throw 'Run a CCS/SysConfig build of adc_dma_onboard_selftest first; Debug/device.opt is missing.'
}

New-Item -ItemType Directory -Force -Path $build | Out-Null
$libraryRoots = @('01_bsp', '02_acquisition', '03_measurement', '04_dsp',
    '05_precision', '06_generator', '07_signal_frontend')
$allSources = @($libraryRoots | ForEach-Object {
    Get-ChildItem -LiteralPath (Join-Path $contestRoot $_) -Recurse `
        -Filter 'signal_*.c' -File
})

# Device-binding sources require mutually different generated instance names.
# They are full-linked by platform_closure/copy_assembly with the matching
# SysConfig profile, rather than forced into this one-profile aggregate image.
$profileSpecificSources = @($allSources | Where-Object {
    ($_.Name -match '_mspm0g3507\.c$') -or
    ($_.FullName -match '\\02_acquisition\\adc_fifo_dma\\signal_adc_fifo_dma\.c$')
})
$sources = @($allSources | Where-Object {
    ($_.Name -notmatch '_mspm0g3507\.c$') -and
    ($_.FullName -notmatch '\\02_acquisition\\adc_fifo_dma\\signal_adc_fifo_dma\.c$')
})
$sources += Get-Item -LiteralPath (Join-Path $contestRoot `
    '08_applications\common\signal_integration.c')
$includeDirs = Get-ChildItem -LiteralPath $contestRoot -Recurse -Filter 'signal_*.h' -File |
    ForEach-Object { $_.Directory.FullName } | Sort-Object -Unique
$includeDirs += @(
    $project,
    $debug,
    (Join-Path $SdkRoot 'source'),
    (Join-Path $SdkRoot 'source\third_party\CMSIS\Core\Include')
)
$includeArgs = $includeDirs | Sort-Object -Unique | ForEach-Object { '-I' + $_ }
$responseArg = ('@' + (Join-Path $debug 'device.opt'))
$compileArgs = @(
    '-march=thumbv6m', '-mcpu=cortex-m0plus', '-mfloat-abi=soft',
    '-mlittle-endian', '-mthumb', '-O2', '-std=c11',
    '-Wall', '-Werror', '-c', $responseArg
)

$objects = @()
foreach ($source in $sources) {
    $relative = $source.FullName.Substring($contestRoot.Length + 1)
    $objectName = (($relative -replace '[\\/:]', '_') -replace '\.c$', '.o')
    $object = Join-Path $build $objectName
    & $Compiler @compileArgs @includeArgs '-o' $object $source.FullName
    if ($LASTEXITCODE -ne 0) { throw "Compile failed: $relative" }
    $objects += $object
}

$testSource = Join-Path $contestRoot '10_tests\pc\test_signal_library.c'
$testObject = Join-Path $build 'test_signal_library.o'
& $Compiler @compileArgs @includeArgs '-o' $testObject $testSource
if ($LASTEXITCODE -ne 0) { throw 'Compile failed: test_signal_library.c' }
$objects += $testObject

$output = Join-Path $build 'signal_library_ticlang_buildcheck.out'
$map = Join-Path $build 'signal_library_ticlang_buildcheck.map'
$linkArgs = @(
    $responseArg,
    '-march=thumbv6m', '-mcpu=cortex-m0plus', '-mfloat-abi=soft',
    '-mlittle-endian', '-mthumb', '-O2', '-g', '-Wall', '-Werror',
    ('-Wl,-m' + $map),
    ('-Wl,-i' + (Join-Path $SdkRoot 'source')),
    ('-Wl,-i' + $project),
    ('-Wl,-i' + (Join-Path $debug 'syscfg')),
    '-Wl,-iD:\TI\CCS\ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\lib',
    '-Wl,--diag_wrap=off', '-Wl,--display_error_number',
    '-Wl,--warn_sections', '-Wl,--heap_size=16', '-Wl,--rom_model', '-o', $output
)
$linkInputs = @(
    (Join-Path $debug 'ti_msp_dl_config.o'),
    (Join-Path $debug 'startup_mspm0g350x_ticlang.o')
) + $objects + @(
    ('-Wl,-l' + (Join-Path $debug 'device_linker.cmd')),
    ('-Wl,-l' + (Join-Path $debug 'device.cmd.genlibs')),
    '-Wl,-llibc.a'
)

& $Compiler @linkArgs @linkInputs
if ($LASTEXITCODE -ne 0) { throw 'Aggregate TI Arm Clang link failed.' }

Write-Output "library_sources=$($allSources.Count) aggregate_sources=$($sources.Count) profile_specific_excluded=$($profileSpecificSources.Count) compiled=$($sources.Count + 1) linked=1"
if ($profileSpecificSources.Count -ne 0) {
    Write-Output ('profile_specific=' + (($profileSpecificSources | ForEach-Object {
        $_.FullName.Substring($contestRoot.Length + 1)
    }) -join ';'))
}
Write-Output "output=$output"
