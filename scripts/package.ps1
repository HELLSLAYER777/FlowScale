# Collects a distributable FlowScale folder and zips it.
#   powershell -ExecutionPolicy Bypass -File scripts\package.ps1 `
#       -BuildDir build\Release -Name FlowScale-win64 [-OutDir dist]
# Expects CMake to have already copied shaders\ (+ models\ for the neural build)
# next to FlowScale.exe in -BuildDir.
param(
    [Parameter(Mandatory = $true)][string]$BuildDir,
    [Parameter(Mandatory = $true)][string]$Name,
    [string]$OutDir = "dist"
)

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
$exe  = Join-Path $BuildDir "FlowScale.exe"
if (-not (Test-Path $exe)) { Write-Error "FlowScale.exe not found in '$BuildDir' -- build first." }

$stage = Join-Path $env:TEMP "$Name-stage"
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Path $stage | Out-Null

Copy-Item $exe $stage\
if (Test-Path (Join-Path $BuildDir "shaders")) { Copy-Item -Recurse (Join-Path $BuildDir "shaders") $stage\shaders }
if (Test-Path (Join-Path $BuildDir "models"))  { Copy-Item -Recurse (Join-Path $BuildDir "models")  $stage\models  }

# Any runtime DLLs the build dropped next to the exe (onnxruntime.dll, ...) or
# that vcpkg deployed to vcpkg_installed\<triplet>\bin.
Get-ChildItem $BuildDir -Filter *.dll -ErrorAction SilentlyContinue | ForEach-Object { Copy-Item $_.FullName $stage\ }
$vcpkgBin = Join-Path $repo "build\vcpkg_installed\x64-windows\bin"
if (Test-Path $vcpkgBin) {
    Get-ChildItem $vcpkgBin -Filter *.dll | ForEach-Object { Copy-Item $_.FullName $stage\ -ErrorAction SilentlyContinue }
}

# Papers and docs users need.
foreach ($f in "LICENSE", "THIRD_PARTY_NOTICES.md", "README.md", "docs\BUILDING.md") {
    $src = Join-Path $repo $f
    if (Test-Path $src) {
        $dst = Join-Path $stage (Split-Path -Leaf $f)
        Copy-Item $src $dst
    }
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$zip = Join-Path $OutDir "$Name.zip"
if (Test-Path $zip) { Remove-Item -Force $zip }
Compress-Archive -Path "$stage\*" -DestinationPath $zip -Force
Remove-Item -Recurse -Force $stage
Write-Host "Packaged -> $zip" -ForegroundColor Green
