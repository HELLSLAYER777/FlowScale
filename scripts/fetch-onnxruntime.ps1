# Downloads the official prebuilt ONNX Runtime for Windows x64 into _deps\
# and prints the directory to pass to CMake as ONNXRUNTIME_ROOT_DIR.
#   powershell -ExecutionPolicy Bypass -File scripts\fetch-onnxruntime.ps1 [-Version 1.30.0]
#
# Why not the vcpkg port? The port compiles ORT from source (protobuf, abseil,
# onnx ...) which takes hours; CI uses Microsoft's prebuilt NuGet package.
param(
    [ValidatePattern('^\d+\.\d+\.\d+(?:-[a-zA-Z0-9.-]+)?$')]
    [string]$Version = "1.30.0"
)

$ErrorActionPreference = "Stop"
$ProgressPreference = 'SilentlyContinue'   # Invoke-WebRequest is ~10x faster without the progress bar

$depsDir = Join-Path (Split-Path -Parent $PSScriptRoot) "_deps"
$root    = Join-Path $depsDir "onnxruntime-win-x64-$Version"
$required = @(
    "include/onnxruntime_cxx_api.h",
    "include/onnxruntime_c_api.h",
    "lib/onnxruntime.lib",
    "lib/onnxruntime.dll",
    "lib/onnxruntime_providers_shared.dll"
)

$missing = $required | Where-Object { -not (Test-Path -LiteralPath (Join-Path $root $_) -PathType Leaf) }
if (-not $missing) {
    Write-Host "ONNX Runtime $Version already present -> $root" -ForegroundColor Green
    Write-Host $root
    exit 0
}
$Version = $Version.ToLowerInvariant()
$packageId = "microsoft.ml.onnxruntime"
$packageName = "$packageId.$Version.nupkg"
$url = "https://api.nuget.org/v3-flatcontainer/$packageId/$Version/$packageName"
$expectedSha = Get-Content -LiteralPath (Join-Path $PSScriptRoot "onnxruntime.sha256") |
    ForEach-Object {
        $sha, $name = $_ -split '\s+', 2
        if ($name -eq $packageName) { $sha }
    }
$staging = Join-Path $depsDir ([guid]::NewGuid().ToString())
$package = Join-Path $staging $packageName
New-Item -ItemType Directory -Path (Join-Path $staging "include"), (Join-Path $staging "lib") -Force | Out-Null
try {
    Write-Host "Downloading $url"
    Invoke-WebRequest -UseBasicParsing -Uri $url -OutFile $package
    if ($expectedSha) {
        $sha = (Get-FileHash -LiteralPath $package -Algorithm SHA256).Hash
        if ($sha -ne $expectedSha) { throw "SHA-256 mismatch: $sha (expected $expectedSha)" }
        Write-Host "SHA-256 OK"
    } else {
        Write-Warning "No pinned SHA-256 for $Version -- verify the hash manually."
    }

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [IO.Compression.ZipFile]::OpenRead($package)
    try {
        foreach ($entry in $archive.Entries) {
            if ($entry.FullName -match '^build/native/include/([\w.-]+)$') {
                $dest = Join-Path $staging "include/$($Matches[1])"
            } elseif ($entry.FullName -match '^runtimes/win-x64/native/(onnxruntime\.lib|onnxruntime(?:_providers_shared)?\.dll)$') {
                $dest = Join-Path $staging "lib/$($Matches[1])"
            } else {
                continue
            }
            [IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $dest)
        }
    } finally {
        $archive.Dispose()
    }
    foreach ($file in $required) {
        if (-not (Test-Path -LiteralPath (Join-Path $staging $file) -PathType Leaf)) {
            throw "Unexpected NuGet package layout: missing $file"
        }
    }
    New-Item -ItemType Directory -Path $root -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $staging "include"), (Join-Path $staging "lib") -Destination $root -Recurse -Force
} finally {
    Remove-Item -LiteralPath $staging -Recurse -Force
}
Write-Host "ONNX Runtime $Version ready -> $root" -ForegroundColor Green
Write-Host "Configure with:  cmake -B build -DFLOWSCALE_WITH_NEURAL=ON -DONNXRUNTIME_ROOT_DIR=`"$root`" ..."
Write-Host $root
