param(
    [string]$RepoRoot = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = "Stop"
$manifestPath = Join-Path $RepoRoot "00_docs\canonical_repository\PERIPHERAL_API_FREEZE_BASELINE.txt"
$roots = @("01_bsp", "02_acquisition", "06_generator", "07_signal_frontend")

if (-not (Test-Path -LiteralPath $manifestPath)) {
    throw "API manifest not found: $manifestPath"
}

$expected = @{}
Get-Content -LiteralPath $manifestPath | ForEach-Object {
    $line = $_.Trim()
    if (($line.Length -eq 0) -or $line.StartsWith("#")) { return }
    if ($line -notmatch "^([0-9A-Fa-f]{64})\s{2}(.+)$") {
        throw "Invalid manifest line: $_"
    }
    $expected[$Matches[2]] = $Matches[1].ToUpperInvariant()
}

$headers = @()
foreach ($root in $roots) {
    $headers += Get-ChildItem -LiteralPath (Join-Path $RepoRoot $root) -Recurse -Filter "signal_*.h"
}

$failed = $false
foreach ($header in ($headers | Sort-Object FullName)) {
    $relative = $header.FullName.Substring($RepoRoot.Length + 1).Replace("\", "/")
    if (-not $expected.ContainsKey($relative)) {
        Write-Error "Untracked public header: $relative"
        $failed = $true
        continue
    }
    $actualHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $header.FullName).Hash
    if ($actualHash -ne $expected[$relative]) {
        Write-Error "API changed without manifest update: $relative"
        $failed = $true
    }
    $expected.Remove($relative)
}

foreach ($missing in ($expected.Keys | Sort-Object)) {
    Write-Error "Manifest entry has no public header: $missing"
    $failed = $true
}

if ($failed) {
    throw "Peripheral API freeze check FAILED. Record intentional changes in 00_docs/canonical_repository/CHANGE_LOG.md before updating the baseline."
}

Write-Host "Peripheral API freeze check PASS: $($headers.Count) public headers unchanged."
