param(
    [string]$SdkRoot = 'C:\ti\mspm0_sdk_2_11_00_07',
    [string]$TiClangRoot = 'C:\ti\ti_cgt_arm_llvm_4.0.2.LTS'
)

$ErrorActionPreference = 'Stop'
$scriptRoot = (Resolve-Path -LiteralPath $PSScriptRoot).Path
$algorithmRoot = (Resolve-Path -LiteralPath (Join-Path $scriptRoot '..\..\..')).Path
$buildRoot = [System.IO.Path]::GetFullPath((Join-Path $scriptRoot 'build_offline'))
$expectedBuildRoot = [System.IO.Path]::GetFullPath((Join-Path $scriptRoot 'build_offline'))
if ($buildRoot -ne $expectedBuildRoot) {
    throw 'Unsafe build path.'
}
if (Test-Path -LiteralPath $buildRoot) {
    Remove-Item -LiteralPath $buildRoot -Recurse -Force
}
New-Item -ItemType Directory -Path $buildRoot | Out-Null

$tiClang = Join-Path $TiClangRoot 'bin\tiarmclang.exe'
$startupSource = Join-Path $SdkRoot 'source\ti\devices\msp\m0p\startup_system_files\ticlang\startup_mspm0g350x_ticlang.c'
$deviceLinker = Join-Path $SdkRoot 'examples\nortos\LP_MSPM0G3507\cmsis_dsp\cmsis_dsp_fft_q15\ticlang\device_linker.cmd'
$cmsisLibrary = Join-Path $SdkRoot 'source\third_party\CMSIS\DSP\Lib\ticlang\m0p\arm_cortexM0l_math.a'
$driverLibrary = Join-Path $SdkRoot 'source\ti\driverlib\lib\ticlang\m0p\mspm0g1x0x_g3x0x\driverlib.a'
$iqRtsLibrary = Join-Path $SdkRoot 'source\ti\iqmath\lib\ticlang\m0p\rts\mspm0g1x0x_g3x0x\iqmath.a'
$iqMathAclLibrary = Join-Path $SdkRoot 'source\ti\iqmath\lib\ticlang\m0p\mathacl\mspm0g1x0x_g3x0x\iqmath.a'
foreach ($required in @($tiClang, $startupSource, $deviceLinker, $cmsisLibrary,
                         $driverLibrary, $iqRtsLibrary, $iqMathAclLibrary)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required tool or SDK file not found: $required"
    }
}

