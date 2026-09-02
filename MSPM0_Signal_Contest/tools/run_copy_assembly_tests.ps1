param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')),
    [string]$AlgorithmRoot = $RepoRoot,
    [string]$SdkRoot = 'C:\TI\mspm0_sdk_2_11_00_07',
    [string]$CcsRoot = 'D:\TI\CCS'
)

$ErrorActionPreference = 'Stop'

$sysconfig = Join-Path $CcsRoot 'ccs\utils\sysconfig_1.28.0\sysconfig_cli.bat'
$compiler = Join-Path $CcsRoot 'ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\bin\tiarmclang.exe'
$compilerLib = Join-Path $CcsRoot 'ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\lib'
$product = Join-Path $SdkRoot '.metadata\product.json'
$sdkSource = Join-Path $SdkRoot 'source'
$cmsis = Join-Path $sdkSource 'third_party\CMSIS\Core\Include'
$startup = Join-Path $sdkSource 'ti\devices\msp\m0p\startup_system_files\ticlang\startup_mspm0g350x_ticlang.c'
$buildRoot = Join-Path $RepoRoot '10_tests\copy_assembly\build'
$blankProfile = Join-Path $RepoRoot '08_applications\signal_contest_template\signal_contest_template.syscfg'

$contestStatus = '01_bsp\common\signal_status.h'
$algorithmStatus = '03_measurement\common\signal_algorithm_status.h'
$algorithmMath = @(
    '03_measurement\common\signal_math_backend.h',
    '03_measurement\common\signal_math_backend_config.h'
)

function New-CopyTarget {
    param(
        [string]$Name,
        [ValidateSet('contest', 'algorithm')][string]$Root,
        [string]$Main,
        [string[]]$Files,
        [string]$Profile
    )
    [pscustomobject]@{
        Name = $Name
        Root = $Root
        Main = $Main
        Files = $Files
        Profile = $Profile
    }
}

