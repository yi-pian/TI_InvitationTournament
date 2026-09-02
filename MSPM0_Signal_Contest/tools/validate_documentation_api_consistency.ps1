param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..'))
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path
$AlgorithmRoot = (Resolve-Path -LiteralPath (
    Join-Path $RepoRoot '..\MSPM0_Signal_Contest')).Path

$formalRoots = @(
    (Join-Path $RepoRoot '01_bsp'),
    (Join-Path $RepoRoot '02_acquisition'),
    (Join-Path $RepoRoot '06_generator'),
    (Join-Path $RepoRoot '07_signal_frontend'),
    (Join-Path $RepoRoot '08_applications\common'),
    (Join-Path $AlgorithmRoot '03_measurement'),
    (Join-Path $AlgorithmRoot '04_dsp'),
    (Join-Path $AlgorithmRoot '05_precision')
)

$headerFiles = @($formalRoots | ForEach-Object {
    Get-ChildItem -LiteralPath $_ -Recurse -Filter '*.h' -File
})
$legacyCompatibilityRoot = Join-Path $RepoRoot '11_legacy_compatibility\algorithms'
$legacyHeaderFiles = @()
if (Test-Path -LiteralPath $legacyCompatibilityRoot -PathType Container) {
    $legacyHeaderFiles = @(Get-ChildItem -LiteralPath $legacyCompatibilityRoot -Recurse -Filter '*.h' -File)
}
$readmeFiles = @($formalRoots | ForEach-Object {
    Get-ChildItem -LiteralPath $_ -Recurse -Filter 'README.md' -File
} | Where-Object {
    $_.DirectoryName -ne (Join-Path $AlgorithmRoot '03_measurement\common')
})
$headerText = ($headerFiles | ForEach-Object {
    Get-Content -LiteralPath $_.FullName -Encoding utf8 -Raw
}) -join "`n"
$legacyHeaderText = ($legacyHeaderFiles | ForEach-Object {
    Get-Content -LiteralPath $_.FullName -Encoding utf8 -Raw
}) -join "`n"

$errors = [System.Collections.Generic.List[string]]::new()

# Deleted/renamed public function references are not allowed in formal README files.
foreach ($readme in $readmeFiles) {
    $text = Get-Content -LiteralPath $readme.FullName -Encoding utf8 -Raw
    $functionMatches = [regex]::Matches($text,
        '\b(?:Signal[A-Za-z0-9_]+|TFT_ILI9341_[A-Za-z0-9_]+)\s*\(')
    foreach ($match in $functionMatches) {
        $name = $match.Value -replace '\s*\($', ''
        if (($headerText -notmatch ('\b' + [regex]::Escape($name) + '\s*\(')) -and
            ($legacyHeaderText -notmatch ('\b' + [regex]::Escape($name) + '\s*\('))) {
            $errors.Add("STALE_API function '$name' in $($readme.FullName)")
        }
    }

    # Catch stale public struct/typedef names in examples such as removed contexts.
    $typeMatches = [regex]::Matches($text,
        '\b(?:signal|tft)_[a-z0-9_]+_t\b')
    foreach ($match in $typeMatches) {
        $name = $match.Value
        if (($headerText -notmatch ('\b' + [regex]::Escape($name) + '\b')) -and
            ($legacyHeaderText -notmatch ('\b' + [regex]::Escape($name) + '\b'))) {
            $errors.Add("STALE_API type '$name' in $($readme.FullName)")
        }
    }
}

# A COMPILE_VERIFIED block must be byte-for-byte equivalent after newline
# normalization to the real source named by its marker.
$verifiedBlockCount = 0
$markerPattern = '<!--\s*COMPILE_VERIFIED_EXAMPLE:\s*([^>]+?)\s*-->\s*```c\r?\n(.*?)\r?\n```'
foreach ($readme in $readmeFiles) {
    $text = Get-Content -LiteralPath $readme.FullName -Encoding utf8 -Raw
    $matches = [regex]::Matches($text, $markerPattern,
        [System.Text.RegularExpressions.RegexOptions]::Singleline)
    foreach ($match in $matches) {
        ++$verifiedBlockCount
        $relativeSource = $match.Groups[1].Value.Trim() -replace '/', '\'
        $sourcePath = Join-Path $RepoRoot $relativeSource
        if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
            $errors.Add("MISSING_EXAMPLE source '$relativeSource' in $($readme.FullName)")
            continue
        }
        $source = (Get-Content -LiteralPath $sourcePath -Encoding utf8 -Raw) `
            -replace "`r`n", "`n"
        $documented = $match.Groups[2].Value -replace "`r`n", "`n"
        if ($source.TrimEnd() -cne $documented.TrimEnd()) {
            $errors.Add("STALE_API compile-verified block differs from '$relativeSource' in $($readme.FullName)")
        }
    }
}
if ($verifiedBlockCount -lt 2) {
    $errors.Add('MISSING_EXAMPLE expected synchronized DAC DC blocks in module and platform README files')
}

