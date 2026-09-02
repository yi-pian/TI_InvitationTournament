param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")),
    [string]$SdkRoot = "C:\TI\mspm0_sdk_2_11_00_07",
    [string]$CcsRoot = "D:\TI\CCS"
)

$ErrorActionPreference = "Stop"

$sysconfig = Join-Path $CcsRoot "ccs\utils\sysconfig_1.28.0\sysconfig_cli.bat"
$compiler = Join-Path $CcsRoot "ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\bin\tiarmclang.exe"
$compilerLib = Join-Path $CcsRoot "ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\lib"
$sdkSource = Join-Path $SdkRoot "source"
$product = Join-Path $SdkRoot ".metadata\product.json"
$startup = Join-Path $sdkSource "ti\devices\msp\m0p\startup_system_files\ticlang\startup_mspm0g350x_ticlang.c"
$outputRoot = Join-Path $PSScriptRoot "build_target"

foreach ($required in @($sysconfig, $compiler, $product, $startup)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Missing required tool/input: $required"
    }
}

function Get-CompileFlags([string]$buildDir, [string[]]$defines) {
    $flags = @(
        "@device.opt", "-DARM_MATH_CM0", "-march=thumbv6m",
        "-mcpu=cortex-m0plus", "-mfloat-abi=soft", "-mlittle-endian",
        "-mthumb", "-O2", "-g", "-Wall", "-Werror"
    )
    $flags += $defines
    $includeDirs = @(
        $buildDir,
        (Join-Path $sdkSource "third_party\CMSIS\Core\Include"),
        (Join-Path $sdkSource "third_party\CMSIS\DSP\Include"),
        $sdkSource,
        (Join-Path $RepoRoot "01_bsp\common"),
        (Join-Path $RepoRoot "03_measurement\common"),
        (Join-Path $RepoRoot "04_dsp\fft"),
        (Join-Path $RepoRoot "algorithm_backends"),
        (Join-Path $RepoRoot "algorithm_backends\reference"),
        (Join-Path $RepoRoot "algorithm_backends\cmsis_dsp"),
        (Join-Path $RepoRoot "algorithm_backends\iqmath")
    )
    foreach ($includeDir in $includeDirs) { $flags += "-I$includeDir" }
    return $flags
}

function Invoke-Compile([string]$buildDir, [string]$source,
    [string]$object, [string[]]$defines) {
    $flags = Get-CompileFlags $buildDir $defines
    Push-Location $buildDir
    try {
        & $compiler -c @flags -o $object $source | Out-Host
        if ($LASTEXITCODE -ne 0) { throw "Compile failed: $source" }
    }
    finally { Pop-Location }
}

function Invoke-Link([string]$buildDir, [string]$name,
    [string[]]$objects, [bool]$allowFailure) {
    $link = @(
        "@device.opt", "-march=thumbv6m", "-mcpu=cortex-m0plus",
        "-mfloat-abi=soft", "-mlittle-endian", "-mthumb", "-O2", "-g",
        "-Wall", "-Werror", "-Wl,-m$name.map", "-Wl,-i$sdkSource",
        "-Wl,-i$buildDir", "-Wl,-i$compilerLib", "-Wl,--diag_wrap=off",
        "-Wl,--display_error_number", "-Wl,--warn_sections",
        "-Wl,--rom_model", "-o", "$name.out"
    )
    $link += $objects
    $link += @(
        "-Wl,-l./device_linker.cmd", "-Wl,-ldevice.cmd.genlibs",
        "-Wl,-llibc.a"
    )
    Push-Location $buildDir
    try {
        & $compiler @link | Out-Host
        $success = ($LASTEXITCODE -eq 0)
        if ((-not $success) -and (-not $allowFailure)) {
            throw "Link failed: $name"
        }
        return $success
    }
    finally { Pop-Location }
}

function Get-MapMemory([string]$mapPath) {
    $flashLine = Select-String -LiteralPath $mapPath -Pattern '^\s+FLASH\s+' |
        Select-Object -First 1
    $sramLine = Select-String -LiteralPath $mapPath -Pattern '^\s+SRAM\s+' |
        Select-Object -First 1
    if (($null -eq $flashLine) -or ($null -eq $sramLine)) {
        throw "Memory summary not found in $mapPath"
    }
    $flashFields = $flashLine.Line.Trim() -split '\s+'
    $sramFields = $sramLine.Line.Trim() -split '\s+'
    return [pscustomobject]@{
        flash_bytes = [Convert]::ToInt32($flashFields[3], 16)
        sram_bytes = [Convert]::ToInt32($sramFields[3], 16)
    }
}