$targets = @(
    (New-CopyTarget 'adc_dma' 'contest' `
        '02_acquisition\adc_dma\README_MINIMAL_EXAMPLE.c' @(
            '02_acquisition\adc_dma\signal_adc_dma.c',
            '02_acquisition\adc_dma\signal_adc_dma.h', $contestStatus) `
        (Join-Path $RepoRoot '09_examples\integration_profiles\PROFILE_01_ADC_CAPTURE\profile.syscfg')),
    (New-CopyTarget 'adc_fifo_dma' 'contest' `
        '02_acquisition\adc_fifo_dma\README_MINIMAL_EXAMPLE.c' @(
            '02_acquisition\adc_fifo_dma\signal_adc_fifo_dma.c',
            '02_acquisition\adc_fifo_dma\signal_adc_fifo_dma.h', $contestStatus) `
        (Join-Path $RepoRoot '09_examples\integration_profiles\PROFILE_08_ADC_FIFO_MAX\profile.syscfg')),
    (New-CopyTarget 'dual_adc' 'contest' `
        '02_acquisition\adc_dual_sync\README_MINIMAL_EXAMPLE.c' @(
            '02_acquisition\adc_dual_sync\signal_dual_adc_mspm0g3507.c',
            '02_acquisition\adc_dual_sync\signal_dual_adc_mspm0g3507.h',
            $contestStatus) `
        (Join-Path $RepoRoot '09_examples\integration_profiles\PROFILE_02_DUAL_ADC\profile.syscfg')),
    (New-CopyTarget 'timer_capture' 'contest' `
        '02_acquisition\timer_capture\README_MINIMAL_EXAMPLE.c' @(
            '02_acquisition\timer_capture\signal_timer_capture_mspm0g3507.c',
            '02_acquisition\timer_capture\signal_timer_capture_mspm0g3507.h',
            $contestStatus) `
        (Join-Path $RepoRoot '09_examples\integration_profiles\PROFILE_05_FREQUENCY\profile.syscfg')),
    (New-CopyTarget 'dac_dma' 'contest' `
        '06_generator\dac_dma\README_MINIMAL_EXAMPLE.c' @(
            '06_generator\dac_dma\signal_dac_dma_mspm0g3507.c',
            '06_generator\dac_dma\signal_dac_dma_mspm0g3507.h',
            $contestStatus) `
        (Join-Path $RepoRoot '09_examples\integration_profiles\PROFILE_03_DAC_GENERATOR\profile.syscfg')),
    (New-CopyTarget 'dds' 'contest' `
        '06_generator\dds\README_MINIMAL_EXAMPLE.c' @(
            '06_generator\dds\signal_dds.c',
            '06_generator\dds\signal_dds.h', $contestStatus) $blankProfile),
    (New-CopyTarget 'tft_ili9341' 'contest' `
        '01_bsp\tft_ili9341\README_MINIMAL_EXAMPLE.c' @(
            '01_bsp\tft_ili9341\signal_tft_ili9341.c',
            '01_bsp\tft_ili9341\signal_tft_ili9341.h',
            '01_bsp\tft_ili9341\signal_tft_ili9341_font_data.inc',
            '01_bsp\tft_ili9341\signal_tft_ili9341_mspm0g3507.c',
            '01_bsp\tft_ili9341\signal_tft_ili9341_mspm0g3507.h',
            $contestStatus) `
        (Join-Path $RepoRoot '09_examples\tft_ili9341_lp_mspm0g3507\tft_ili9341.syscfg')),
    (New-CopyTarget 'ssd1306' 'contest' `
        '12_external_devices\display\ssd1306\README_MINIMAL_EXAMPLE.c' @(
            '01_bsp\common\signal_status.h',
            '12_external_devices\display\ssd1306\ssd1306.c',
            '12_external_devices\display\ssd1306\ssd1306.h',
            '12_external_devices\display\ssd1306\ssd1306_font_6x8.inc',
            '12_external_devices\display\ssd1306\ssd1306_mspm0_i2c.c',
            '12_external_devices\display\ssd1306\ssd1306_mspm0_i2c.h',
            '12_external_devices\display\ssd1306\ssd1306_mspm0g3507.c',
            '12_external_devices\display\ssd1306\ssd1306_mspm0g3507.h',
            '12_external_devices\00_common\mspm0_blocking_bus.c',
            '12_external_devices\00_common\mspm0_blocking_bus.h') `
        (Join-Path $RepoRoot '09_examples\ssd1306_lp_mspm0g3507\ssd1306.syscfg')),
    (New-CopyTarget 'st7789' 'contest' `
        '12_external_devices\display\st7789\README_MINIMAL_EXAMPLE.c' @(
            '01_bsp\common\signal_status.h',
            '12_external_devices\display\st7789\signal_tft_st7789.c',
            '12_external_devices\display\st7789\signal_tft_st7789.h',
            '12_external_devices\display\st7789\signal_tft_st7789_mspm0g3507.c',
            '12_external_devices\display\st7789\signal_tft_st7789_mspm0g3507.h') `
        (Join-Path $RepoRoot '09_examples\tft_ili9341_lp_mspm0g3507\tft_ili9341.syscfg')),

    (New-CopyTarget 'adc_to_voltage' 'algorithm' `
        '03_measurement\adc_to_voltage\README_MINIMAL_EXAMPLE.c' @(
            '03_measurement\adc_to_voltage\signal_adc_to_voltage.c',
            '03_measurement\adc_to_voltage\signal_adc_to_voltage.h',
            $algorithmStatus) $blankProfile),
    (New-CopyTarget 'vpp' 'algorithm' `
        '03_measurement\vpp\README_MINIMAL_EXAMPLE.c' @(
            '03_measurement\vpp\signal_vpp.c',
            '03_measurement\vpp\signal_vpp.h', $algorithmStatus) $blankProfile),
    (New-CopyTarget 'rms' 'algorithm' `
        '03_measurement\rms\README_MINIMAL_EXAMPLE.c' (@(
            '03_measurement\rms\signal_rms.c',
            '03_measurement\rms\signal_rms.h', $algorithmStatus) + `
            $algorithmMath) $blankProfile),
    (New-CopyTarget 'ac_rms' 'algorithm' `
        '03_measurement\ac_rms\README_MINIMAL_EXAMPLE.c' (@(
            '03_measurement\ac_rms\signal_ac_rms.c',
            '03_measurement\ac_rms\signal_ac_rms.h', $algorithmStatus) + `
            $algorithmMath) $blankProfile),
    (New-CopyTarget 'remove_dc' 'algorithm' `
        '04_dsp\remove_dc\README_MINIMAL_EXAMPLE.c' @(
            '04_dsp\remove_dc\signal_remove_dc.c',
            '04_dsp\remove_dc\signal_remove_dc.h', $algorithmStatus) $blankProfile),
    (New-CopyTarget 'window' 'algorithm' `
        '04_dsp\window\README_MINIMAL_EXAMPLE.c' @(
            '04_dsp\window\signal_window.c',
            '04_dsp\window\signal_window.h', $algorithmStatus) $blankProfile),
    (New-CopyTarget 'fft' 'algorithm' `
        '04_dsp\fft\README_MINIMAL_EXAMPLE.c' @(
            '04_dsp\fft\signal_fft.c',
            '04_dsp\fft\signal_fft.h',
            '04_dsp\fft\signal_fft_backend_config.h',
            '03_measurement\common\signal_complex.h', $algorithmStatus) $blankProfile),
    (New-CopyTarget 'fft_magnitude' 'algorithm' `
        '04_dsp\fft_magnitude\README_MINIMAL_EXAMPLE.c' @(
            '04_dsp\fft_magnitude\signal_fft_magnitude.c',
            '04_dsp\fft_magnitude\signal_fft_magnitude.h',
            '03_measurement\common\signal_complex.h', $algorithmStatus) $blankProfile),
    (New-CopyTarget 'peak_detect' 'algorithm' `
        '04_dsp\peak_detect\README_MINIMAL_EXAMPLE.c' @(
            '04_dsp\peak_detect\signal_peak_detect.c',
            '04_dsp\peak_detect\signal_peak_detect.h', $algorithmStatus) $blankProfile),
    (New-CopyTarget 'fft_interpolation' 'algorithm' `
        '05_precision\fft_parabolic_interpolation\README_MINIMAL_EXAMPLE.c' @(
            '05_precision\fft_parabolic_interpolation\signal_fft_parabolic_interpolation.c',
            '05_precision\fft_parabolic_interpolation\signal_fft_parabolic_interpolation.h',
            $algorithmStatus) $blankProfile),
    (New-CopyTarget 'harmonic' 'algorithm' `
        '04_dsp\harmonic\README_MINIMAL_EXAMPLE.c' @(
            '04_dsp\harmonic\signal_harmonic.c',
            '04_dsp\harmonic\signal_harmonic.h',
            '05_precision\multi_bin_energy\signal_multi_bin_energy.c',
            '05_precision\multi_bin_energy\signal_multi_bin_energy.h',
            $algorithmStatus) $blankProfile),
    (New-CopyTarget 'thd' 'algorithm' `
        '04_dsp\thd\README_MINIMAL_EXAMPLE.c' @(
            '04_dsp\thd\signal_thd.c',
            '04_dsp\thd\signal_thd.h',
            '04_dsp\harmonic\signal_harmonic.h', $algorithmStatus) $blankProfile),
    (New-CopyTarget 'phase' 'algorithm' `
        '03_measurement\phase\README_MINIMAL_EXAMPLE.c' (@(
            '03_measurement\phase\signal_phase.c',
            '03_measurement\phase\signal_phase.h',
            '03_measurement\common\signal_complex.h', $algorithmStatus) + `
            $algorithmMath) $blankProfile)
)

foreach ($required in @($sysconfig, $compiler, $product, $sdkSource,
    $cmsis, $startup, $blankProfile)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required build input not found: $required"
    }
}

New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null
$buildRootFull = [IO.Path]::GetFullPath($buildRoot).TrimEnd('\') + '\'

function Invoke-TICompile([string]$WorkingDir, [string]$Source,
    [string]$Object, [string[]]$Flags) {
    Push-Location $WorkingDir
    try {
        $saved = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        $output = & $compiler -c @Flags -o $Object $Source 2>&1
        $exitCode = $LASTEXITCODE
        $ErrorActionPreference = $saved
        if ($output) { $output | Out-File -LiteralPath 'compile.log' -Append -Encoding utf8 }
        if ($exitCode -ne 0) {
            if ($output) { $output | Out-Host }
            throw "Compile failed: $Source"
        }
    }
    finally {
        $ErrorActionPreference = $saved
        Pop-Location
    }
}

function Get-MapUsage([string]$MapPath) {
    $lines = Get-Content -LiteralPath $MapPath
    $flash = ($lines | Where-Object { $_ -match '^\s*FLASH\s+' } |
        Select-Object -First 1).Trim() -split '\s+'
    $sram = ($lines | Where-Object { $_ -match '^\s*SRAM\s+' } |
        Select-Object -First 1).Trim() -split '\s+'
    [pscustomobject]@{
        FlashBytes = [Convert]::ToInt32($flash[3], 16)
        SramBytes = [Convert]::ToInt32($sram[3], 16)
    }
}

$results = @()
foreach ($target in $targets) {
    $stage = 'PREPARE'
    $result = [ordered]@{
        target = $target.Name
        readme_manifest = 'NOT_RUN'
        copy_scope = 'NOT_RUN'
        sysconfig = 'NOT_RUN'
        compile = 'NOT_RUN'
        link = 'NOT_RUN'
        flash_bytes = $null
        sram_bytes_including_stack = $null
        board = 'NOT_RUN'
        readiness = 'NOT_READY'
        copied_files = @()
        error = ''
    }
    try {
        Write-Host "[$($target.Name)] Copy into isolated project"
        $targetDir = Join-Path $buildRoot $target.Name
        $targetFull = [IO.Path]::GetFullPath($targetDir)
        if (-not ($targetFull + '\').StartsWith($buildRootFull,
            [StringComparison]::OrdinalIgnoreCase)) {
            throw "Unsafe test directory: $targetFull"
        }
        if (Test-Path -LiteralPath $targetDir) {
            Remove-Item -LiteralPath $targetDir -Recurse -Force
        }
        $projectDir = Join-Path $targetDir 'copied_project'
        $moduleDir = Join-Path $projectDir 'modules'
        $generatedDir = Join-Path $targetDir 'generated'
        New-Item -ItemType Directory -Force -Path $moduleDir, $generatedDir |
            Out-Null

        $sourceRoot = if ($target.Root -eq 'contest') {
            $RepoRoot
        } else {
            $AlgorithmRoot
        }
        $mainSource = Join-Path $sourceRoot $target.Main
        $readmePath = Join-Path (Split-Path -Parent $mainSource) 'README.md'
        $readmeText = Get-Content -LiteralPath $readmePath -Encoding utf8 -Raw
        foreach ($relative in $target.Files) {
            $requiredLeaf = [IO.Path]::GetFileName($relative)
            if (-not $readmeText.Contains($requiredLeaf)) {
                throw "README copy list omits: $requiredLeaf"
            }
        }
        if (-not $readmeText.Contains('README_MINIMAL_EXAMPLE.c')) {
            throw 'README does not point to README_MINIMAL_EXAMPLE.c'
        }
        $result.readme_manifest = 'PASS'
        Copy-Item -LiteralPath $mainSource -Destination (Join-Path $projectDir 'main.c')
        $result.copied_files += [IO.Path]::GetFileName($mainSource)

        $seenNames = @{}
        foreach ($relative in $target.Files) {
            $sourcePath = Join-Path $sourceRoot $relative
            if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
                throw "Missing formal module file: $sourcePath"
            }
            $leaf = [IO.Path]::GetFileName($sourcePath)
            if ($seenNames.ContainsKey($leaf)) {
                throw "Duplicate copied file name: $leaf"
            }
            $seenNames[$leaf] = $true
            Copy-Item -LiteralPath $sourcePath -Destination (Join-Path $moduleDir $leaf)
            $result.copied_files += $leaf
        }
        Copy-Item -LiteralPath $target.Profile -Destination `
            (Join-Path $projectDir 'contest.syscfg')

        $copiedC = @(Get-ChildItem -LiteralPath $moduleDir -Filter '*.c' |
            Select-Object -ExpandProperty FullName)
        $projectFull = [IO.Path]::GetFullPath($projectDir).TrimEnd('\') + '\'
        foreach ($source in $copiedC + @((Join-Path $projectDir 'main.c'))) {
            if (-not ([IO.Path]::GetFullPath($source).StartsWith($projectFull,
                [StringComparison]::OrdinalIgnoreCase))) {
                throw "Non-copied source entered compile set: $source"
            }
        }
        $result.copy_scope = 'PASS'

        $stage = 'SYSCONFIG'
        Write-Host "[$($target.Name)] SysConfig"
        $saved = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        $sysOutput = & $sysconfig -s $product --script `
            (Join-Path $projectDir 'contest.syscfg') -o $generatedDir `
            --compiler ticlang 2>&1
        $sysExit = $LASTEXITCODE
        $ErrorActionPreference = $saved
        $sysOutput | Set-Content -LiteralPath `
            (Join-Path $targetDir 'sysconfig.log') -Encoding utf8
        if ($sysExit -ne 0) {
            $sysOutput | Out-Host
            throw 'SysConfig generation failed'
        }
        $result.sysconfig = 'PASS'

        $stage = 'COMPILE'
        Write-Host "[$($target.Name)] Compile copied sources"
        $compileSources = @((Join-Path $projectDir 'main.c')) + $copiedC + @(
            (Join-Path $generatedDir 'ti_msp_dl_config.c'), $startup)
        $includeDirs = @($projectDir, $moduleDir, $generatedDir, $cmsis, $sdkSource)
        $flags = @('@device.opt', '-march=thumbv6m', '-mcpu=cortex-m0plus',
            '-mfloat-abi=soft', '-mlittle-endian', '-mthumb', '-std=c11', '-O2',
            '-g', '-Wall', '-Werror', '-ffunction-sections', '-fdata-sections')
        foreach ($include in $includeDirs) { $flags += "-I$include" }

        $objects = @()
        for ($index = 0; $index -lt $compileSources.Count; ++$index) {
            $object = 'obj_{0:D2}.o' -f $index
            Invoke-TICompile $generatedDir $compileSources[$index] $object $flags
            $objects += $object
        }
        $result.compile = 'PASS'

        $stage = 'LINK'
        Write-Host "[$($target.Name)] Full link"
        $link = @('@device.opt', '-march=thumbv6m', '-mcpu=cortex-m0plus',
            '-mfloat-abi=soft', '-mlittle-endian', '-mthumb', '-O2', '-g',
            '-Wall', '-Werror', "-Wl,-m$($target.Name).map",
            "-Wl,-i$sdkSource", "-Wl,-i$generatedDir", "-Wl,-i$compilerLib",
            '-Wl,--diag_wrap=off', '-Wl,--display_error_number',
            '-Wl,--warn_sections', '-Wl,--rom_model', '-o', "$($target.Name).out")
        $link += $objects
        $link += @('-Wl,-l./device_linker.cmd', '-Wl,-ldevice.cmd.genlibs',
            '-Wl,-llibc.a')
        Push-Location $generatedDir
        try {
            $saved = $ErrorActionPreference
            $ErrorActionPreference = 'Continue'
            $linkOutput = & $compiler @link 2>&1
            $linkExit = $LASTEXITCODE
            $ErrorActionPreference = $saved
            $linkOutput | Set-Content -LiteralPath 'link.log' -Encoding utf8
            if ($linkExit -ne 0) {
                $linkOutput | Out-Host
                throw 'Full application link failed'
            }
        }
        finally {
            $ErrorActionPreference = $saved
            Pop-Location
        }
        $usage = Get-MapUsage (Join-Path $generatedDir "$($target.Name).map")
        $result.link = 'PASS'
        $result.flash_bytes = $usage.FlashBytes
        $result.sram_bytes_including_stack = $usage.SramBytes
        $result.readiness = 'COPY_READY'
        Write-Host "[$($target.Name)] COPY TEST PASS"
    }
    catch {
        $result.error = "$stage`: $($_.Exception.Message)"
        Write-Warning "[$($target.Name)] $($result.error)"
    }
    $results += [pscustomobject]$result
}

$jsonPath = Join-Path $buildRoot 'copy_assembly_results.json'
$csvPath = Join-Path $buildRoot 'copy_assembly_results.csv'
$results | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $jsonPath -Encoding utf8
$results | Select-Object target, readme_manifest, copy_scope, sysconfig, compile, link,
    flash_bytes, sram_bytes_including_stack, board, readiness, error |
    Export-Csv -LiteralPath $csvPath -NoTypeInformation -Encoding utf8
$results | Select-Object target, readme_manifest, sysconfig, compile, link, flash_bytes,
    sram_bytes_including_stack, readiness, error | Format-Table -AutoSize

$passed = @($results | Where-Object readiness -eq 'COPY_READY').Count
Write-Output "COPY TEST PASS: $passed/$($results.Count)"
if ($passed -ne $results.Count) { exit 1 }
