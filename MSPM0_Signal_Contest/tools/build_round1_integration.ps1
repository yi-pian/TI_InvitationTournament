param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')),
    [string]$AlgorithmRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')),
    [string]$SdkRoot = 'C:\TI\mspm0_sdk_2_11_00_07',
    [string]$CcsRoot = 'D:\TI\CCS',
    [string]$OutputDirectoryName = 'round1_build_closure',
    [string[]]$TargetNames = @(),
    [string[]]$ExtraDefines = @(),
    [int]$FftBackend = 0,
    [int]$SampleCountOverride = 0,
    [string]$TargetManifestPath = (Join-Path $PSScriptRoot 'round1_integration_targets.ps1'),
    [string]$TargetFunction = 'Get-Round1IntegrationTargets'
)

$ErrorActionPreference = 'Stop'
. $TargetManifestPath

$sysconfig = Join-Path $CcsRoot 'ccs\utils\sysconfig_1.28.0\sysconfig_cli.bat'
$compiler = Join-Path $CcsRoot 'ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\bin\tiarmclang.exe'
$sizeTool = Join-Path $CcsRoot 'ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\bin\tiarmsize.exe'
$compilerLib = Join-Path $CcsRoot 'ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\lib'
$product = Join-Path $SdkRoot '.metadata\product.json'
$sdkSource = Join-Path $SdkRoot 'source'
$cmsis = Join-Path $sdkSource 'third_party\CMSIS\Core\Include'
$cmsisDsp = Join-Path $sdkSource 'third_party\CMSIS\DSP\Include'
$cmsisLibrary = Join-Path $sdkSource 'third_party\CMSIS\DSP\Lib\ticlang\m0p\arm_cortexM0l_math.a'
$startup = Join-Path $sdkSource 'ti\devices\msp\m0p\startup_system_files\ticlang\startup_mspm0g350x_ticlang.c'
$outputRoot = Join-Path $RepoRoot (Join-Path '10_tests\integration' $OutputDirectoryName)
$flashTotal = 0x20000
$sramTotal = 0x8000

foreach ($required in @(
    $sysconfig, $compiler, $sizeTool, $compilerLib, $product, $sdkSource,
    $cmsis, $cmsisDsp, $cmsisLibrary, $startup, $AlgorithmRoot
)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required build input not found: $required"
    }
}
New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null

function Hex-ToInt([string]$Hex) {
    return [Convert]::ToInt32($Hex, 16)
}

function Get-MapMetrics([string]$MapPath) {
    $lines = Get-Content -LiteralPath $MapPath
    $flashLine = $lines | Where-Object { $_ -match '^\s*FLASH\s+' } | Select-Object -First 1
    $sramLine = $lines | Where-Object { $_ -match '^\s*SRAM\s+' } | Select-Object -First 1
    if (($null -eq $flashLine) -or ($null -eq $sramLine)) {
        throw "Memory configuration missing from $MapPath"
    }
    $flashFields = $flashLine.Trim() -split '\s+'
    $sramFields = $sramLine.Trim() -split '\s+'
    $flashUsed = Hex-ToInt $flashFields[3]
    $sramUsed = Hex-ToInt $sramFields[3]
    $stackLine = $lines | Where-Object {
        $_ -match '^\.stack\s+\d+\s+[0-9a-fA-F]+\s+[0-9a-fA-F]+'
    } | Select-Object -First 1
    if ($null -eq $stackLine) {
        throw "Stack section missing from $MapPath"
    }
    $stackFields = $stackLine.Trim() -split '\s+'
    $stackBytes = Hex-ToInt $stackFields[3]

    $buffers = @()
    foreach ($line in $lines) {
        if ($line -match '^\s*[0-9a-fA-F]{8}\s+(?<size>[0-9a-fA-F]{8})\s+.+\(\.(?:bss|data)\.(?<name>[^\)]+)\)') {
            $bytes = Hex-ToInt $Matches.size
            if ($bytes -ge 256) {
                $buffers += [pscustomobject]@{
                    name = $Matches.name
                    bytes = $bytes
                }
            }
        }
    }
    $buffers = $buffers | Sort-Object bytes -Descending
    return [pscustomobject]@{
        flash_used_bytes = $flashUsed
        flash_total_bytes = $flashTotal
        flash_remaining_bytes = $flashTotal - $flashUsed
        sram_used_bytes_including_stack = $sramUsed
        sram_total_bytes = $sramTotal
        stack_reserved_bytes = $stackBytes
        static_sram_bytes_excluding_stack = $sramUsed - $stackBytes
        sram_remaining_bytes = $sramTotal - $sramUsed
        large_buffers = @($buffers)
    }
}

