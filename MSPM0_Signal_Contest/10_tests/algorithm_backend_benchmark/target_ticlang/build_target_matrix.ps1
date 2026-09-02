param(
    [string]$SdkRoot = 'C:\ti\mspm0_sdk_2_11_00_07',
    [string]$SysConfigRoot = 'C:\ti\sysconfig_1.26.2',
    [string]$TiClangRoot = 'C:\ti\ti_cgt_arm_llvm_4.0.2.LTS'
)

$ErrorActionPreference = 'Stop'
$scriptRoot = (Resolve-Path -LiteralPath $PSScriptRoot).Path
$algorithmRoot = (Resolve-Path -LiteralPath (Join-Path $scriptRoot '..\..\..')).Path
$buildRoot = Join-Path $scriptRoot 'build'
$expectedBuildRoot = [System.IO.Path]::GetFullPath((Join-Path $scriptRoot 'build'))
if ([System.IO.Path]::GetFullPath($buildRoot) -ne $expectedBuildRoot) {
    throw 'Unsafe build path.'
}
if (Test-Path -LiteralPath $buildRoot) {
    Remove-Item -LiteralPath $buildRoot -Recurse -Force
}
New-Item -ItemType Directory -Path $buildRoot | Out-Null

$tiClang = Join-Path $TiClangRoot 'bin\tiarmclang.exe'
$sysConfig = Join-Path $SysConfigRoot 'sysconfig_cli.bat'
$product = Join-Path $SdkRoot '.metadata\product.json'
$deviceLinker = Join-Path $SdkRoot 'examples\nortos\LP_MSPM0G3507\cmsis_dsp\cmsis_dsp_fft_q15\ticlang\device_linker.cmd'
foreach ($required in @($tiClang, $sysConfig, $product, $deviceLinker)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required tool or SDK file not found: $required"
    }
}

function Invoke-CheckedTool {
    param(
        [string]$Tool,
        [string[]]$ArgumentList,
        [string]$LogPath,
        [switch]$AllowFailure
    )
    $savedErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    $output = & $Tool @ArgumentList 2>&1
    $exitCode = $LASTEXITCODE
    $ErrorActionPreference = $savedErrorActionPreference
    @(
        'ARGUMENT_COUNT=' + $ArgumentList.Count
        ($ArgumentList -join "`n")
        'TOOL_OUTPUT:'
        $output
    ) | Set-Content -LiteralPath $LogPath -Encoding UTF8
    if (($exitCode -ne 0) -and (-not $AllowFailure)) {
        throw "Tool failed ($exitCode). See $LogPath"
    }
    return $exitCode
}

function New-SysConfigOutput {
    param([string]$Name, [string]$SysCfgFile)
    $outputDir = Join-Path $buildRoot $Name
    New-Item -ItemType Directory -Path $outputDir | Out-Null
    $arguments = @(
        '--compiler', 'ticlang',
        '--product', $product,
        '--output', $outputDir,
        $SysCfgFile
    )
    Invoke-CheckedTool -Tool $sysConfig -ArgumentList $arguments -LogPath (Join-Path $outputDir 'sysconfig.log') | Out-Null
    return $outputDir
}

function Get-CommonCompileArguments {
    param([string]$SysCfgOutput)
    return @(
        ('-I' + (Join-Path $algorithmRoot '03_measurement\common'))
        ('-I' + (Join-Path $algorithmRoot '03_measurement\rms'))
        ('-I' + (Join-Path $algorithmRoot '03_measurement\phase'))
        ('-I' + (Join-Path $algorithmRoot '04_dsp\fft'))
        ('-I' + $SysCfgOutput)
        ('-I' + (Join-Path $SdkRoot 'source'))
        ('-I' + (Join-Path $SdkRoot 'source\third_party\CMSIS\Core\Include'))
        ('-I' + (Join-Path $SdkRoot 'source\third_party\CMSIS\DSP\Include'))
        ('@' + (Join-Path $SysCfgOutput 'device.opt'))
        '-DARM_MATH_CM0'
        '-O2'
        '-mcpu=cortex-m0plus'
        '-march=thumbv6m'
        '-mfloat-abi=soft'
        '-mthumb'
        '-fno-strict-aliasing'
        '-Wall'
    )
}

function Link-Target {
    param(
        [string]$OutputDir,
        [string[]]$Objects,
        [string]$Name
    )
    $outFile = Join-Path $OutputDir ($Name + '.out')
    $mapFile = Join-Path $OutputDir ($Name + '.map')
    $arguments = @('-Wl,-u,_c_int00') + $Objects + @(
        '-ldevice.cmd.genlibs'
        ('-L' + $OutputDir)
        ('-L' + (Join-Path $SdkRoot 'source'))
        $deviceLinker
        ('-Wl,-m,' + $mapFile)
        '-Wl,--rom_model'
        '-Wl,--warn_sections'
        ('-L' + (Join-Path $TiClangRoot 'lib'))
        '-llibc.a'
        '-o'
        $outFile
    )
    return Invoke-CheckedTool -Tool $tiClang -ArgumentList $arguments -LogPath (Join-Path $OutputDir 'link.log') -AllowFailure
}