function Invoke-SysConfigVariant([string]$variant, [string]$scriptName) {
    $buildDir = Join-Path $outputRoot $variant
    New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
    & $sysconfig -s $product --script (Join-Path $PSScriptRoot $scriptName) `
        -o $buildDir --compiler ticlang | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "SysConfig failed: $variant" }
    $clockHeader = Get-Content -Raw (Join-Path $buildDir "ti_msp_dl_config.h")
    if ($clockHeader -notmatch '#define CPUCLK_FREQ\s+80000000') {
        throw "$variant did not generate CPUCLK_FREQ=80000000"
    }
    return $buildDir
}

function Build-FullBenchmark([string]$variant, [string]$scriptName) {
    $buildDir = Invoke-SysConfigVariant $variant $scriptName
    $sources = @(
        (Join-Path $PSScriptRoot "backend_benchmark_target.c"),
        (Join-Path $RepoRoot "algorithm_backends\signal_backend.c"),
        (Join-Path $RepoRoot "algorithm_backends\signal_fixed_point.c"),
        (Join-Path $RepoRoot "algorithm_backends\cmsis_dsp\signal_cmsis_dsp_backend.c"),
        (Join-Path $RepoRoot "algorithm_backends\iqmath\signal_iqmath_backend.c"),
        (Join-Path $buildDir "ti_msp_dl_config.c"),
        $startup
    )
    $objects = @()
    for ($index = 0; $index -lt $sources.Count; $index++) {
        $object = "full_{0:D2}.o" -f $index
        Invoke-Compile $buildDir $sources[$index] $object @()
        $objects += $object
    }
    $name = "backend_benchmark_$variant"
    [void](Invoke-Link $buildDir $name $objects $false)
    $memory = Get-MapMemory (Join-Path $buildDir "$name.map")
    return [pscustomobject]@{
        variant = $variant
        cpu_hz = 80000000
        compiler = "TI Arm Clang 5.1.1.LTS"
        sysconfig = "PASS"
        compile_link = "PASS"
        board_run = "PENDING"
        flash_bytes = $memory.flash_bytes
        sram_bytes = $memory.sram_bytes
    }
}

function Build-FFTMatrix([string]$baseBuildDir) {
    $rows = @()
    $backends = @(
        [pscustomobject]@{ name = "CMSIS_DSP_Q15"; id = 1 },
        [pscustomobject]@{ name = "CMSIS_DSP_Q31"; id = 2 },
        [pscustomobject]@{ name = "CMSIS_DSP_F32"; id = 3 }
    )
    foreach ($backend in $backends) {
        foreach ($size in @(256, 512, 1024, 2048)) {
            $caseName = "$($backend.name)_$size"
            $caseDir = Join-Path $outputRoot (Join-Path "fft_matrix" $caseName)
            New-Item -ItemType Directory -Force -Path $caseDir | Out-Null
            foreach ($generated in @(
                "device.opt", "device_linker.cmd", "device.cmd.genlibs",
                "ti_msp_dl_config.h"
            )) {
                Copy-Item -LiteralPath (Join-Path $baseBuildDir $generated) `
                    -Destination (Join-Path $caseDir $generated) -Force
            }
            $defines = @(
                "-DSIGNAL_BENCHMARK_FFT_SIZE=$size",
                "-DSIGNAL_BENCHMARK_FFT_BACKEND=$($backend.id)",
                "-DSIGNAL_BENCHMARK_ENABLE_IQMATH=0",
                "-DSIGNAL_CMSIS_DSP_FIXED_FFT_SIZE=$size"
            )
            $objects = @()
            Invoke-Compile $caseDir (Join-Path $PSScriptRoot "fft_build_probe_target.c") "probe.o" $defines
            $objects += "probe.o"
            Invoke-Compile $caseDir (Join-Path $RepoRoot "algorithm_backends\cmsis_dsp\signal_cmsis_dsp_backend.c") "cmsis.o" $defines
            $objects += "cmsis.o"
            Invoke-Compile $caseDir (Join-Path $baseBuildDir "ti_msp_dl_config.c") "config.o" $defines
            Invoke-Compile $caseDir $startup "startup.o" $defines
            $objects += @("config.o", "startup.o")
            $linked = Invoke-Link $caseDir $caseName $objects $true
            if ($linked) {
                $memory = Get-MapMemory (Join-Path $caseDir "$caseName.map")
                $rows += [pscustomobject]@{
                    backend = $backend.name; fft_size = $size
                    target_build = "PASS"; cycles = "PENDING_BOARD"
                    flash_bytes = $memory.flash_bytes
                    sram_bytes = $memory.sram_bytes
                }
            }
            else {
                $rows += [pscustomobject]@{
                    backend = $backend.name; fft_size = $size
                    target_build = "RAM_INFEASIBLE"; cycles = "PENDING_BOARD"
                    flash_bytes = $null; sram_bytes = $null
                }
            }
        }
    }
    return $rows
}

New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null
$fullResults = @()
$fullResults += Build-FullBenchmark "rts" "backend_benchmark_rts.syscfg"
$fullResults += Build-FullBenchmark "mathacl" "backend_benchmark_mathacl.syscfg"
$matrix = Build-FFTMatrix (Join-Path $outputRoot "rts")

$fullResults | ConvertTo-Json | Set-Content -LiteralPath `
    (Join-Path $outputRoot "target_backend_build_results.json") -Encoding utf8
$matrix | Export-Csv -LiteralPath (Join-Path $outputRoot "fft_target_build_matrix.csv") `
    -NoTypeInformation -Encoding utf8

$fullResults | Format-Table -AutoSize
$matrix | Format-Table -AutoSize
