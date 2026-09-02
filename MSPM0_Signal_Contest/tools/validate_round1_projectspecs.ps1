param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')),
    [string]$AlgorithmRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')),
    [switch]$Q31Variants,
    [string]$TargetManifestPath = (Join-Path $PSScriptRoot 'round1_integration_targets.ps1'),
    [string]$TargetFunction = 'Get-Round1IntegrationTargets',
    [string]$OutputDirectoryName = ''
)

$ErrorActionPreference = 'Stop'
. $TargetManifestPath
$outputDirectory = if ($OutputDirectoryName) {
    $OutputDirectoryName
} elseif ($Q31Variants) { 'round1_backend_q31' } else { 'round1_build_closure' }
$output = Join-Path $RepoRoot (Join-Path '10_tests\integration' $outputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null

function Normalize([string]$Path) {
    return [System.IO.Path]::GetFullPath($Path).TrimEnd('\')
}

$results = @()
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
    $targetName = if ($Q31Variants) { $target.Name.Replace('_round1', '_q31') } else { $target.Name }
    $specDir = Join-Path $RepoRoot "08_applications\$($target.AppDirectory)\ticlang"
    $specPath = Join-Path $specDir "$targetName`_LP_MSPM0G3507_nortos_ticlang.projectspec"
    if (-not (Test-Path -LiteralPath $specPath -PathType Leaf)) {
        throw "Missing projectspec: $specPath"
    }
    [xml]$xml = Get-Content -Raw -Encoding utf8 -LiteralPath $specPath
    $project = $xml.projectSpec.project
    if ($project.name -ne $targetName) {
        throw "Project name mismatch: $specPath"
    }
    $options = [string]$project.compilerBuildOptions
    foreach ($required in @(
        '-Wall', '-Werror', '-std=c11', '@device.opt',
        '-DARM_MATH_CM0', 'CMSIS/DSP/Include',
        '${COM_TI_MSPM0_SDK_INSTALL_DIR}/source',
        '-I${PROJECT_ROOT}/${ConfigName}',
        '-I${APP_SOURCE_ROOT}',
        '${MSPM0_SIGNAL_LIBRARY_ROOT}/MSPM0_Signal_Contest/'
    )) {
        if (-not $options.Contains($required)) {
            throw "$($target.Name) missing compiler option: $required"
        }
    }
    if ($target.AlgorithmSources.Count -gt 0 -and
        -not $options.Contains('${MSPM0_SIGNAL_LIBRARY_ROOT}/MSPM0_Signal_Contest/')) {
        throw "$($target.Name) missing portable algorithm repository include"
    }
    if ($options.Contains('${PROJECT_ROOT}/modules/')) {
        throw "$($target.Name) uses virtual folder as physical include"
    }
    if ($options.Contains('${PROJECT_ROOT}/../')) {
        throw "$($target.Name) uses a non-portable PROJECT_ROOT parent include"
    }
    $appRootVariable = @($project.pathVariable | Where-Object {
        $_.name -eq 'APP_SOURCE_ROOT' -and $_.path -eq '../' -and $_.scope -eq 'project'
    })
    if ($appRootVariable.Count -ne 1) {
        throw "$($target.Name) must define project-scoped APP_SOURCE_ROOT"
    }
    foreach ($define in $target.Defines) {
        if (-not $options.Contains("-D$define")) {
            throw "$($target.Name) missing define $define"
        }
    }
    if ($effectiveQ31) {
        foreach ($requiredQ31 in @(
            '-DARM_MATH_CM0', '-DSIGNAL_FFT_BACKEND=2',
            '-fno-strict-aliasing', 'CMSIS/DSP/Include'
        )) {
            if (-not $options.Contains($requiredQ31)) {
                throw "$targetName missing Q31 option: $requiredQ31"
            }
        }
    }
    if (-not ([string]$project.linkerBuildOptions).Contains('-ldevice.cmd.genlibs')) {
        throw "$targetName missing SysConfig generated library list"
    }
    $sdkSourceToken = [regex]::Escape(
        '${COM_TI_MSPM0_SDK_INSTALL_DIR}/source')
    if (([string]$project.linkerBuildOptions) -notmatch $sdkSourceToken) {
        throw "$($target.Name) missing SDK linker path"
    }
    $sdkProductToken = [regex]::Escape(
        '${COM_TI_MSPM0_SDK_INSTALL_DIR}/.metadata/product.json')
    if (([string]$project.sysConfigBuildOptions) -notmatch $sdkProductToken) {
        throw "$($target.Name) missing SDK SysConfig product path"
    }

    $expectedSources = @()
    foreach ($relative in $target.ContestSources) {
        $expectedSources += Normalize (Join-Path $RepoRoot $relative)
    }
    foreach ($relative in $target.AlgorithmSources) {
        $expectedSources += Normalize (Join-Path $AlgorithmRoot $relative)
    }
    $expectedSources = $expectedSources | Sort-Object -Unique

    $actualSources = @()
    $profileCount = 0
    foreach ($file in $project.file) {
        $portablePath = ([string]$file.path).Replace('/', '\')
        if ($portablePath.StartsWith('MSPM0_SIGNAL_LIBRARY_ROOT\MSPM0_Signal_Contest\')) {
            $relative = $portablePath.Substring('MSPM0_SIGNAL_LIBRARY_ROOT\MSPM0_Signal_Contest\'.Length)
            $resolved = Normalize (Join-Path $RepoRoot $relative)
        }
        else {
            $resolved = Normalize (Join-Path $specDir $portablePath)
        }
        if (-not (Test-Path -LiteralPath $resolved -PathType Leaf)) {
            throw "$($target.Name) linked file missing: $resolved"
        }
        if ($resolved.EndsWith('.syscfg', [System.StringComparison]::OrdinalIgnoreCase)) {
            $profileRelative = "09_examples\integration_profiles\$($target.Profile)\profile.syscfg"
            $expectedProfile = Normalize (Join-Path $RepoRoot $profileRelative)
            if ($resolved -ne $expectedProfile) {
                throw "$($target.Name) wrong SysConfig profile: $resolved"
            }
            if ($file.action -ne 'link') {
                throw "$($target.Name) SysConfig must be linked, not copied"
            }
            $profileText = Get-Content -Raw -Encoding utf8 -LiteralPath $resolved
            if (($profileText -notmatch 'ProjectConfig') -or
                ($profileText -notmatch 'genLibCMSIS\s*=\s*true')) {
                throw "$($target.Name) profile is not CMSIS-DSP-ready"
            }
            $profileCount++
        }
        elseif ($resolved.EndsWith('.c', [System.StringComparison]::OrdinalIgnoreCase) -and
            ([string]$file.action -eq 'link')) {
            $actualSources += $resolved
        }
    }
    $actualSources = $actualSources | Sort-Object -Unique
    if ($profileCount -ne 1) {
        throw "$($target.Name) must link exactly one SysConfig profile"
    }
    if (Compare-Object $expectedSources $actualSources) {
        throw "$($target.Name) projectspec source set differs from target manifest"
    }
    foreach ($algorithmSource in $target.AlgorithmSources) {
        $absolute = Normalize (Join-Path $AlgorithmRoot $algorithmSource)
        if (-not ($actualSources -contains $absolute)) {
            throw "$($target.Name) algorithm source is not linked from canonical repository"
        }
    }
    $results += [pscustomobject]@{
        application = $target.DisplayName
        target = $targetName
        projectspec = $specPath.Substring($RepoRoot.Length + 1)
        profile = $target.Profile
        linked_c_sources = $actualSources.Count
        algorithm_source_of_truth = 'PASS'
        peripheral_source_of_truth = 'PASS'
        include_paths = 'PASS'
        sysconfig_path = 'PASS'
        sdk_paths = 'PASS'
        xml = 'PASS'
    }
}

$resultPath = Join-Path $output 'projectspec_validation.json'
$results | ConvertTo-Json | Set-Content -LiteralPath $resultPath -Encoding utf8
$results | Format-Table -AutoSize
Write-Output "Round 1 projectspec validation PASS: $($results.Count)/$($results.Count)"
