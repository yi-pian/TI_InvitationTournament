param(
    [string]$RepoRoot = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = "Stop"
$algorithmRoot = (Resolve-Path (Join-Path $RepoRoot '..\MSPM0_Signal_Contest')).Path
$applicationRoot = Join-Path $RepoRoot '08_applications'
$specs = Get-ChildItem -LiteralPath $applicationRoot -Recurse -Filter "*.projectspec"
if ($specs.Count -eq 0) { throw "No .projectspec files found" }

$results = @()
foreach ($spec in ($specs | Sort-Object FullName)) {
    $text = Get-Content -LiteralPath $spec.FullName -Raw
    $isCopyTemplate = $spec.FullName -like '*\signal_contest_template\*'
    if ($text -match '\$\{PROJECT_ROOT\}/modules/') {
        throw "Virtual folder used as compiler include path: $($spec.FullName)"
    }
    if ($text -match '\$\{PROJECT_ROOT\}/\.\./') {
        throw "Non-portable PROJECT_ROOT parent include: $($spec.FullName)"
    }
    if (-not $text.Contains('-Werror')) {
        throw "Missing -Werror in compilerBuildOptions: $($spec.FullName)"
    }
    foreach ($cmsisOption in @('-DARM_MATH_CM0', 'CMSIS/DSP/Include', '-ldevice.cmd.genlibs')) {
        if (-not $text.Contains($cmsisOption)) {
            throw "Missing CMSIS-ready option '$cmsisOption': $($spec.FullName)"
        }
    }
    if ($isCopyTemplate) {
        if (-not $text.Contains('-I${PROJECT_ROOT}/modules')) {
            throw "Copy template misses local modules include: $($spec.FullName)"
        }
        if ($text.Contains('MSPM0_SIGNAL_LIBRARY_ROOT')) {
            throw "Copy template still depends on library root: $($spec.FullName)"
        }
        if ($text -match 'action="link"') {
            throw "Copy template contains linked source: $($spec.FullName)"
        }
        $copyCount = 0
        [regex]::Matches($text, '<file\s+path="([^"]+)"[^>]+action="copy"') |
            ForEach-Object {
                $copied = $_.Groups[1].Value.Replace('/', '\')
                $resolved = [System.IO.Path]::GetFullPath((
                    Join-Path $spec.DirectoryName $copied))
                if (-not (Test-Path -LiteralPath $resolved -PathType Leaf)) {
                    throw "Template copy source does not exist: $copied -> $resolved"
                }
                $copyCount++
            }
        if ($copyCount -lt 6) {
            throw "Copy template has too few local project files: $($spec.FullName)"
        }
        $results += [pscustomobject]@{
            projectspec = $spec.BaseName
            physical_include_paths = "PASS (local modules)"
            linked_files = "0 (copy policy)"
            wall_werror = "PASS"
            virtual_folder_as_include = "NO"
        }
        continue
    }
    if (-not $text.Contains('<pathVariable name="APP_SOURCE_ROOT" path="../" scope="project"/>')) {
        throw "Missing project-scoped APP_SOURCE_ROOT: $($spec.FullName)"
    }
    if ($text.Contains('arm_cortexM0l_math.a') -and
        -not $text.Contains('-l${COM_TI_MSPM0_SDK_INSTALL_DIR}/source/third_party/CMSIS/DSP/Lib/ticlang/m0p/arm_cortexM0l_math.a')) {
        throw "CMSIS archive is not expressed as a CCS-import-safe -l option: $($spec.FullName)"
    }
    $requiredIncludes = @(
        '-I${MSPM0_SIGNAL_LIBRARY_ROOT}/MSPM0_Signal_Contest/01_bsp/common',
        '-I${APP_SOURCE_ROOT}'
    )
    if ($text.Contains('signal_adc_dma.c')) {
        $requiredIncludes += '-I${MSPM0_SIGNAL_LIBRARY_ROOT}/MSPM0_Signal_Contest/02_acquisition/adc_dma'
    }
    foreach ($requiredInclude in $requiredIncludes) {
        if (-not $text.Contains($requiredInclude)) {
            throw "Missing physical include path '$requiredInclude': $($spec.FullName)"
        }
    }

    $linkedCount = 0
    [regex]::Matches($text, '<file\s+path="([^"]+)"[^>]+action="(link|copy)"') | ForEach-Object {
        $linked = $_.Groups[1].Value.Replace('/', '\')
        if ($linked.StartsWith('MSPM0_SIGNAL_LIBRARY_ROOT\MSPM0_Signal_Contest\')) {
            $relative = $linked.Substring('MSPM0_SIGNAL_LIBRARY_ROOT\MSPM0_Signal_Contest\'.Length)
            $resolved = [System.IO.Path]::GetFullPath((Join-Path $RepoRoot $relative))
        }
        elseif ($linked.StartsWith('MSPM0_SIGNAL_LIBRARY_ROOT\MSPM0_Signal_Contest\')) {
            $relative = $linked.Substring('MSPM0_SIGNAL_LIBRARY_ROOT\MSPM0_Signal_Contest\'.Length)
            $resolved = [System.IO.Path]::GetFullPath((Join-Path $algorithmRoot $relative))
        }
        else {
            $resolved = [System.IO.Path]::GetFullPath((Join-Path $spec.DirectoryName $linked))
        }
        if (-not (Test-Path -LiteralPath $resolved -PathType Leaf)) {
            throw "Linked source does not exist: $linked -> $resolved"
        }
        $insideContest = $resolved.StartsWith(
            $RepoRoot, [System.StringComparison]::OrdinalIgnoreCase)
        $insideAlgorithm = $resolved.StartsWith(
            $algorithmRoot, [System.StringComparison]::OrdinalIgnoreCase)
        if ((-not $insideContest) -and (-not $insideAlgorithm)) {
            throw "Linked source escapes approved module repositories: $resolved"
        }
        if ($_.Groups[2].Value -eq 'link') { $linkedCount++ }
    }
    $minimumLinked = if ($text.Contains('signal_adc_dma.c')) { 3 } else { 2 }
    if ($linkedCount -lt $minimumLinked) {
        throw "Insufficient linked single-source-of-truth files: $($spec.FullName)"
    }

    $results += [pscustomobject]@{
        projectspec = $spec.BaseName
        physical_include_paths = "PASS"
        linked_files = "$linkedCount PASS"
        wall_werror = "PASS"
        virtual_folder_as_include = "NO"
    }
}

$results | Format-Table -AutoSize
Write-Host "Projectspec path check PASS: $($specs.Count) files."
