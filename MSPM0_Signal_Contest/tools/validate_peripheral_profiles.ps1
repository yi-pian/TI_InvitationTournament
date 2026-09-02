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
$profilesRoot = Join-Path $RepoRoot "09_examples\integration_profiles"
$buildRoot = Join-Path $RepoRoot "10_tests\peripheral_profiles\build"
$smokeMain = Join-Path $RepoRoot "10_tests\peripheral_profiles\profile_smoke_main.c"

foreach ($required in @($sysconfig, $compiler, $product, $startup, $smokeMain)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required file not found: $required"
    }
}

New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null
$results = @()

Get-ChildItem -LiteralPath $profilesRoot -Directory | Sort-Object Name | ForEach-Object {
    $profileName = $_.Name
    $script = Join-Path $_.FullName "profile.syscfg"
    $build = Join-Path $buildRoot $profileName
    New-Item -ItemType Directory -Force -Path $build | Out-Null

    Write-Host "[$profileName] SysConfig"
    & $sysconfig -s $product --script $script -o $build --compiler ticlang
    if ($LASTEXITCODE -ne 0) {
        throw "$profileName SysConfig failed with exit code $LASTEXITCODE"
    }

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
        "-Werror",
        "-I$build",
        "-I$cmsis",
        "-I$sdkSource"
    )

    Push-Location $build
    try {
        Write-Host "[$profileName] Compile"
        & $compiler -c @common -o "ti_msp_dl_config.o" "ti_msp_dl_config.c"
        if ($LASTEXITCODE -ne 0) { throw "$profileName generated config compile failed" }

        & $compiler -c @common -o "startup.o" $startup
        if ($LASTEXITCODE -ne 0) { throw "$profileName startup compile failed" }

        & $compiler -c @common -o "profile_smoke_main.o" $smokeMain
        if ($LASTEXITCODE -ne 0) { throw "$profileName smoke main compile failed" }

        Write-Host "[$profileName] Link"
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
            "-Wl,-m$profileName.map",
            "-Wl,-i$sdkSource",
            "-Wl,-i$build",
            "-Wl,-i$compilerLib",
            "-Wl,--diag_wrap=off",
            "-Wl,--display_error_number",
            "-Wl,--warn_sections",
            "-Wl,--rom_model",
            "-o",
            "$profileName.out",
            "ti_msp_dl_config.o",
            "startup.o",
            "profile_smoke_main.o",
            "-Wl,-l./device_linker.cmd",
            "-Wl,-ldevice.cmd.genlibs",
            "-Wl,-llibc.a"
        )
        & $compiler @link
        if ($LASTEXITCODE -ne 0) { throw "$profileName link failed" }
    }
    finally {
        Pop-Location
    }

    $results += [pscustomobject]@{
        profile = $profileName
        sysconfig = "PASS"
        compile_wall_werror = "PASS"
        link = "PASS"
        board = "NOT_RUN"
    }
}

$summaryPath = Join-Path $buildRoot "profile_build_results.json"
$results | ConvertTo-Json | Set-Content -LiteralPath $summaryPath -Encoding utf8
$results | Format-Table -AutoSize
Write-Host "Result: $summaryPath"
