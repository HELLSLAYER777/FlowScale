# FlowScale build script (run from the repo root)
#   powershell -ExecutionPolicy Bypass -File scripts\build.ps1 [-Neural]
param(
    [switch]$Neural
)

$ErrorActionPreference = "Stop"

if (-not $env:VCPKG_ROOT) {
    Write-Error "VCPKG_ROOT is not set. See docs/BUILDING.md."
}

$preset = if ($Neural) { "windows-release-neural" } else { "windows-release" }

Write-Host "== Configure ($preset)" -ForegroundColor Cyan
cmake --preset $preset
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "== Build" -ForegroundColor Cyan
cmake --build --preset $preset --config Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "== Collect package" -ForegroundColor Cyan
powershell -ExecutionPolicy Bypass -File "$PSScriptRoot\package.ps1" -BuildDir build\Release -Name FlowScale-portable
Write-Host "Done (run dist\FlowScale-portable.zip contents)" -ForegroundColor Green
