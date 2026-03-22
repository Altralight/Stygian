param(
  [string]$Clang = "",
  [string]$Zig = "",
  [switch]$AutoFetch,
  [switch]$SkipDvui
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Set-Location $root

function Resolve-ClangPair {
  param([string]$RequestedClang)

  $clang = $RequestedClang
  if (-not $clang) {
    if ($env:CLANG -and (Test-Path $env:CLANG)) {
      $clang = $env:CLANG
    } else {
      $clang = "D:\msys64\clang64\bin\clang.exe"
    }
  }
  if (-not (Test-Path $clang)) {
    throw "clang not found: $clang"
  }

  $clangxx = Join-Path (Split-Path -Parent $clang) "clang++.exe"
  if (-not (Test-Path $clangxx)) {
    throw "clang++ not found next to clang: $clangxx"
  }

  return @{ Clang = $clang; ClangXX = $clangxx }
}

function Resolve-Zig {
  param([string]$RequestedZig)

  if ($RequestedZig) {
    if (-not (Test-Path $RequestedZig)) {
      throw "zig not found: $RequestedZig"
    }
    return (Resolve-Path $RequestedZig).Path
  }

  if ($env:ZIG -and (Test-Path $env:ZIG)) {
    return (Resolve-Path $env:ZIG).Path
  }

  $zigCmd = Get-Command zig -ErrorAction SilentlyContinue
  if ($zigCmd -and $zigCmd.Source -and (Test-Path $zigCmd.Source)) {
    return (Resolve-Path $zigCmd.Source).Path
  }

  $fallback = "D:\SystemTools\Zig\0.15.2\zig.exe"
  if (Test-Path $fallback) {
    return (Resolve-Path $fallback).Path
  }

  throw "zig not found. install zig, put it on PATH, or pass -Zig"
}

function Get-ToolVersion {
  param([string]$ExePath)

  $version = & $ExePath version
  if ($LASTEXITCODE -ne 0) {
    throw "failed to query version for $ExePath"
  }
  return ($version | Select-Object -First 1).Trim()
}

function Resolve-DvuiZig {
  param([string]$PreferredZig)

  if ($PreferredZig) {
    $preferredVersion = Get-ToolVersion -ExePath $PreferredZig
    if ($preferredVersion -like "0.14.*") {
      return (Resolve-Path $PreferredZig).Path
    }
  }

  $toolsDir = "build/_cmp_tools"
  $zigDir = Join-Path $toolsDir "zig-0.14.1"
  $zigExe = Join-Path $zigDir "zig.exe"
  if (Test-Path $zigExe) {
    return (Resolve-Path $zigExe).Path
  }

  if (-not (Test-Path $toolsDir)) {
    New-Item -ItemType Directory -Path $toolsDir | Out-Null
  }

  $zipPath = Join-Path $toolsDir "zig-0.14.1.zip"
  $url = "https://ziglang.org/download/0.14.1/zig-x86_64-windows-0.14.1.zip"
  $sha256 = "554f5378228923ffd558eac35e21af020c73789d87afeabf4bfd16f2e6feed2c"

  $haveValidZip = $false
  if (Test-Path $zipPath) {
    $existingHash = (Get-FileHash -Algorithm SHA256 $zipPath).Hash.ToLowerInvariant()
    $haveValidZip = ($existingHash -eq $sha256)
  }

  if (-not $haveValidZip) {
    Write-Host "[dvui] downloading Zig 0.14.1..."
    & curl.exe -L --fail --output $zipPath $url
    if ($LASTEXITCODE -ne 0) {
      throw "[dvui] failed to download Zig 0.14.1"
    }
  }

  $downloadHash = (Get-FileHash -Algorithm SHA256 $zipPath).Hash.ToLowerInvariant()
  if ($downloadHash -ne $sha256) {
    throw "[dvui] zig 0.14.1 hash mismatch"
  }

  Add-Type -AssemblyName System.IO.Compression.FileSystem
  $zip = [IO.Compression.ZipFile]::OpenRead((Resolve-Path $zipPath))
  try {
    foreach ($entry in $zip.Entries) {
      $destPath = Join-Path $toolsDir $entry.FullName
      if ($entry.FullName.EndsWith('/')) {
        if (-not (Test-Path $destPath)) {
          New-Item -ItemType Directory -Path $destPath -Force | Out-Null
        }
        continue
      }

      $destDir = Split-Path -Parent $destPath
      if ($destDir -and -not (Test-Path $destDir)) {
        New-Item -ItemType Directory -Path $destDir -Force | Out-Null
      }

      [IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $destPath, $true)
    }
  } finally {
    $zip.Dispose()
  }

  if (-not (Test-Path $zigExe)) {
    throw "[dvui] zig 0.14.1 missing after extraction"
  }
  return (Resolve-Path $zigExe).Path
}

function Require-Dependencies {
  $required = @(
    "build/_cmp_src/imgui/imgui.h",
    "build/_cmp_src/clay/clay.h",
    "build/_cmp_src/nuklear/nuklear.h",
    "build/_cmp_src/dvui/build.zig"
  )

  $missing = @($required | Where-Object { -not (Test-Path $_) })
  if ($missing.Count -eq 0) {
    return
  }
  if (-not $AutoFetch) {
    throw "comparison deps missing. run benchmarks\comparison\fetch_deps.ps1 or pass -AutoFetch"
  }

  & powershell -NoProfile -ExecutionPolicy Bypass -File benchmarks\comparison\fetch_deps.ps1
  if ($LASTEXITCODE -ne 0) {
    throw "dependency fetch failed"
  }
}

function Patch-DvuiHeadlessSource {
  $dvuiSource = "build/_cmp_src/dvui/src/dvui.zig"
  if (-not (Test-Path $dvuiSource)) {
    throw "[dvui] source missing: $dvuiSource"
  }

  $text = Get-Content $dvuiSource -Raw
  $needle = "pub const useFreeType = !wasm;"
  $replacement = "pub const useFreeType = false;"
  if ($text.Contains($needle)) {
    $updated = $text.Replace($needle, $replacement)
    if ($updated -ne $text) {
      Set-Content -Path $dvuiSource -Value $updated -NoNewline
    }
  } elseif (-not $text.Contains($replacement)) {
    throw "[dvui] could not find useFreeType declaration to patch"
  }
}

function Invoke-Build {
  param(
    [string]$Name,
    [string]$Compiler,
    [string[]]$CompilerArgs,
    [string]$WorkDir = ""
  )

  Write-Host "[$Name] Building..."
  Push-Location
  try {
    if ($WorkDir) {
      Set-Location $WorkDir
    }
    & $Compiler @CompilerArgs
  } finally {
    Pop-Location
  }
  if ($LASTEXITCODE -ne 0) {
    throw "[$Name] build failed"
  }
  Write-Host "[$Name] Build SUCCESS"
}

$tools = Resolve-ClangPair -RequestedClang $Clang
$zig = Resolve-Zig -RequestedZig $Zig
$dvuiZig = Resolve-DvuiZig -PreferredZig $zig
Require-Dependencies
Patch-DvuiHeadlessSource

$outDir = "build/comparison"
if (-not (Test-Path $outDir)) {
  New-Item -ItemType Directory -Path $outDir | Out-Null
}
$cargoTargetDir = "$outDir/egui_target"

$commonC = @("-std=c23", "-O2", "-DNDEBUG", "-Ibenchmarks/comparison")
$commonCpp = @("-std=c++20", "-O2", "-DNDEBUG", "-Ibenchmarks/comparison")
$stygianIncludes = @(
  "-Iinclude",
  "-Ieditor/include",
  "-Iwindow",
  "-Ibackends",
  "-Iwidgets",
  "-Ilayout"
)
$stygianCommonSources = @(
  "editor/src/stygian_editor.c",
  "editor/src/stygian_editor_module.c",
  "widgets/stygian_widgets.c",
  "src/stygian.c",
  "src/stygian_memory.c",
  "src/stygian_triad.c",
  "src/stygian_unicode.c",
  "src/stygian_color.c",
  "src/stygian_icc.c",
  "src/stygian_mtsdf.c",
  "src/stygian_clipboard.c",
  "layout/stygian_layout.c",
  "layout/stygian_dock_impl.c",
  "layout/stygian_tabs.c"
)

$imguiArgs = @()
$imguiArgs += $commonCpp
$imguiArgs += @(
  "benchmarks/comparison/imgui_headless_bench.cpp",
  "build/_cmp_src/imgui/imgui.cpp",
  "build/_cmp_src/imgui/imgui_draw.cpp",
  "build/_cmp_src/imgui/imgui_tables.cpp",
  "build/_cmp_src/imgui/imgui_widgets.cpp",
  "-o", "$outDir/imgui_headless_bench.exe"
)
Invoke-Build -Name "imgui_headless_bench" -Compiler $tools.ClangXX -CompilerArgs $imguiArgs

$clayArgs = @()
$clayArgs += $commonC
$clayArgs += @(
  "benchmarks/comparison/clay_headless_bench.c",
  "-o", "$outDir/clay_headless_bench.exe"
)
Invoke-Build -Name "clay_headless_bench" -Compiler $tools.Clang -CompilerArgs $clayArgs

$nuklearArgs = @()
$nuklearArgs += $commonC
$nuklearArgs += @(
  "benchmarks/comparison/nuklear_headless_bench.c",
  "-o", "$outDir/nuklear_headless_bench.exe"
)
Invoke-Build -Name "nuklear_headless_bench" -Compiler $tools.Clang -CompilerArgs $nuklearArgs

$eguiArgs = @(
  "build",
  "--release",
  "--manifest-path", "benchmarks/comparison/egui_headless_bench/Cargo.toml",
  "--target-dir", $cargoTargetDir
)
Invoke-Build -Name "egui_headless_bench" -Compiler "cargo" -CompilerArgs $eguiArgs
Copy-Item -Force "$cargoTargetDir/release/egui_headless_bench.exe" "$outDir/egui_headless_bench.exe"

if (-not $SkipDvui) {
  $dvuiBuildDir = "benchmarks/comparison/dvui_headless_bench"
  $dvuiArgs = @(
    "build",
    "-Doptimize=ReleaseFast"
  )
  Invoke-Build -Name "dvui_headless_bench" -Compiler $dvuiZig -CompilerArgs $dvuiArgs -WorkDir $dvuiBuildDir
  Copy-Item -Force "$dvuiBuildDir/zig-out/bin/dvui_headless_bench.exe" "$outDir/dvui_headless_bench.exe"
} else {
  Write-Host "[dvui_headless_bench] skipped"
}

$stygianArgs = @()
$stygianArgs += $commonC
$stygianArgs += $stygianIncludes
$stygianArgs += @("-D_CRT_SECURE_NO_WARNINGS")
$stygianArgs += @(
  "benchmarks/comparison/stygian_headless_bench.c"
)
$stygianArgs += $stygianCommonSources
$stygianArgs += @(
  "backends/stygian_ap_gl.c",
  "window/platform/stygian_win32.c",
  "-o", "$outDir/stygian_headless_bench.exe",
  "-luser32",
  "-lgdi32",
  "-ldwmapi",
  "-lopengl32",
  "-lz",
  "-lzstd"
)
Invoke-Build -Name "stygian_headless_bench" -Compiler $tools.Clang -CompilerArgs $stygianArgs

Write-Host "[comparison] Build SUCCESS"
