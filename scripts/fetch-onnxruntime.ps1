# Downloads the official prebuilt ONNX Runtime for Windows x64 into _deps\
# and prints the directory to pass to CMake as ONNXRUNTIME_ROOT_DIR.
#   powershell -ExecutionPolicy Bypass -File scripts\fetch-onnxruntime.ps1 [-Version 1.30.1]
#
# Why not the vcpkg port? The port compiles ORT from source (protobuf, abseil,
# onnx ...) which takes hours; the official zip links identically and is what
# the GitHub Actions workflow uses.
param(
    [string]$Version = "1.30.1"
)

$ErrorActionPreference = "Stop"
$ProgressPreference = 'SilentlyContinue'   # Invoke-WebRequest is ~10x faster without the progress bar

# SHA-256 of onnxruntime-win-x64-<Version>.zip as published on the release page.
$knownSha = @{
    "1.30.1" = "9e70d829a7c2014c6f6d2889cd3578733c2e5bbe919e186b5f2038d59176ab13"
}

$depsDir = Join-Path (Split-Path -Parent $PSScriptRoot) "_deps"
$zip     = Join-Path $depsDir "onnxruntime-win-x64-$Version.zip"
$root    = Join-Path $depsDir "onnxruntime-win-x64-$Version"

if (Test-Path (Join-Path $root "include\onnxruntime_cxx_api.h")) {
    Write-Host "ONNX Runtime $Version already present -> $root" -ForegroundColor Green
    Write-Host $root
    exit 0
}
New-Item -ItemType Directory -Force -Path $depsDir | Out-Null

$url = "https://github.com/microsoft/onnxruntime/releases/download/v$Version/onnxruntime-win-x64-$Version.zip"
Write-Host "Downloading $url"
Invoke-WebRequest -Uri $url -OutFile $zip

if ($knownSha.ContainsKey($Version)) {
    $sha = (Get-FileHash $zip -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($sha -ne $knownSha[$Version]) { Write-Error "SHA-256 mismatch: $sha (expected $($knownSha[$Version]))" }
    Write-Host "SHA-256 OK"
}
else {
    Write-Warning "No pinned SHA-256 for $Version -- verify the hash manually."
}

Expand-Archive -Path $zip -DestinationPath $depsDir -Force
Remove-Item -Force $zip
if (-not (Test-Path (Join-Path $root "include\onnxruntime_cxx_api.h"))) {
    Write-Error "Unexpected archive layout: $root\include\onnxruntime_cxx_api.h not found"
}
Write-Host "ONNX Runtime $Version ready -> $root" -ForegroundColor Green
Write-Host "Configure with:  cmake -B build -DFLOWSCALE_WITH_NEURAL=ON -DONNXRUNTIME_ROOT_DIR=`"$root`" ..."
Write-Host $root
