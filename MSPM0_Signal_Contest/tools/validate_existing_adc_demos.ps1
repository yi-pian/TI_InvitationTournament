param(
    [string]$RepoRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$SdkRoot = "C:\TI\mspm0_sdk_2_11_00_07",
    [string]$CcsRoot = "D:\TI\CCS"
)

$ErrorActionPreference = "Stop"

$sysconfig = Join-Path $CcsRoot "ccs\utils\sysconfig_1.28.0\sysconfig_cli.bat"
$compiler = Join-Path $CcsRoot "ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\bin\tiarmclang.exe"
$compilerLib = Join-Path $CcsRoot "ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\lib"
$product = Join-Path $SdkRoot ".metadata\product.json"
$sdkSource = Join-Path $SdkRoot "source"
$cmsis = Join-Path $sdkSource "third_party\CMSIS\Core\Include"
$startup = Join-Path $sdkSource "ti\devices\msp\m0p\startup_system_files\ticlang\startup_mspm0g350x_ticlang.c"
$adcSource = Join-Path $RepoRoot "02_acquisition\adc_dma\signal_adc_dma.c"
$adcInclude = Join-Path $RepoRoot "02_acquisition\adc_dma"
$commonInclude = Join-Path $RepoRoot "01_bsp\common"
$buildRoot = Join-Path $RepoRoot "10_tests\existing_adc_demos\build"

$demos = @(
    @{ name = "adc_dma_demo"; syscfg = "signal_adc_dma_demo.syscfg" },
    @{ name = "adc_buffer_uart_dump"; syscfg = "adc_buffer_uart_dump.syscfg" },
    @{ name = "adc_dma_onboard_selftest"; syscfg = "adc_dma_onboard_selftest.syscfg" }
)

New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null
$results = @()

foreach ($demo in $demos) {
    $name = $demo.name
    $sourceDir = Join-Path $RepoRoot "09_examples\$name"
    $script = Join-Path $sourceDir $demo.syscfg
    $main = Join-Path $sourceDir "main.c"
    $build = Join-Path $buildRoot $name
    New-Item -ItemType Directory -Force -Path $build | Out-Null

    Write-Host "[$name] SysConfig"
    & $sysconfig -s $product --script $script -o $build --compiler ticlang
    if ($LASTEXITCODE -ne 0) { throw "$name SysConfig failed" }

    $common = @(
        "@device.opt", "-march=thumbv6m", "-mcpu=cortex-m0plus",
        "-mfloat-abi=soft", "-mlittle-endian", "-mthumb", "-O2", "-g",
        "-Wall", "-Werror", "-I$build", "-I$sourceDir", "-I$adcInclude",
        "-I$commonInclude", "-I$cmsis", "-I$sdkSource"
    )

    Push-Location $build
    try {
        foreach ($unit in @(
            @{ out = "config.o"; src = (Join-Path $build "ti_msp_dl_config.c") },
            @{ out = "startup.o"; src = $startup },
            @{ out = "main.o"; src = $main },
            @{ out = "adc_dma.o"; src = $adcSource }
        )) {
            & $compiler -c @common -o $unit.out $unit.src
            if ($LASTEXITCODE -ne 0) { throw "$name compile failed: $($unit.src)" }
        }

        $link = @(
            "@device.opt", "-march=thumbv6m", "-mcpu=cortex-m0plus",
            "-mfloat-abi=soft", "-mlittle-endian", "-mthumb", "-O2", "-g",
            "-Wall", "-Werror", "-Wl,-m$name.map", "-Wl,-i$sdkSource",
            "-Wl,-i$build", "-Wl,-i$compilerLib", "-Wl,--diag_wrap=off",
            "-Wl,--display_error_number", "-Wl,--warn_sections", "-Wl,--rom_model",
            "-o", "$name.out", "config.o", "startup.o", "main.o", "adc_dma.o",
            "-Wl,-l./device_linker.cmd", "-Wl,-ldevice.cmd.genlibs", "-Wl,-llibc.a"
        )
        & $compiler @link
        if ($LASTEXITCODE -ne 0) { throw "$name link failed" }
    }
    finally {
        Pop-Location
    }

    $results += [pscustomobject]@{
        demo = $name
        sysconfig = "PASS"
        compile_wall_werror = "PASS"
        link = "PASS"
    }
}

$results | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $buildRoot "adc_demo_build_results.json") -Encoding utf8
$results | Format-Table -AutoSize