function Get-TiArmSizeMetrics([string]$OutputPath) {
    $sizeOutput = & $sizeTool $OutputPath 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "tiarmsize failed: $OutputPath"
    }
    $dataLine = $sizeOutput | Where-Object {
        $_ -match '^\s*\d+\s+\d+\s+\d+\s+\d+\s+[0-9a-fA-F]+\s+'
    } | Select-Object -First 1
    if ($null -eq $dataLine) {
        throw "tiarmsize output could not be parsed: $OutputPath"
    }
    $fields = $dataLine.Trim() -split '\s+'
    return [pscustomobject]@{
        flash_bytes = [int]$fields[0] + [int]$fields[1]
        sram_bytes = [int]$fields[1] + [int]$fields[2]
    }
}

function Invoke-CompileSource(
    [string]$BuildDir,
    [string]$Source,
    [string]$Object,
    [string[]]$Flags
) {
    Push-Location $BuildDir
    try {
        # Windows PowerShell converts native stderr lines into ErrorRecord objects.
        # Keep those diagnostics capturable without letting ErrorActionPreference=Stop
        # abort before tiarmclang's real exit code can be inspected.
        $savedErrorActionPreference = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        $compileOutput = & $compiler -c @Flags -o $Object $Source 2>&1
        $ErrorActionPreference = $savedErrorActionPreference
        $exitCode = $LASTEXITCODE
        if ($compileOutput) { $compileOutput | Out-Host }
        if ($exitCode -ne 0) {
            throw "Compile failed: $Source"
        }
    }
    finally {
        if ($null -ne $savedErrorActionPreference) {
            $ErrorActionPreference = $savedErrorActionPreference
        }
        Pop-Location
    }
}

function Invoke-LinkApplication(
    [string]$BuildDir,
    [string]$Name,
    [string[]]$Objects,
    [string[]]$Libraries
) {
    $arguments = @(
        '@device.opt', '-march=thumbv6m', '-mcpu=cortex-m0plus',
        '-mfloat-abi=soft', '-mlittle-endian', '-mthumb', '-O2', '-g',
        '-Wall', '-Werror', "-Wl,-m$Name.map", "-Wl,-i$sdkSource",
        "-Wl,-i$BuildDir", "-Wl,-i$compilerLib", '-Wl,--diag_wrap=off',
        '-Wl,--display_error_number', '-Wl,--warn_sections',
        '-Wl,--rom_model', '-o', "$Name.out"
    )
    $arguments += $Objects
    $arguments += $Libraries
    $arguments += @(
        '-Wl,-l./device_linker.cmd', '-Wl,-ldevice.cmd.genlibs',
        '-Wl,-llibc.a'
    )
    Push-Location $BuildDir
    try {
        $linkOutput = & $compiler @arguments 2>&1
        $exitCode = $LASTEXITCODE
        if ($linkOutput) { $linkOutput | Out-Host }
        if ($exitCode -ne 0) {
            throw "Full application link failed: $Name"
        }
    }
    finally {
        Pop-Location
    }
}

$compilerVersion = (& $compiler --version | Select-Object -First 1)
$results = @()
$sourceManifest = @()

