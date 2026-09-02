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
$profile = Join-Path $RepoRoot "09_examples\integration_profiles\PROFILE_01_ADC_CAPTURE\profile.syscfg"
$smokeMain = Join-Path $RepoRoot "10_tests\peripheral_profiles\profile_smoke_main.c"
$templateDir = Join-Path $RepoRoot "08_applications\peripheral_system_template"
$templateSource = Join-Path $templateDir "peripheral_system_template.c"
$templateMain = Join-Path $templateDir "main_template.c"
$build = Join-Path $RepoRoot "10_tests\peripheral_library\build"
$roots = @("01_bsp", "02_acquisition", "06_generator", "07_signal_frontend")

New-Item -ItemType Directory -Force -Path $build | Out-Null

Write-Host "[library] SysConfig PROFILE_01"
& $sysconfig -s $product --script $profile -o $build --compiler ticlang
if ($LASTEXITCODE -ne 0) { throw "SysConfig failed" }

$sources = @()
foreach ($root in $roots) {
    $sources += Get-ChildItem -LiteralPath (Join-Path $RepoRoot $root) -Recurse -Filter "signal_*.c"
}
$sources = $sources | Sort-Object FullName
$moduleCards = @()
foreach ($root in $roots) {
    $moduleCards += Get-ChildItem -LiteralPath (Join-Path $RepoRoot $root) `
        -Recurse -Filter "MODULE_CARD.md" -File
}
if ($moduleCards.Count -ne 47) {
    throw "Expected 47 canonical peripheral modules, found $($moduleCards.Count)"
}
if ($sources.Count -ne 51) {
    throw "Expected 51 source files from 47 canonical peripheral modules, found $($sources.Count)"
}

# A generated SysConfig profile can only own one concrete definition of an
# ADC/DAC/TFT IRQ and its instance macros. Compile PROFILE_01-compatible
# sources here; the other profile-bound contest entries are compiled and
# fully linked in isolation by run_copy_assembly_tests.ps1.
$profileBoundRelative = @(
    '01_bsp\tft_ili9341\signal_tft_ili9341_mspm0g3507.c',
    '02_acquisition\adc_dual_sync\signal_dual_adc_mspm0g3507.c',
    '02_acquisition\adc_fifo_dma\signal_adc_fifo_dma.c',
    '02_acquisition\timer_capture\signal_timer_capture_mspm0g3507.c',
    '06_generator\dac_dma\signal_dac_dma_mspm0g3507.c'
)
$aggregateSources = @($sources | Where-Object {
    $relative = $_.FullName.Substring($RepoRoot.Length + 1)
    $profileBoundRelative -notcontains $relative
})

$includeDirs = @($build, $cmsis, $sdkSource, (Join-Path $RepoRoot "01_bsp\common"))
$includeDirs += $aggregateSources.Directory.FullName | Sort-Object -Unique
$includeDirs += $templateDir
$common = @(
    "@device.opt",
    "-march=thumbv6m",
    "-mcpu=cortex-m0plus",
    "-mfloat-abi=soft",
    "-mlittle-endian",
    "-mthumb",
    "-O2",
    "-g",
    "-Wall",
    "-Werror"
)
foreach ($dir in $includeDirs) { $common += "-I$dir" }

$objects = @()
Push-Location $build
try {
    $index = 0
    foreach ($source in $aggregateSources) {
        $object = "peripheral_{0:D2}.o" -f $index
        Write-Host "[library] Compile $($source.FullName.Substring($RepoRoot.Length + 1))"
        & $compiler -c @common -o $object $source.FullName
        if ($LASTEXITCODE -ne 0) { throw "Compile failed: $($source.FullName)" }
        $objects += $object
        $index++
    }

    & $compiler -c @common -o "ti_msp_dl_config.o" "ti_msp_dl_config.c"
    if ($LASTEXITCODE -ne 0) { throw "Generated config compile failed" }
    & $compiler -c @common -o "startup.o" $startup
    if ($LASTEXITCODE -ne 0) { throw "Startup compile failed" }
    & $compiler -c @common -o "smoke.o" $smokeMain
    if ($LASTEXITCODE -ne 0) { throw "Smoke main compile failed" }

    $link = @(
        "@device.opt",
        "-march=thumbv6m",
        "-mcpu=cortex-m0plus",
        "-mfloat-abi=soft",
        "-mlittle-endian",
        "-mthumb",
        "-O2",
        "-g",
        "-Wall",
        "-Werror",
        "-Wl,-mperipheral_library.map",
        "-Wl,-i$sdkSource",
        "-Wl,-i$build",
        "-Wl,-i$compilerLib",
        "-Wl,--diag_wrap=off",
        "-Wl,--display_error_number",
        "-Wl,--warn_sections",
        "-Wl,--rom_model",
        "-o",
        "peripheral_library.out",
        "ti_msp_dl_config.o",
        "startup.o",
        "smoke.o"
    )
    $link += $objects
    $link += @(
        "-Wl,-l./device_linker.cmd",
        "-Wl,-ldevice.cmd.genlibs",
        "-Wl,-llibc.a"
    )

    Write-Host "[library] Link PROFILE_01-compatible peripheral modules"
    & $compiler @link
    if ($LASTEXITCODE -ne 0) { throw "Peripheral aggregate link failed" }

    Write-Host "[template] Compile and link application skeleton"
    & $compiler -c @common -o "template.o" $templateSource
    if ($LASTEXITCODE -ne 0) { throw "Template compile failed" }
    & $compiler -c @common -o "template_main.o" $templateMain
    if ($LASTEXITCODE -ne 0) { throw "Template main compile failed" }

    $templateLink = @(
        "@device.opt", "-march=thumbv6m", "-mcpu=cortex-m0plus",
        "-mfloat-abi=soft", "-mlittle-endian", "-mthumb", "-O2", "-g",
        "-Wall", "-Werror", "-Wl,-mperipheral_system_template.map",
        "-Wl,-i$sdkSource", "-Wl,-i$build", "-Wl,-i$compilerLib",
        "-Wl,--diag_wrap=off", "-Wl,--display_error_number",
        "-Wl,--warn_sections", "-Wl,--rom_model", "-o",
        "peripheral_system_template.out", "ti_msp_dl_config.o", "startup.o",
        "template.o", "template_main.o", "-Wl,-l./device_linker.cmd",
        "-Wl,-ldevice.cmd.genlibs", "-Wl,-llibc.a"
    )
    & $compiler @templateLink
    if ($LASTEXITCODE -ne 0) { throw "Template link failed" }
}
finally {
    Pop-Location
}

$result = [pscustomobject]@{
    formal_module_count = $moduleCards.Count
    formal_source_file_count = $sources.Count
    aggregate_profile_compatible_source_count = $aggregateSources.Count
    profile_bound_sources = $profileBoundRelative
    compiler = "TI Arm Clang 5.1.1.LTS"
    flags = "-Wall -Werror"
    compile = "PASS"
    aggregate_link = "PASS"
    template_compile_link = "PASS"
    board = "NOT_RUN"
}
$result | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $build "peripheral_library_build_result.json") -Encoding utf8
$result | Format-List
