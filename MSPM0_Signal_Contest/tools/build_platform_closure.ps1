param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')),
    [string]$SdkRoot = 'C:\TI\mspm0_sdk_2_11_00_07',
    [string]$CcsRoot = 'D:\TI\CCS'
)

$ErrorActionPreference = 'Stop'

$sysconfig = Join-Path $CcsRoot 'ccs\utils\sysconfig_1.28.0\sysconfig_cli.bat'
$compiler = Join-Path $CcsRoot 'ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\bin\tiarmclang.exe'
$compilerLib = Join-Path $CcsRoot 'ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\lib'
$product = Join-Path $SdkRoot '.metadata\product.json'
$sdkSource = Join-Path $SdkRoot 'source'
$cmsis = Join-Path $sdkSource 'third_party\CMSIS\Core\Include'
$startup = Join-Path $sdkSource 'ti\devices\msp\m0p\startup_system_files\ticlang\startup_mspm0g350x_ticlang.c'
$buildRoot = Join-Path $RepoRoot '10_tests\platform_closure\build'
$documentationCheck = Join-Path $RepoRoot `
    'tools\validate_documentation_api_consistency.ps1'

$platformSource = '08_applications\common\mspm0g3507\signal_mspm0g3507_platform.c'
$platformBspSources = @(
    '01_bsp\adc\signal_adc.c',
    '01_bsp\comparator\signal_comparator.c',
    '01_bsp\dac\signal_dac.c',
    '01_bsp\dma\signal_dma.c',
    '01_bsp\gpio\signal_gpio.c',
    '01_bsp\system_clock\signal_system_clock.c',
    '01_bsp\timer\signal_timer.c',
    '01_bsp\uart\signal_uart.c'
)

$targets = @(
    [pscustomobject]@{
        Name = 'dac_dc_minimum'
        Profile = 'PROFILE_07_BASIC_IO'
        Main = '09_examples\platform_closure\dac_dc_minimum\main.c'
        Sources = @()
    },
    [pscustomobject]@{
        Name = 'adc_basic_minimum'
        Profile = 'PROFILE_07_BASIC_IO'
        Main = '09_examples\platform_closure\adc_basic_minimum\main.c'
        Sources = @()
    },
    [pscustomobject]@{
        Name = 'uart_minimum'
        Profile = 'PROFILE_07_BASIC_IO'
        Main = '09_examples\platform_closure\uart_minimum\main.c'
        Sources = @()
    },
    [pscustomobject]@{
        Name = 'gpio_minimum'
        Profile = 'PROFILE_07_BASIC_IO'
        Main = '09_examples\platform_closure\gpio_minimum\main.c'
        Sources = @()
    },
    [pscustomobject]@{
        Name = 'adc_timer_trigger_minimum'
        Profile = 'PROFILE_01_ADC_CAPTURE'
        Main = '09_examples\platform_closure\adc_timer_trigger_minimum\main.c'
        Sources = @($platformSource) + $platformBspSources + @(
            '02_acquisition\adc_timer_trigger\signal_adc_timer_trigger.c')
    },
    [pscustomobject]@{
        Name = 'adc_continuous_minimum'
        Profile = 'PROFILE_07_BASIC_IO'
        Main = '09_examples\platform_closure\adc_continuous_minimum\main.c'
        Sources = @('02_acquisition\adc_continuous\signal_adc_continuous.c')
    },
    [pscustomobject]@{
        Name = 'adc_dma_minimum'
        Profile = 'PROFILE_01_ADC_CAPTURE'
        Main = '09_examples\platform_closure\adc_dma_minimum\main.c'
        Sources = @('02_acquisition\adc_dma\signal_adc_dma.c')
    },
    [pscustomobject]@{
        Name = 'dac_dma_minimum'
        Profile = 'PROFILE_03_DAC_GENERATOR'
        Main = '09_examples\platform_closure\dac_dma_minimum\main.c'
        Sources = @(
            '08_applications\common\signal_dac_dma_platform.c',
            '06_generator\dac_dma\signal_dac_dma.c')
    },
    [pscustomobject]@{
        Name = 'timer_capture_minimum'
        Profile = 'PROFILE_05_FREQUENCY'
        Main = '09_examples\platform_closure\timer_capture_minimum\main.c'
        Sources = @($platformSource) + $platformBspSources + @(
            '08_applications\common\mspm0g3507\signal_mspm0g3507_capture_platform.c',
            '02_acquisition\timer_capture\signal_timer_capture.c')
    },
    [pscustomobject]@{
        Name = 'tft_ili9341_minimum'
        Profile = 'TFT_ILI9341'
        ProfilePath = '09_examples\tft_ili9341_lp_mspm0g3507\tft_ili9341.syscfg'
        Main = '09_examples\tft_ili9341_lp_mspm0g3507\main.c'
        Sources = @(
            '08_applications\common\mspm0g3507\signal_mspm0g3507_tft_platform.c',
            '01_bsp\tft_ili9341\signal_tft_ili9341.c')
    }
)

foreach ($required in @($sysconfig, $compiler, $product, $sdkSource,
    $cmsis, $startup, $documentationCheck)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required build input not found: $required"
    }
}

$global:LASTEXITCODE = 0
& $documentationCheck -RepoRoot $RepoRoot
if ($LASTEXITCODE -ne 0) {
    throw 'Documentation/API consistency check failed before build.'
}
New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null

function Invoke-TICompile([string]$BuildDir, [string]$Source,
    [string]$Object, [string[]]$Flags) {
    Push-Location $BuildDir
    try {
        $saved = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        $output = & $compiler -c @Flags -o $Object $Source 2>&1
        $ErrorActionPreference = $saved
        $exitCode = $LASTEXITCODE
        if ($output) { $output | Out-Host }
        if ($exitCode -ne 0) { throw "Compile failed: $Source" }
    }
    finally {
        $ErrorActionPreference = $saved
        Pop-Location
    }
}

function Get-MapUsage([string]$MapPath) {
    $lines = Get-Content -LiteralPath $MapPath
    $flash = ($lines | Where-Object { $_ -match '^\s*FLASH\s+' } |
        Select-Object -First 1).Trim() -split '\s+'
    $sram = ($lines | Where-Object { $_ -match '^\s*SRAM\s+' } |
        Select-Object -First 1).Trim() -split '\s+'
    return [pscustomobject]@{
        FlashBytes = [Convert]::ToInt32($flash[3], 16)
        SramBytes = [Convert]::ToInt32($sram[3], 16)
    }
}

$results = @()
foreach ($target in $targets) {
    Write-Host "[$($target.Name)] SysConfig"
    $buildDir = Join-Path $buildRoot $target.Name
    New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
    $profile = if ($null -ne $target.ProfilePath) {
        Join-Path $RepoRoot $target.ProfilePath
    } else {
        Join-Path $RepoRoot (
            "09_examples\integration_profiles\$($target.Profile)\profile.syscfg")
    }
    $sysOutput = & $sysconfig -s $product --script $profile -o $buildDir `
        --compiler ticlang 2>&1
    $sysExit = $LASTEXITCODE
    $sysOutput | Set-Content -LiteralPath (Join-Path $buildDir 'sysconfig.log') `
        -Encoding utf8
    $sysOutput | Out-Host
    if ($sysExit -ne 0) { throw "SysConfig failed: $($target.Name)" }

    $sources = @((Join-Path $RepoRoot $target.Main))
    $sources += $target.Sources | ForEach-Object { Join-Path $RepoRoot $_ }
    $compileSources = $sources + @(
        (Join-Path $buildDir 'ti_msp_dl_config.c'), $startup)
    foreach ($source in $compileSources) {
        if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
            throw "Missing source: $source"
        }
    }
    $includeDirs = @($buildDir, $cmsis, $sdkSource,
        (Join-Path $RepoRoot '01_bsp\common'),
        (Join-Path $RepoRoot '01_bsp\button'),
        (Join-Path $RepoRoot '01_bsp\latching_button_switch'),
        (Join-Path $RepoRoot '01_bsp\matrix_keypad_4x4'),
        (Join-Path $RepoRoot '08_applications\common'),
        (Join-Path $RepoRoot '08_applications\common\mspm0g3507'))
    $includeDirs += $sources | ForEach-Object { Split-Path -Parent $_ }
    $includeDirs = $includeDirs | Sort-Object -Unique
    $flags = @('@device.opt', '-march=thumbv6m', '-mcpu=cortex-m0plus',
        '-mfloat-abi=soft', '-mlittle-endian', '-mthumb', '-std=c11', '-O2',
        '-g', '-Wall', '-Werror', '-ffunction-sections', '-fdata-sections')
    foreach ($include in $includeDirs) { $flags += "-I$include" }

    Write-Host "[$($target.Name)] Compile"
    $objects = @()
    for ($index = 0; $index -lt $compileSources.Count; ++$index) {
        $object = 'obj_{0:D2}.o' -f $index
        Invoke-TICompile $buildDir $compileSources[$index] $object $flags
        $objects += $object
    }

    Write-Host "[$($target.Name)] Full link"
    $link = @('@device.opt', '-march=thumbv6m', '-mcpu=cortex-m0plus',
        '-mfloat-abi=soft', '-mlittle-endian', '-mthumb', '-O2', '-g',
        '-Wall', '-Werror', "-Wl,-m$($target.Name).map",
        "-Wl,-i$sdkSource", "-Wl,-i$buildDir", "-Wl,-i$compilerLib",
        '-Wl,--diag_wrap=off', '-Wl,--display_error_number',
        '-Wl,--warn_sections', '-Wl,--rom_model', '-o',
        "$($target.Name).out")
    $link += $objects
    $link += @('-Wl,-l./device_linker.cmd', '-Wl,-ldevice.cmd.genlibs',
        '-Wl,-llibc.a')
    Push-Location $buildDir
    try {
        $linkOutput = & $compiler @link 2>&1
        $linkExit = $LASTEXITCODE
        if ($linkOutput) { $linkOutput | Out-Host }
        if ($linkExit -ne 0) { throw "Link failed: $($target.Name)" }
    }
    finally { Pop-Location }

    $usage = Get-MapUsage (Join-Path $buildDir "$($target.Name).map")
    $results += [pscustomobject]@{
        target = $target.Name
        profile = $target.Profile
        sysconfig = 'PASS'
        compile = 'PASS'
        link = 'PASS'
        flash_bytes = $usage.FlashBytes
        sram_bytes_including_stack = $usage.SramBytes
        board = 'NOT_RUN'
        status = 'BUILD_VERIFIED'
    }
    Write-Host "[$($target.Name)] PASS"
}

$resultPath = Join-Path $buildRoot 'platform_closure_build_results.json'
$results | ConvertTo-Json | Set-Content -LiteralPath $resultPath -Encoding utf8
$results | Format-Table -AutoSize
Write-Output "Platform closure full build/link PASS: $($results.Count)/$($results.Count)"