$selectedTargets = @(& $TargetFunction)
$normalizedExtraDefines = @($ExtraDefines | ForEach-Object { $_ -split ',' } |
    Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
if ($TargetNames.Count -gt 0) {
    $requestedTargetNames = @($TargetNames | ForEach-Object { $_ -split ',' } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    $selectedTargets = @($selectedTargets | Where-Object { $requestedTargetNames -contains $_.Name })
    if ($selectedTargets.Count -ne $requestedTargetNames.Count) {
        throw 'One or more requested target names are not in the Round 1 manifest.'
    }
}

foreach ($target in $selectedTargets) {
    $hasFftSource = @($target.AlgorithmSources | Where-Object {
        $_ -eq '04_dsp\fft\signal_fft.c'
    }).Count -gt 0
    $targetFftBackend = if ($null -ne $target.FftBackend) {
        [int]$target.FftBackend
    } else { 0 }
    $effectiveFftBackend = if ($FftBackend -ne 0) {
        $FftBackend
    } elseif ($targetFftBackend -ne 0) {
        $targetFftBackend
    } elseif ($hasFftSource) {
        1
    } else {
        0
    }
    Write-Host "[$($target.Name)] SysConfig generate"
    $buildDir = Join-Path $outputRoot $target.Name
    New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
    $profile = Join-Path $RepoRoot "09_examples\integration_profiles\$($target.Profile)\profile.syscfg"
    $sysOutput = & $sysconfig -s $product --script $profile -o $buildDir --compiler ticlang 2>&1
    $sysExit = $LASTEXITCODE
    $sysOutput | Set-Content -LiteralPath (Join-Path $buildDir 'sysconfig.log') -Encoding utf8
    $sysOutput | Out-Host
    if ($sysExit -ne 0) {
        throw "SysConfig failed: $($target.Name)"
    }
    foreach ($generated in @(
        'ti_msp_dl_config.c', 'ti_msp_dl_config.h', 'device.opt',
        'device_linker.cmd', 'device.cmd.genlibs'
    )) {
        if (-not (Test-Path -LiteralPath (Join-Path $buildDir $generated))) {
            throw "$($target.Name) missing generated file: $generated"
        }
    }
    $generatedHeader = Get-Content -Raw -LiteralPath (Join-Path $buildDir 'ti_msp_dl_config.h')
    if ($generatedHeader -notmatch 'void\s+SYSCFG_DL_init\s*\(void\)') {
        throw "$($target.Name) generated init API mismatch"
    }

    $sources = @(
        (Join-Path $RepoRoot "08_applications\$($target.AppDirectory)\main.c")
    )
    foreach ($relative in $target.ContestSources) {
        $sources += (Join-Path $RepoRoot $relative)
    }
    foreach ($relative in $target.AlgorithmSources) {
        $sources += (Join-Path $AlgorithmRoot $relative)
    }
    foreach ($source in $sources) {
        if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
            throw "$($target.Name) source missing: $source"
        }
        $sourceManifest += [pscustomobject]@{
            target = $target.Name
            source = $source
            sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $source).Hash
        }
    }
    $compileSources = $sources + @(
        (Join-Path $buildDir 'ti_msp_dl_config.c'),
        $startup
    )
    $includeDirs = @(
        $buildDir, $cmsis, $sdkSource,
        (Join-Path $RepoRoot "08_applications\$($target.AppDirectory)"),
        (Join-Path $RepoRoot '08_applications\common'),
        (Join-Path $RepoRoot '01_bsp\common')
    )
    $includeDirs += $sources | ForEach-Object { Split-Path -Parent $_ }
    if ($target.AlgorithmSources.Count -gt 0) {
        $includeDirs += (Join-Path $AlgorithmRoot '03_measurement\common')
    }
    $includeDirs += $cmsisDsp
    $includeDirs = $includeDirs | Sort-Object -Unique
    $flags = @(
        '@device.opt', '-DARM_MATH_CM0', '-march=thumbv6m',
        '-mcpu=cortex-m0plus', '-mfloat-abi=soft', '-mlittle-endian',
        '-mthumb', '-std=c11', '-O2', '-g', '-Wall', '-Werror',
        '-ffunction-sections', '-fdata-sections'
    )
    foreach ($define in $target.Defines) { $flags += '-D' + $define }
    foreach ($define in $normalizedExtraDefines) { $flags += '-D' + $define }
    if ($effectiveFftBackend -ne 0) {
        $flags += '-DSIGNAL_FFT_BACKEND=' + $effectiveFftBackend
        $flags += '-fno-strict-aliasing'
    }
    if ($null -ne $target.MathBackend) {
        $flags += '-DSIGNAL_MATH_BACKEND=' + [int]$target.MathBackend
    }
    if ($SampleCountOverride -gt 0) {
        $flags += '-DSIGNAL_SAMPLE_COUNT=' + $SampleCountOverride
    }
    foreach ($include in $includeDirs) { $flags += '-I' + $include }

    Write-Host "[$($target.Name)] Compile $($compileSources.Count) translation units"
    $objects = @()
    for ($index = 0; $index -lt $compileSources.Count; $index++) {
        $object = 'obj_{0:D2}.o' -f $index
        Invoke-CompileSource $buildDir $compileSources[$index] $object $flags
        $objects += $object
    }

    Write-Host "[$($target.Name)] Full link"
    # ProjectConfig.genLibCMSIS in every canonical profile writes the SDK CMSIS-DSP
    # archive into device.cmd.genlibs. Keep this command-line build identical to CCS.
    $libraries = @()
    Invoke-LinkApplication $buildDir $target.Name $objects $libraries
    $mapPath = Join-Path $buildDir "$($target.Name).map"
    $outPath = Join-Path $buildDir "$($target.Name).out"
    if ((-not (Test-Path -LiteralPath $mapPath)) -or
        (-not (Test-Path -LiteralPath $outPath))) {
        throw "$($target.Name) link artifacts missing"
    }
    $metrics = Get-MapMetrics $mapPath
    $sizeMetrics = Get-TiArmSizeMetrics $outPath
    if (($sizeMetrics.flash_bytes -ne $metrics.flash_used_bytes) -or
        ($sizeMetrics.sram_bytes -ne $metrics.sram_used_bytes_including_stack)) {
        throw "$($target.Name) map/tiarmsize resource mismatch"
    }
    $warningCount = @($sysOutput | Where-Object { $_ -match '(?i)warning' }).Count
    $results += [pscustomobject]@{
        application = $target.DisplayName
        target = $target.Name
        profile = $target.Profile
        projectspec = "08_applications/$($target.AppDirectory)/ticlang/$($target.Name)_LP_MSPM0G3507_nortos_ticlang.projectspec"
        sysconfig = 'PASS'
        sysconfig_warning_count = $warningCount
        compile = 'PASS'
        link = 'PASS'
        compiled_translation_units = $compileSources.Count
        fft_backend = $effectiveFftBackend
        sample_count_override = $SampleCountOverride
        flash_used_bytes = $metrics.flash_used_bytes
        flash_total_bytes = $metrics.flash_total_bytes
        flash_remaining_bytes = $metrics.flash_remaining_bytes
        sram_used_bytes_including_stack = $metrics.sram_used_bytes_including_stack
        sram_total_bytes = $metrics.sram_total_bytes
        stack_reserved_bytes = $metrics.stack_reserved_bytes
        static_sram_bytes_excluding_stack = $metrics.static_sram_bytes_excluding_stack
        sram_remaining_bytes = $metrics.sram_remaining_bytes
        tiarmsize_cross_check = 'PASS'
        large_buffers = $metrics.large_buffers
        map = $mapPath
        output = $outPath
        board = 'NOT_RUN'
        maturity = 'BUILD_VERIFIED'
    }
    Write-Host "[$($target.Name)] PASS Flash=$($metrics.flash_used_bytes) SRAM=$($metrics.sram_used_bytes_including_stack)"
}

