[CmdletBinding()]
param(
    [string]$Gcc = "gcc",
    [string]$TiArmClang = "D:\TI\CCS\ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\bin\tiarmclang.exe"
)

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$recipeRoot = Join-Path $repo "00_docs\recipes"
$buildRoot = Join-Path $repo "10_tests\algorithm_recipes\build"
$resultPath = Join-Path $buildRoot "direct_recipe_results.json"
$sourcePath = Join-Path $buildRoot "direct_recipes_generated.c"
$exePath = Join-Path $buildRoot "direct_recipes_pc.exe"
$tiObjectPath = Join-Path $buildRoot "direct_recipes_ticlang.obj"

New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null

$requiredHeadingPatterns = @(
    '(?m)^## 1\.',
    '(?m)^## 2\.',
    '(?m)^## 3\.',
    '(?m)^## 4\.',
    '(?m)^## 5\.',
    '(?m)^## 6\.',
    '(?m)^## 7\.',
    '(?m)^## 8\.',
    '(?m)^## 9\.',
    '(?m)^### .+'
)

$snippets = New-Object System.Collections.Generic.List[string]
$recipeFiles = @(Get-ChildItem -LiteralPath $recipeRoot -File -Filter "*.md" | Sort-Object Name)
$cmsisRecipeCount = 0
$cmsisRecipeNames = @(
    "ac_rms.md", "cmsis_fft_spectrum.md", "mean.md", "minmax.md",
    "normalize.md", "offset_correction.md", "remove_dc.md", "rms.md",
    "scaling.md", "vpp.md"
)
if ($recipeFiles.Count -eq 0) {
    throw "No recipe Markdown files found."
}

foreach ($file in $recipeFiles) {
    $text = Get-Content -Raw -Encoding UTF8 -LiteralPath $file.FullName
    if ($file.Name -in $cmsisRecipeNames) {
        ++$cmsisRecipeCount
        continue
    }
    foreach ($pattern in $requiredHeadingPatterns) {
        if ($text -notmatch $pattern) {
            throw "$($file.Name): missing required heading pattern '$pattern'"
        }
    }
    $match = [regex]::Match(
        $text,
        '(?s)<!-- DIRECT_COPY_BEGIN -->\s*```c\s*(.*?)\s*```\s*<!-- DIRECT_COPY_END -->')
    if (-not $match.Success) {
        throw "$($file.Name): DIRECT_COPY block missing or malformed"
    }
    $snippets.Add("/* From $($file.Name) */`r`n$($match.Groups[1].Value)")
}

$testMain = @'

#include <math.h>
#include <stdio.h>

static int failures = 0;

static void check_near(const char *name, float actual, float expected, float tolerance)
{
    if (fabsf(actual - expected) > tolerance) {
        printf("FAIL %s actual=%f expected=%f\n", name, actual, expected);
        ++failures;
    }
}

int main(void)
{
    const float vpp_x[] = {0.5f, 2.5f, 1.0f, 2.0f};
    const uint16_t raw[] = {0U, 4095U};
    const float clip_x[] = {0.0f, 1.0f, 3.3f};
    const float peak_x[] = {0.0f, 1.0f, 5.0f, 3.0f};
    const float crossings[] = {0.0f, 10.0f, 20.0f, 30.0f};
    float converted[2];
    float peak_value;
    uint32_t index = 99U;

    recipe_adc_to_voltage(raw, converted, 2U, 3.3f, 4095U, 1.0f, 0.0f);
    check_near("adc_0", converted[0], 0.0f, 1.0e-6f);
    check_near("adc_full", converted[1], 3.3f, 1.0e-6f);
    if (!recipe_find_first_above(vpp_x, 4U, 2.0f, &index) || index != 1U) ++failures;
    if (recipe_count_clipped(clip_x, 3U, 0.01f, 3.29f) != 2U) ++failures;
    if (recipe_peak_index(peak_x, 1U, 3U, &peak_value) != 2U) ++failures;
    check_near("peak_value", peak_value, 5.0f, 1.0e-6f);
    check_near("multicycle", recipe_multicycle_frequency(crossings, 4U, 1000.0f),
               100.0f, 1.0e-6f);

    if (failures != 0) return 1;
    printf("NON-CMSIS DIRECT RECIPE PC TEST PASS\n");
    return 0;
}
'@

$source = ($snippets -join "`r`n`r`n") + $testMain
Set-Content -Encoding UTF8 -LiteralPath $sourcePath -Value $source

$gccOutput = & $Gcc -std=c11 -O2 -Wall -Wextra -Werror -pedantic `
    $sourcePath -lm -o $exePath 2>&1 | Out-String
$gccCompile = if ($LASTEXITCODE -eq 0) { "PASS" } else { "FAIL" }
$pcRun = "NOT_RUN"
$runOutput = ""
if ($gccCompile -eq "PASS") {
    $runOutput = & $exePath 2>&1 | Out-String
    $pcRun = if ($LASTEXITCODE -eq 0) { "PASS" } else { "FAIL" }
}

$tiCompile = "NOT_RUN"
$tiOutput = ""
if (Test-Path -LiteralPath $TiArmClang) {
    $tiOutput = & $TiArmClang -c -std=c11 -O2 -Wall -Wextra -Werror `
        -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft `
        -mlittle-endian -mthumb $sourcePath -o $tiObjectPath 2>&1 | Out-String
    $tiCompile = if ($LASTEXITCODE -eq 0) { "PASS" } else { "FAIL" }
}

$result = [ordered]@{
    date = (Get-Date -Format "yyyy-MM-dd")
    recipe_count = $recipeFiles.Count
    non_cmsis_direct_recipe_count = $snippets.Count
    cmsis_recipe_count = $cmsisRecipeCount
    structure_check = "PASS"
    pc_gcc_compile = $gccCompile
    pc_truth_test = $pcRun
    ti_arm_clang_compile = $tiCompile
    board = "NOT_RUN"
    gcc_output = $gccOutput.Trim()
    pc_output = $runOutput.Trim()
    ti_output = $tiOutput.Trim()
}
$result | ConvertTo-Json -Depth 4 | Set-Content -Encoding UTF8 -LiteralPath $resultPath
$result | Format-List

if (($gccCompile -ne "PASS") -or ($pcRun -ne "PASS") -or
    (($tiCompile -ne "PASS") -and ($tiCompile -ne "NOT_RUN"))) {
    exit 1
}