function Invoke-ToolToLog {
    param(
        [string]$Tool,
        [string[]]$ArgumentList,
        [string]$LogPath
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
    return $exitCode
}

function Get-CompileArguments {
    return @(
        ('-I' + (Join-Path $algorithmRoot '03_measurement\common'))
        ('-I' + (Join-Path $algorithmRoot '03_measurement\rms'))
        ('-I' + (Join-Path $algorithmRoot '03_measurement\phase'))
        ('-I' + (Join-Path $algorithmRoot '04_dsp\fft'))
        ('-I' + (Join-Path $SdkRoot 'source'))
        ('-I' + (Join-Path $SdkRoot 'source\third_party\CMSIS\Core\Include'))
        ('-I' + (Join-Path $SdkRoot 'source\third_party\CMSIS\DSP\Include'))
        '-D__MSPM0G3507__'
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

function Compile-Source {
    param(
        [string]$Source,
        [string]$Object,
        [string[]]$ExtraArguments
    )
    $arguments = (Get-CompileArguments) + $ExtraArguments + @('-c', $Source, '-o', $Object)
    return Invoke-ToolToLog -Tool $tiClang -ArgumentList $arguments -LogPath ($Object + '.log')
}

function Link-Objects {
    param(
        [string]$OutputDir,
        [string]$Name,
        [string[]]$Objects,
        [string[]]$Libraries
    )
    $outFile = Join-Path $OutputDir ($Name + '.out')
    $mapFile = Join-Path $OutputDir ($Name + '.map')
    $arguments = @('-Wl,-u,_c_int00') + $Objects + $Libraries + @(
        $deviceLinker
        ('-Wl,-m,' + $mapFile)
        '-Wl,--rom_model'
        '-Wl,--warn_sections'
        ('-L' + (Join-Path $TiClangRoot 'lib'))
        '-llibc.a'
        '-o'
        $outFile
    )
    return Invoke-ToolToLog -Tool $tiClang -ArgumentList $arguments -LogPath (Join-Path $OutputDir 'link.log')
}

$rows = [System.Collections.Generic.List[string]]::new()
$rows.Add('backend,count,status,out,map')
$backendNames = @('REFERENCE_C', 'CMSIS_Q15', 'CMSIS_Q31', 'CMSIS_F32')
$counts = @(512, 1024, 2048, 4096)

for ($backend = 0; $backend -lt $backendNames.Count; ++$backend) {
    foreach ($count in $counts) {
        $name = $backendNames[$backend] + '_' + $count
        $outputDir = Join-Path $buildRoot $name
        New-Item -ItemType Directory -Path $outputDir | Out-Null
        $mainObject = Join-Path $outputDir 'target_fft_smoke.obj'
        $fftObject = Join-Path $outputDir 'signal_fft.obj'
        $startupObject = Join-Path $outputDir 'startup.obj'
        $extra = @(
            ('-DSIGNAL_FFT_BACKEND=' + $backend)
            ('-DSIGNAL_TARGET_FFT_COUNT=' + $count)
        )
        $compileCodes = @(
            (Compile-Source -Source (Join-Path $scriptRoot 'target_fft_smoke.c') -Object $mainObject -ExtraArguments $extra),
            (Compile-Source -Source (Join-Path $algorithmRoot '04_dsp\fft\signal_fft.c') -Object $fftObject -ExtraArguments $extra),
            (Compile-Source -Source $startupSource -Object $startupObject -ExtraArguments @())
        )
        if (($compileCodes | Where-Object { $_ -ne 0 }).Count -eq 0) {
            $libraries = if ($backend -eq 0) { @() } else { @($cmsisLibrary) }
            $linkCode = Link-Objects -OutputDir $outputDir -Name $name -Objects @($mainObject, $fftObject, $startupObject) -Libraries $libraries
            $status = if ($linkCode -eq 0) { 'BUILD_LINK_VERIFIED' } else { 'LINK_FAILED' }
        } else {
            $status = 'COMPILE_FAILED'
        }
        $rows.Add("$($backendNames[$backend]),$count,$status,$outputDir\$name.out,$outputDir\$name.map")
    }
}

foreach ($profile in @(
    @('MATH_REFERENCE', '', '0'),
    @('IQMATH_RTS', $iqRtsLibrary, '1'),
    @('IQMATH_MATHACL', $iqMathAclLibrary, '2')
)) {
    $name = $profile[0]
    $outputDir = Join-Path $buildRoot $name
    New-Item -ItemType Directory -Path $outputDir | Out-Null
    $mainObject = Join-Path $outputDir 'target_iqmath_smoke.obj'
    $rmsObject = Join-Path $outputDir 'signal_rms.obj'
    $phaseObject = Join-Path $outputDir 'signal_phase.obj'
    $startupObject = Join-Path $outputDir 'startup.obj'
    $mathBackendArgument = @('-DSIGNAL_MATH_BACKEND=' + $profile[2])
    $compileCodes = @(
        (Compile-Source -Source (Join-Path $scriptRoot 'target_iqmath_smoke.c') -Object $mainObject -ExtraArguments $mathBackendArgument),
        (Compile-Source -Source (Join-Path $algorithmRoot '03_measurement\rms\signal_rms.c') -Object $rmsObject -ExtraArguments $mathBackendArgument),
        (Compile-Source -Source (Join-Path $algorithmRoot '03_measurement\phase\signal_phase.c') -Object $phaseObject -ExtraArguments $mathBackendArgument),
        (Compile-Source -Source $startupSource -Object $startupObject -ExtraArguments @())
    )
    if (($compileCodes | Where-Object { $_ -ne 0 }).Count -eq 0) {
        $mathLibraries = if ($profile[1] -eq '') { @() } else { @($profile[1], $driverLibrary) }
        $linkCode = Link-Objects -OutputDir $outputDir -Name $name -Objects @($mainObject, $rmsObject, $phaseObject, $startupObject) -Libraries $mathLibraries
        $status = if ($linkCode -eq 0) { 'BUILD_LINK_VERIFIED' } else { 'LINK_FAILED' }
    } else {
        $status = 'COMPILE_FAILED'
    }
    $rows.Add("$name,NA,$status,$outputDir\$name.out,$outputDir\$name.map")
}

$matrixPath = Join-Path $buildRoot 'target_build_matrix.csv'
$rows | Set-Content -LiteralPath $matrixPath -Encoding UTF8
Get-Content -LiteralPath $matrixPath
