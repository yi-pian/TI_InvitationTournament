param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')),
    [string]$AlgorithmRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')),
    [switch]$Q31Variants,
    [string]$TargetManifestPath = (Join-Path $PSScriptRoot 'round1_integration_targets.ps1'),
    [string]$TargetFunction = 'Get-Round1IntegrationTargets'
)

$ErrorActionPreference = 'Stop'
. $TargetManifestPath

function To-ForwardSlash([string]$Path) {
    return $Path.Replace('\', '/')
}

function Xml-Escape([string]$Text) {
    return [System.Security.SecurityElement]::Escape($Text)
}

function Get-RelativePath([string]$BaseDirectory, [string]$TargetPath) {
    $separator = [System.IO.Path]::DirectorySeparatorChar
    $baseWithSeparator = $BaseDirectory.TrimEnd($separator) + $separator
    $baseUri = [System.Uri]::new($baseWithSeparator)
    $targetUri = [System.Uri]::new($TargetPath)
    return [System.Uri]::UnescapeDataString(
        $baseUri.MakeRelativeUri($targetUri).ToString())
}

$targets = @(& $TargetFunction)
if ($Q31Variants) {
    $q31TargetNames = @(
        'frequency_meter_c_round1', 'spectrum_analyzer_round1',
        'harmonic_thd_analyzer_round1', 'dual_channel_phase_meter_round1'
    )
    $targets = @($targets | Where-Object { $q31TargetNames -contains $_.Name })
}

foreach ($target in $targets) {
    $targetFftBackend = if ($null -ne $target.FftBackend) { [int]$target.FftBackend } else { 0 }
    $effectiveQ31 = $Q31Variants -or ($targetFftBackend -eq 2)
    $targetName = if ($Q31Variants) {
        $target.Name.Replace('_round1', '_q31')
    } else {
        $target.Name
    }
    $appDir = Join-Path $RepoRoot "08_applications\$($target.AppDirectory)"
    $specDir = Join-Path $appDir 'ticlang'
    $projectRoot = Join-Path $specDir $target.Name
    New-Item -ItemType Directory -Force -Path $specDir | Out-Null

    $sourceFiles = @()
    foreach ($relative in $target.ContestSources) {
        $sourceFiles += (Join-Path $RepoRoot $relative)
    }
    foreach ($relative in $target.AlgorithmSources) {
        $sourceFiles += (Join-Path $AlgorithmRoot $relative)
    }

    $includeDirs = @(
        (Join-Path $RepoRoot '01_bsp\common'),
        (Join-Path $RepoRoot '08_applications\common')
    )
    $includeDirs += $sourceFiles | ForEach-Object { Split-Path -Parent $_ }
    if ($target.AlgorithmSources.Count -gt 0) {
        $includeDirs += (Join-Path $AlgorithmRoot '03_measurement\common')
    }
    if ($effectiveQ31) {
        $includeDirs += (Join-Path $AlgorithmRoot '04_dsp\fft')
    }
    $includeDirs = $includeDirs | Sort-Object -Unique

    $compilerOptions = @(
        '-I${PROJECT_ROOT}',
        '-I${PROJECT_ROOT}/${ConfigName}',
        '-I${APP_SOURCE_ROOT}'
    )
    foreach ($dir in $includeDirs) {
        if ($dir.StartsWith($RepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
            $relative = To-ForwardSlash (Get-RelativePath $RepoRoot $dir)
            $compilerOptions += '-I${MSPM0_SIGNAL_LIBRARY_ROOT}/MSPM0_Signal_Contest/' + $relative
        }
        elseif ($dir.StartsWith($AlgorithmRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
            $relative = To-ForwardSlash (Get-RelativePath $AlgorithmRoot $dir)
            $compilerOptions += '-I${MSPM0_SIGNAL_LIBRARY_ROOT}/MSPM0_Signal_Contest/' + $relative
        }
        else {
            throw "Include directory is outside the approved repositories: $dir"
        }
    }
    foreach ($define in $target.Defines) {
        $compilerOptions += '-D' + $define
    }
    $compilerOptions += @(
        '-DARM_MATH_CM0',
        '-I${COM_TI_MSPM0_SDK_INSTALL_DIR}/source/third_party/CMSIS/DSP/Include'
    )
    if ($effectiveQ31) {
        $compilerOptions += @('-DSIGNAL_FFT_BACKEND=2', '-fno-strict-aliasing')
    }
    if ($null -ne $target.MathBackend) {
        $compilerOptions += '-DSIGNAL_MATH_BACKEND=' + [int]$target.MathBackend
    }
    $compilerOptions += @(
        '-std=c11', '-O2', '-g', '-Wall', '-Werror',
        '-ffunction-sections', '-fdata-sections', '@device.opt',
        '-I${COM_TI_MSPM0_SDK_INSTALL_DIR}/source/third_party/CMSIS/Core/Include',
        '-I${COM_TI_MSPM0_SDK_INSTALL_DIR}/source',
        '-mcpu=cortex-m0plus', '-march=thumbv6m', '-mfloat-abi=soft', '-mthumb'
    )
    $compilerText = ($compilerOptions | ForEach-Object { '            ' + $_ }) -join "`n"

    $fileLines = @()
    foreach ($name in @('main.c', 'signal_config.h', 'README.md')) {
        $fileLines += "        <file path=`"../$name`" openOnCreation=`"false`" excludeFromBuild=`"false`" action=`"copy`" />"
    }
    $profileRelative = "09_examples/integration_profiles/$($target.Profile)/profile.syscfg"
    $fileLines += "        <file path=`"MSPM0_SIGNAL_LIBRARY_ROOT/MSPM0_Signal_Contest/$profileRelative`" openOnCreation=`"true`" excludeFromBuild=`"false`" action=`"link`" />"
    $sourceIndex = 0
    foreach ($source in $sourceFiles) {
        if ($source.StartsWith($AlgorithmRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
            $relative = To-ForwardSlash (Get-RelativePath $AlgorithmRoot $source)
            $portablePath = "MSPM0_SIGNAL_LIBRARY_ROOT/MSPM0_Signal_Contest/$relative"
            $group = 'algorithm'
        }
        elseif ($source.StartsWith($RepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
            $relative = To-ForwardSlash (Get-RelativePath $RepoRoot $source)
            $portablePath = "MSPM0_SIGNAL_LIBRARY_ROOT/MSPM0_Signal_Contest/$relative"
            $group = 'peripheral_and_glue'
        }
        else {
            throw "Source is outside the approved repositories: $source"
        }
        $fileLines += "        <file path=`"$portablePath`" targetDirectory=`"sources/$group`" createVirtualFolders=`"true`" openOnCreation=`"false`" excludeFromBuild=`"false`" action=`"link`" />"
        $sourceIndex++
    }
    $filesText = $fileLines -join "`n"
    $description = if ($effectiveQ31) {
        Xml-Escape("CMSIS-ready integration, Q31 FFT selected: $($target.DisplayName)")
    } else {
        Xml-Escape("CMSIS-ready integration baseline: $($target.DisplayName)")
    }

    $xml = @"
<?xml version="1.0" encoding="UTF-8"?>
<projectSpec>
    <applicability>
        <when><context deviceFamily="ARM" deviceId="MSPM0G3507" /></when>
    </applicability>
    <project
        name="$targetName"
        configurations="Debug"
        toolChain="TICLANG"
        connection="TIXDS110_Connection.xml"
        device="MSPM0G3507"
        ignoreDefaultDeviceSettings="true"
        ignoreDefaultCCSSettings="true"
        products="MSPM0-SDK;sysconfig"
        compilerBuildOptions="
$compilerText
        "
        linkerBuildOptions="
            -ldevice.cmd.genlibs
            -L`${COM_TI_MSPM0_SDK_INSTALL_DIR}/source
            -L`${PROJECT_ROOT}
            -L`${PROJECT_BUILD_DIR}/syscfg
            -Wl,--rom_model
            -Wl,--warn_sections
            -L`${CG_TOOL_ROOT}/lib
            -llibc.a
        "
        sysConfigBuildOptions="
            --output .
            --product `${COM_TI_MSPM0_SDK_INSTALL_DIR}/.metadata/product.json
            --compiler ticlang
        "
        sourceLookupPath="`${COM_TI_MSPM0_SDK_INSTALL_DIR}/source/ti/driverlib"
        description="$description">
        <pathVariable name="APP_SOURCE_ROOT" path="../" scope="project"/>
        <property name="buildProfile" value="release"/>
        <property name="isHybrid" value="true"/>
$filesText
    </project>
</projectSpec>
"@
    $specPath = Join-Path $specDir "$targetName`_LP_MSPM0G3507_nortos_ticlang.projectspec"
    [System.IO.File]::WriteAllText($specPath, $xml, [System.Text.UTF8Encoding]::new($false))
    Write-Output "generated $specPath ($sourceIndex linked sources)"
}
