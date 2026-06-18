param(
  [string]$Version = "latest",
  [ValidateSet("x64", "x86", "arm64")]
  [string]$Arch = "x64",
  [string]$InstallDir = (Join-Path $PSScriptRoot "..\third_party\onnxruntime"),
  [switch]$Force
)

$ErrorActionPreference = "Stop"
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

function Get-OnnxRuntimeRelease {
  param([string]$RequestedVersion)

  $headers = @{ "User-Agent" = "llvc-onnxruntime-installer" }
  if ($RequestedVersion -eq "latest") {
    return Invoke-RestMethod `
      -Uri "https://api.github.com/repos/microsoft/onnxruntime/releases/latest" `
      -Headers $headers
  }

  $tag = $RequestedVersion
  if (-not $tag.StartsWith("v")) {
    $tag = "v$tag"
  }

  return Invoke-RestMethod `
    -Uri "https://api.github.com/repos/microsoft/onnxruntime/releases/tags/$tag" `
    -Headers $headers
}

function Find-NativeWindowsAsset {
  param(
    [object]$Release,
    [string]$RequestedArch
  )

  $assetNamePattern = "^onnxruntime-win-$([regex]::Escape($RequestedArch))-[0-9].*\.zip$"
  $asset = $Release.assets |
    Where-Object {
      $_.name -match $assetNamePattern -and
      $_.name -notmatch "gpu|cuda|dml|directml|tensorrt"
    } |
    Select-Object -First 1

  if (-not $asset) {
    $names = ($Release.assets | ForEach-Object { $_.name }) -join "`n  "
    throw "Could not find a CPU Windows $RequestedArch ONNX Runtime zip in release $($Release.tag_name). Assets:`n  $names"
  }

  return $asset
}

$targetDir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($InstallDir)
$headerPath = Join-Path $targetDir "include\onnxruntime_cxx_api.h"

if ((Test-Path $targetDir) -and -not $Force) {
  if (Test-Path $headerPath) {
    Write-Host "ONNX Runtime already exists: $targetDir"
    Write-Host "Use -Force to reinstall."
    exit 0
  }

  throw "InstallDir already exists but does not look like ONNX Runtime: $targetDir"
}

$release = Get-OnnxRuntimeRelease -RequestedVersion $Version
$asset = Find-NativeWindowsAsset -Release $release -RequestedArch $Arch
$tempRoot = Join-Path ([IO.Path]::GetTempPath()) ("llvc-onnxruntime-" + [guid]::NewGuid())
$zipPath = Join-Path $tempRoot $asset.name

try {
  New-Item -ItemType Directory -Force -Path $tempRoot | Out-Null

  Write-Host "Downloading $($asset.name) from $($release.tag_name)..."
  Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $zipPath

  Write-Host "Extracting..."
  Expand-Archive -Path $zipPath -DestinationPath $tempRoot -Force

  $packageDir = Get-ChildItem -Path $tempRoot -Directory |
    Where-Object { $_.Name -like "onnxruntime-win-*" } |
    Select-Object -First 1

  if (-not $packageDir) {
    throw "Downloaded archive did not contain an onnxruntime-win-* directory."
  }

  if (Test-Path $targetDir) {
    Remove-Item -LiteralPath $targetDir -Recurse -Force
  }

  New-Item -ItemType Directory -Force -Path $targetDir | Out-Null
  Copy-Item -Path (Join-Path $packageDir.FullName "*") -Destination $targetDir -Recurse -Force

  Write-Host "Installed ONNX Runtime to: $targetDir"
  Write-Host "Next:"
  Write-Host "  cmake --preset onnx-local"
  Write-Host "  cmake --build --preset onnx-local"
} finally {
  if (Test-Path $tempRoot) {
    Remove-Item -LiteralPath $tempRoot -Recurse -Force
  }
}
