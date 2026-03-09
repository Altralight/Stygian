param(
  [string]$Target,
  [string]$Group,
  [string]$Clang = "",
  [string]$VulkanSdk = "",
  [switch]$NoShaderCheck,
  [switch]$EnableCapture
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Set-Location $root

$manifestPath = Join-Path $root "compile/targets.json"
if (-not (Test-Path $manifestPath)) {
  throw "missing manifest: $manifestPath"
}
$manifest = Get-Content -Raw $manifestPath | ConvertFrom-Json

if (-not $Clang) {
  if ($env:CLANG -and (Test-Path $env:CLANG)) {
    $Clang = $env:CLANG
  } else {
    $Clang = "D:\msys64\clang64\bin\clang.exe"
  }
}
if (-not (Test-Path $Clang)) {
  throw "clang not found: $Clang"
}

if (-not $VulkanSdk) {
  if ($env:VULKAN_SDK) {
    $VulkanSdk = $env:VULKAN_SDK
  } else {
    $VulkanSdk = "D:\SystemTools\VulkanSDK\1.4"
  }
}

if (-not (Test-Path "build")) {
  New-Item -ItemType Directory -Path "build" | Out-Null
}

function Test-ShaderOutputs {
  param([string]$Name, [switch]$Skip)
  if ($Skip) { return }
  if (-not (Test-Path "shaders/build/stygian.vert.glsl")) {
    throw "[$Name] missing shader output: shaders/build/stygian.vert.glsl (run build_shaders.bat)"
  }
  if (-not (Test-Path "shaders/build/stygian.frag.glsl")) {
    throw "[$Name] missing shader output: shaders/build/stygian.frag.glsl (run build_shaders.bat)"
  }
}

function Invoke-TargetBuild {
  param([string]$Name)

  $targetDef = $manifest.targets.$Name
  if (-not $targetDef) {
    $availableTargets = ($manifest.targets.PSObject.Properties.Name | Sort-Object) -join ", "
    throw "unknown target: $Name`navailable targets: $availableTargets"
  }

  Test-ShaderOutputs -Name $Name -Skip:$NoShaderCheck

  $args = @()
  $args += $manifest.common.flags
  foreach ($inc in $manifest.common.includes) {
    $args += "-I"
    $args += $inc
  }
  if ($targetDef.backend -eq "vk") {
    if (-not (Test-Path $VulkanSdk)) {
      throw "[$Name] Vulkan SDK not found: $VulkanSdk"
    }
    $args += "-I$VulkanSdk\Include"
    $args += "-DSTYGIAN_DEMO_VULKAN"
    $args += "-DSTYGIAN_VULKAN"
  }
  foreach ($def in $manifest.common.defines) {
    $args += "-D$def"
  }

  $captureRequested = $EnableCapture -or ($env:STYGIAN_ENABLE_CAPTURE -eq "1")
  if ($captureRequested) {
    $captureInclude = "capture/include"
    $captureSourceRoot = "capture/src"
    if (-not (Test-Path $captureInclude) -or -not (Test-Path $captureSourceRoot)) {
      throw "[$Name] capture requested but missing local capture sources under capture/ (expected capture/include + capture/src)"
    }
    $captureSources = Get-ChildItem -Path $captureSourceRoot -Recurse -Filter "*.c" |
      Sort-Object FullName |
      ForEach-Object { $_.FullName }
    if (-not $captureSources -or $captureSources.Count -eq 0) {
      throw "[$Name] capture requested but no C sources found under $captureSourceRoot"
    }
    $args += "-DSTYGIAN_CAPTURE_ENABLED=1"
    $args += "-I"
    $args += $captureInclude
    $args += "-I"
    $args += $captureSourceRoot
    $args += $captureSources
    Write-Host "[$Name] Capture workload enabled (local capture/ sources)"
  }

  $args += $targetDef.entry_source
  $args += $manifest.common.sources
  if ($targetDef.backend -eq "vk") {
    $args += $manifest.common.vk_backend_source
  } else {
    $args += $manifest.common.gl_backend_source
  }
  $args += "window/platform/stygian_win32.c"

  $outPath = "build/$($targetDef.output_stem).exe"
  $args += "-o"
  $args += $outPath

  if ($targetDef.backend -eq "vk") {
    $args += "-L$VulkanSdk\Lib"
    $args += "-lvulkan-1"
  }
  $args += "-luser32"
  $args += "-lgdi32"
  $args += "-ldwmapi"
  $args += "-lopengl32"
  $args += "-lz"
  $args += "-lzstd"
  if ($captureRequested) {
    $args += "-lole32"
    $args += "-loleaut32"
    $args += "-lmfplat"
    $args += "-lmfreadwrite"
    $args += "-lmfuuid"
  }

  Write-Host "[$Name] Building..."
  & $Clang @args
  if ($LASTEXITCODE -ne 0) {
    throw "[$Name] build failed"
  }
  Write-Host "[$Name] Build SUCCESS: $outPath"
}

if ($Group) {
  $groupDef = $manifest.groups.$Group
  if (-not $groupDef) {
    $availableGroups = ($manifest.groups.PSObject.Properties.Name | Sort-Object) -join ", "
    throw "unknown group: $Group`navailable groups: $availableGroups"
  }
  foreach ($name in $groupDef) {
    Invoke-TargetBuild -Name $name
  }
  Write-Host "[$Group] Build SUCCESS"
  exit 0
}

if (-not $Target) {
  throw "specify -Target <name> or -Group <name>"
}

Invoke-TargetBuild -Name $Target
