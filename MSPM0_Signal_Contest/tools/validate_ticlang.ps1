param(
    [string]$Compiler = 'D:\ti\CCS\ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\bin\tiarmclang.exe',
    [string]$SdkRoot = 'C:\TI\mspm0_sdk_2_11_00_07'
)

$ErrorActionPreference = 'Stop'
$contestRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

if (-not (Test-Path -LiteralPath $Compiler)) {
    throw "TI Arm Clang not found: $Compiler"
}
$sdkSource = Join-Path $SdkRoot 'source'
if (-not (Test-Path -LiteralPath $sdkSource)) {
    throw "MSPM0 SDK source not found: $sdkSource"
}

$sourceRoots = @('01_bsp', '02_acquisition', '03_measurement', '04_dsp',
    '05_precision', '06_generator', '07_signal_frontend', '08_applications')
$sources = @($sourceRoots | ForEach-Object {
    Get-ChildItem -LiteralPath (Join-Path $contestRoot $_) -Recurse `
        -Filter 'signal_*.c' -File
} | Where-Object {
    $_.Name -ne 'README_MINIMAL_EXAMPLE.c' -and
    (Get-Content -LiteralPath $_.FullName -Raw) -notmatch `
        '#include\s+"ti_msp_dl_config\.h"'
})

$includeDirs = Get-ChildItem -LiteralPath $contestRoot -Recurse -Filter 'signal_*.h' -File |
    ForEach-Object { $_.Directory.FullName } |
    Sort-Object -Unique |
    ForEach-Object { $_.Substring($contestRoot.Length + 1) }

$baseArgs = @(
    '-march=thumbv6m',
    '-mcpu=cortex-m0plus',
    '-mfloat-abi=soft',
    '-mlittle-endian',
    '-mthumb',
    '-std=c11',
    '-DARM_MATH_CM0',
    '-D__MSPM0G3507__',
    '-Wall',
    '-Werror',
    '-fsyntax-only'
)
$includeArgs = $includeDirs | ForEach-Object { '-I' + $_ }
$includeArgs += @(
    ('-I' + (Join-Path $sdkSource 'third_party\CMSIS\Core\Include')),
    ('-I' + (Join-Path $sdkSource 'third_party\CMSIS\DSP\Include')),
    ('-I' + $sdkSource)
)
$failed = @()

Push-Location $contestRoot
try {
    foreach ($source in $sources) {
        $relativeSource = $source.FullName.Substring($contestRoot.Length + 1)
        & $Compiler @baseArgs @includeArgs $relativeSource
        if ($LASTEXITCODE -ne 0) {
            $failed += $relativeSource
        }
    }
}
finally {
    Pop-Location
}

Write-Output "tiarmclang_checked=$($sources.Count) failed=$($failed.Count)"
if ($failed.Count -ne 0) {
    $failed | ForEach-Object { Write-Error "TI Arm Clang failed: $_" }
    exit 1
}