$metadata = [pscustomobject]@{
    date = (Get-Date -Format 'yyyy-MM-dd')
    compiler = $compilerVersion
    compiler_path = $compiler
    tiarmsize_path = $sizeTool
    sysconfig_path = $sysconfig
    sdk_root = $SdkRoot
    algorithm_root = $AlgorithmRoot
    requested_fft_backend = $FftBackend
    extra_defines = $normalizedExtraDefines
    sample_count_override = $SampleCountOverride
    target_count = $results.Count
    results = $results
}
$metadata | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $outputRoot 'round1_build_results.json') -Encoding utf8
$sourceManifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $outputRoot 'round1_source_manifest.json') -Encoding utf8
$results | Select-Object application,target,profile,sysconfig,compile,link,
    flash_used_bytes,flash_remaining_bytes,sram_used_bytes_including_stack,
    stack_reserved_bytes,sram_remaining_bytes,maturity |
    Export-Csv -LiteralPath (Join-Path $outputRoot 'round1_build_results.csv') -NoTypeInformation -Encoding utf8
$results | Format-Table application,sysconfig,compile,link,
    flash_used_bytes,sram_used_bytes_including_stack,stack_reserved_bytes,
    sram_remaining_bytes -AutoSize
Write-Output "Round 1 full build/link closure PASS: $($results.Count)/$($results.Count)"