$matrixRows = [System.Collections.Generic.List[string]]::new()
$matrixRows.Add('backend,count,status,out,map')
$cmsisSyscfg = New-SysConfigOutput -Name 'syscfg_cmsis' -SysCfgFile (Join-Path $scriptRoot 'cmsis_backend_smoke.syscfg')
$backendNames = @('REFERENCE_C', 'CMSIS_Q15', 'CMSIS_Q31', 'CMSIS_F32')
$counts = @(512, 1024, 2048, 4096)

for ($backend = 0; $backend -lt $backendNames.Count; ++$backend) {
    foreach ($count in $counts) {
        $name = $backendNames[$backend] + '_' + $count
        $outputDir = Join-Path $buildRoot $name
        New-Item -ItemType Directory -Path $outputDir | Out-Null
        Copy-Item -Path (Join-Path $cmsisSyscfg '*') -Destination $outputDir -Recurse -Force
        $compileArguments = Get-CommonCompileArguments -SysCfgOutput $outputDir
        $compileArguments += @(
            ('-DSIGNAL_FFT_BACKEND=' + $backend)
            ('-DSIGNAL_TARGET_FFT_COUNT=' + $count)
        )
        $objects = [System.Collections.Generic.List[string]]::new()
        $sourcePairs = @(
            @((Join-Path $scriptRoot 'target_fft_smoke.c'), 'target_fft_smoke.obj'),
            @((Join-Path $algorithmRoot '04_dsp\fft\signal_fft.c'), 'signal_fft.obj')
        )
        foreach ($generatedC in Get-ChildItem -LiteralPath $outputDir -Filter '*.c') {
            $sourcePairs += ,@($generatedC.FullName, ([System.IO.Path]::GetFileNameWithoutExtension($generatedC.Name) + '.obj'))
        }
        $compileOk = $true
        foreach ($pair in $sourcePairs) {
            $objectPath = Join-Path $outputDir $pair[1]
            $logPath = Join-Path $outputDir ($pair[1] + '.log')
            $arguments = $compileArguments + @('-c', $pair[0], '-o', $objectPath)
            $code = Invoke-CheckedTool -Tool $tiClang -ArgumentList $arguments -LogPath $logPath -AllowFailure
            if ($code -ne 0) {
                $compileOk = $false
                break
            }
            $objects.Add($objectPath)
        }
        if ($compileOk) {
            $linkCode = Link-Target -OutputDir $outputDir -Objects $objects.ToArray() -Name $name
            $status = if ($linkCode -eq 0) { 'BUILD_LINK_VERIFIED' } else { 'LINK_FAILED' }
        } else {
            $status = 'COMPILE_FAILED'
        }
        $outPath = Join-Path $outputDir ($name + '.out')
        $mapPath = Join-Path $outputDir ($name + '.map')
        $matrixRows.Add("$($backendNames[$backend]),$count,$status,$outPath,$mapPath")
    }
}

foreach ($iqProfile in @('iqmath_rts', 'iqmath_mathacl')) {
    $syscfgFile = Join-Path $scriptRoot ($iqProfile + '_smoke.syscfg')
    $outputDir = New-SysConfigOutput -Name $iqProfile -SysCfgFile $syscfgFile
    $compileArguments = Get-CommonCompileArguments -SysCfgOutput $outputDir
    $mathBackend = if ($iqProfile -eq 'iqmath_rts') { '1' } else { '2' }
    $compileArguments += @('-DSIGNAL_MATH_BACKEND=' + $mathBackend)
    $objects = [System.Collections.Generic.List[string]]::new()
    $sourcePairs = @(
        @((Join-Path $scriptRoot 'target_iqmath_smoke.c'), 'target_iqmath_smoke.obj'),
        @((Join-Path $algorithmRoot '03_measurement\rms\signal_rms.c'), 'signal_rms.obj'),
        @((Join-Path $algorithmRoot '03_measurement\phase\signal_phase.c'), 'signal_phase.obj')
    )
    foreach ($generatedC in Get-ChildItem -LiteralPath $outputDir -Filter '*.c') {
        $sourcePairs += ,@($generatedC.FullName, ([System.IO.Path]::GetFileNameWithoutExtension($generatedC.Name) + '.obj'))
    }
    $compileOk = $true
    foreach ($pair in $sourcePairs) {
        $objectPath = Join-Path $outputDir $pair[1]
        $code = Invoke-CheckedTool -Tool $tiClang -ArgumentList ($compileArguments + @('-c', $pair[0], '-o', $objectPath)) -LogPath (Join-Path $outputDir ($pair[1] + '.log')) -AllowFailure
        if ($code -ne 0) {
            $compileOk = $false
            break
        }
        $objects.Add($objectPath)
    }
    if ($compileOk) {
        $linkCode = Link-Target -OutputDir $outputDir -Objects $objects.ToArray() -Name $iqProfile
        $status = if ($linkCode -eq 0) { 'BUILD_LINK_VERIFIED' } else { 'LINK_FAILED' }
    } else {
        $status = 'COMPILE_FAILED'
    }
    $matrixRows.Add("$iqProfile,NA,$status,$outputDir,$outputDir")
}

$matrixPath = Join-Path $buildRoot 'target_build_matrix.csv'
$matrixRows | Set-Content -LiteralPath $matrixPath -Encoding UTF8
Get-Content -LiteralPath $matrixPath