# Every current public MSPM0G3507 adapter entry must remain discoverable from
# the platform README. Signature correctness is then enforced by real examples.
$platformReadmePath = Join-Path $RepoRoot `
    '08_applications\common\mspm0g3507\README.md'
$platformReadme = Get-Content -LiteralPath $platformReadmePath `
    -Encoding utf8 -Raw
$platformHeaders = Get-ChildItem -LiteralPath (Split-Path $platformReadmePath) `
    -Filter '*.h' -File
$platformHeaderText = ($platformHeaders | ForEach-Object {
    Get-Content -LiteralPath $_.FullName -Encoding utf8 -Raw
}) -join "`n"
$platformFunctions = [regex]::Matches($platformHeaderText,
    '\bSignalMSPM0G3507_[A-Za-z0-9_]+\s*\(') |
    ForEach-Object { $_.Value -replace '\s*\($', '' } |
    Sort-Object -Unique
foreach ($name in $platformFunctions) {
    if ($platformReadme -notmatch ('\b' + [regex]::Escape($name) + '\b')) {
        $errors.Add("STALE_API platform README omits public entry '$name'")
    }
}

# Platform-dependent modules must expose the fixed discovery section and the
# relevant formal adapter/example or an explicit API_GAP reference.
$bindings = @(
    @{ Readme='01_bsp\adc\README.md'; Tokens=@('mspm0g3507','adc_basic_minimum') },
    @{ Readme='01_bsp\button\README.md'; Tokens=@('mspm0g3507','test_signal_library.c') },
    @{ Readme='01_bsp\comparator\README.md'; Tokens=@('mspm0g3507','timer_capture_minimum') },
    @{ Readme='01_bsp\dac\README.md'; Tokens=@('mspm0g3507','dac_dc_minimum') },
    @{ Readme='01_bsp\dma\README.md'; Tokens=@('mspm0g3507','adc_dma_minimum') },
    @{ Readme='01_bsp\gpio\README.md'; Tokens=@('mspm0g3507','gpio_minimum') },
    @{ Readme='01_bsp\gpamp\README.md'; Tokens=@('API_GAP','MODULE_INTEGRATION_GAPS.md') },
    @{ Readme='01_bsp\latching_button_switch\README.md'; Tokens=@('mspm0g3507','test_signal_library.c') },
    @{ Readme='01_bsp\matrix_keypad_4x4\README.md'; Tokens=@('mspm0g3507','test_signal_library.c') },
    @{ Readme='01_bsp\opa\README.md'; Tokens=@('API_GAP','MODULE_INTEGRATION_GAPS.md') },
    @{ Readme='01_bsp\tft_ili9341\README.md'; Tokens=@('signal_tft_ili9341_mspm0g3507','tft_ili9341_lp_mspm0g3507'); SectionPattern='## 3\..*SysConfig.*Pin' },
    @{ Readme='01_bsp\timer\README.md'; Tokens=@('mspm0g3507','adc_timer_trigger_minimum') },
    @{ Readme='01_bsp\uart\README.md'; Tokens=@('mspm0g3507','uart_minimum') },
    @{ Readme='02_acquisition\adc_basic\README.md'; Tokens=@('mspm0g3507','adc_basic_minimum') },
    @{ Readme='02_acquisition\adc_continuous\README.md'; Tokens=@('adc_continuous_minimum','signal_adc_frame_callback_t') },
    @{ Readme='02_acquisition\adc_dma\README.md'; Tokens=@('PROFILE_01_ADC_CAPTURE','README_MINIMAL_EXAMPLE.c'); Section='## 3. SysConfig / Pin' },
    @{ Readme='02_acquisition\adc_fifo_dma\README.md'; Tokens=@('PROFILE_08_ADC_FIFO_MAX','README_MINIMAL_EXAMPLE.c'); Section='## 3. SysConfig / Pin' },
    @{ Readme='02_acquisition\adc_dual_sync\README.md'; Tokens=@('signal_dual_adc_mspm0g3507','PROFILE_02_DUAL_ADC'); Section='## 3. SysConfig / Pin' },
    @{ Readme='02_acquisition\adc_timer_trigger\README.md'; Tokens=@('mspm0g3507','adc_timer_trigger_minimum') },
    @{ Readme='02_acquisition\timer_capture\README.md'; Tokens=@('signal_timer_capture_mspm0g3507','PROFILE_05_FREQUENCY'); Section='## 3. SysConfig / Pin' },
    @{ Readme='06_generator\dac_dc\README.md'; Tokens=@('mspm0g3507','dac_dc_minimum') },
    @{ Readme='06_generator\dac_dma\README.md'; Tokens=@('signal_dac_dma_mspm0g3507','PROFILE_03_DAC_GENERATOR'); Section='## 3. SysConfig / Pin' },
    @{ Readme='07_signal_frontend\comparator_threshold\README.md'; Tokens=@('mspm0g3507','timer_capture_minimum') },
    @{ Readme='07_signal_frontend\comparator_zero_cross\README.md'; Tokens=@('mspm0g3507','timer_capture_minimum') },
    @{ Readme='07_signal_frontend\gpamp_buffer\README.md'; Tokens=@('API_GAP','MODULE_INTEGRATION_GAPS.md') },
    @{ Readme='07_signal_frontend\gpamp_gain\README.md'; Tokens=@('API_GAP','MODULE_INTEGRATION_GAPS.md') },
    @{ Readme='07_signal_frontend\opa_buffer\README.md'; Tokens=@('API_GAP','MODULE_INTEGRATION_GAPS.md') },
    @{ Readme='07_signal_frontend\opa_inverting\README.md'; Tokens=@('API_GAP','MODULE_INTEGRATION_GAPS.md') },
    @{ Readme='07_signal_frontend\opa_noninverting_pga\README.md'; Tokens=@('API_GAP','MODULE_INTEGRATION_GAPS.md') },
    @{ Readme='08_applications\common\dac_dma_platform_adapter\README.md'; Tokens=@('signal_dac_dma_platform','dac_dma_minimum') },
    @{ Readme='08_applications\common\dual_adc_platform_adapter\README.md'; Tokens=@('signal_dual_adc_platform','dual_channel_phase_meter') },
    @{ Readme='08_applications\common\integration_glue\README.md'; Tokens=@('Not Applicable','signal_integration.c') },
    @{ Readme='08_applications\common\mspm0g3507\README.md'; Tokens=@('platform_closure','dac_dc_minimum') }
)
foreach ($binding in $bindings) {
    $path = Join-Path $RepoRoot $binding.Readme
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        $errors.Add("MISSING_PLATFORM_LINK README '$($binding.Readme)'")
        continue
    }
    $text = Get-Content -LiteralPath $path -Encoding utf8 -Raw
    $sectionPattern = if ($binding.ContainsKey('SectionPattern')) {
        $binding.SectionPattern
    } elseif ($binding.ContainsKey('Section')) {
        [regex]::Escape($binding.Section)
    } else {
        [regex]::Escape('## Hardware / Platform Binding')
    }
    if ($text -notmatch ('(?m)^' + $sectionPattern + '\s*$')) {
        $errors.Add("MISSING_PLATFORM_LINK section '$sectionPattern' in '$($binding.Readme)'")
    }
    foreach ($token in $binding.Tokens) {
        if ($text -notmatch [regex]::Escape($token)) {
            $errors.Add("MISSING_PLATFORM_LINK token '$token' in '$($binding.Readme)'")
        }
    }
}

$result = [pscustomobject]@{
    date = (Get-Date -Format 'yyyy-MM-dd')
    formal_readmes_scanned = $readmeFiles.Count
    formal_headers_scanned = $headerFiles.Count
    platform_public_functions = $platformFunctions.Count
    compile_verified_blocks = $verifiedBlockCount
    binding_readmes_checked = $bindings.Count
    errors = $errors.Count
    issues = @($errors | Sort-Object -Unique)
    status = if ($errors.Count -eq 0) { 'PASS' } else { 'FAIL' }
}

$result | Format-List
$resultDirectory = Join-Path $RepoRoot `
    '10_tests\documentation_api_consistency'
New-Item -ItemType Directory -Force -Path $resultDirectory | Out-Null
$result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (
    Join-Path $resultDirectory 'documentation_api_consistency_results.json') `
    -Encoding utf8
if ($errors.Count -ne 0) {
    $errors | Sort-Object -Unique | ForEach-Object { Write-Error $_ }
    exit 1
}
Write-Output 'Documentation/API consistency check PASS.'
