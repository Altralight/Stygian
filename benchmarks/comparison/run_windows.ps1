param(
  [string]$Clang = "",
  [switch]$AutoFetch,
  [switch]$NoBuild,
  [string]$OutputDir = "",
  [int]$Count = 10000,
  [int]$MutateCount = 100,
  [double]$Seconds = 2.0,
  [double]$WarmupSeconds = 1.0,
  [string[]]$Scenes = @("cards", "textwall", "graph", "inspector", "hierarchy"),
  [string[]]$Libraries = @()
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Set-Location $root

function Parse-CompSummaryLine {
  param([string]$Line)

  if (-not $Line.StartsWith("COMPSUMMARY ")) {
    return $null
  }

  $summary = @{}
  foreach ($token in $Line.Substring(12).Split(' ')) {
    if ($token -match '^([^=]+)=(.*)$') {
      $summary[$matches[1]] = $matches[2]
    }
  }
  return $summary
}

function Parse-StygianDetailLine {
  param([string]$Line)

  if (-not $Line.StartsWith("STYGIANDETAIL ")) {
    return $null
  }

  $detail = @{}
  foreach ($token in $Line.Substring(14).Split(' ')) {
    if ($token -match '^([^=]+)=(.*)$') {
      $detail[$matches[1]] = $matches[2]
    }
  }
  return $detail
}

function Append-CompSummaryCsv {
  param(
    [string]$Path,
    [hashtable]$Summary
  )

  $header = "library,mode,scene,scenario,count,mutate_count,frames,wall_fps,avg_frame_ms,min_frame_ms,max_frame_ms,avg_commands,avg_vertices,avg_indices,avg_text_bytes"
  if (-not (Test-Path $Path)) {
    Set-Content -Path $Path -Value $header
  }

  $row = "{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11},{12},{13},{14}" -f `
    $Summary["library"], `
    $Summary["mode"], `
    $Summary["scene"], `
    $Summary["scenario"], `
    $Summary["count"], `
    $Summary["mutate"], `
    $Summary["frames"], `
    $Summary["wall_fps"], `
    $Summary["frame_ms"], `
    $Summary["min_ms"], `
    $Summary["max_ms"], `
    $Summary["commands"], `
    $Summary["vertices"], `
    $Summary["indices"], `
    $Summary["text_bytes"]
  Add-Content -Path $Path -Value $row
}

function Find-Row {
  param(
    [object[]]$Rows,
    [string]$Library,
    [string]$Mode,
    [string]$Scene,
    [string]$Scenario
  )

  return $Rows | Where-Object {
    $_.library -eq $Library -and
    $_.mode -eq $Mode -and
    $_.scene -eq $Scene -and
    $_.scenario -eq $Scenario
  } | Select-Object -First 1
}

function Quote-CmdArg {
  param([string]$Value)
  return '"' + ($Value -replace '"', '\"') + '"'
}

if (-not $NoBuild) {
  $buildArgs = @(
    "-NoProfile",
    "-ExecutionPolicy", "Bypass",
    "-File", "benchmarks\comparison\build_windows.ps1"
  )
  if ($Clang) {
    $buildArgs += @("-Clang", $Clang)
  }
  if ($AutoFetch) {
    $buildArgs += "-AutoFetch"
  }
  if ($Libraries.Count -gt 0 -and -not ($Libraries -contains "dvui")) {
    $buildArgs += "-SkipDvui"
  }

  & powershell @buildArgs
  if ($LASTEXITCODE -ne 0) {
    throw "comparison build failed"
  }
}

if (-not $OutputDir) {
  $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
  $OutputDir = "build/comparison/runs/$stamp"
}
if (-not (Test-Path $OutputDir)) {
  New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
}

$clangBin = ""
if ($Clang) {
  $clangBin = Split-Path -Parent $Clang
} else {
  $clangBin = "D:\msys64\clang64\bin"
}
if ($clangBin -and (Test-Path $clangBin)) {
  $env:PATH = "$clangBin;$env:PATH"
}

$csvPath = Join-Path $OutputDir "summary.csv"
$logPath = Join-Path $OutputDir "summary.log"
$mdPath = Join-Path $OutputDir "summary.md"

$targets = @(
  @{
    Name = "stygian"
    Exe = "build/comparison/stygian_headless_bench.exe"
    Modes = @("cpu-authoring-full-build", "raw-gpu-resident", "eval-only-replay")
    WritesCsv = $true
  },
  @{ Name = "dvui"; Exe = "build/comparison/dvui_headless_bench.exe"; Modes = @(); WritesCsv = $false },
  @{ Name = "egui"; Exe = "build/comparison/egui_headless_bench.exe"; Modes = @(); WritesCsv = $true },
  @{ Name = "imgui"; Exe = "build/comparison/imgui_headless_bench.exe"; Modes = @(); WritesCsv = $true },
  @{ Name = "clay"; Exe = "build/comparison/clay_headless_bench.exe"; Modes = @(); WritesCsv = $true },
  @{ Name = "nuklear"; Exe = "build/comparison/nuklear_headless_bench.exe"; Modes = @(); WritesCsv = $true }
)

if ($Libraries.Count -gt 0) {
  $targetLookup = @{}
  foreach ($name in $Libraries) {
    $targetLookup[$name.ToLowerInvariant()] = $true
  }
  $targets = @($targets | Where-Object { $targetLookup.ContainsKey($_.Name.ToLowerInvariant()) })
  if ($targets.Count -eq 0) {
    throw "no comparison targets matched -Libraries"
  }
}

$scenes = @($Scenes)
$scenarios = @("static", "sparse", "fullhot")

foreach ($target in $targets) {
  $modes = @($target.Modes)
  if ($modes.Count -eq 0) {
    $modes = @($null)
  }
  foreach ($mode in $modes) {
    foreach ($scene in $scenes) {
      foreach ($scenario in $scenarios) {
      $args = @(
        "--scene", $scene,
        "--scenario", $scenario,
        "--count", "$Count",
        "--mutate-count", "$MutateCount",
        "--seconds", "$Seconds",
        "--warmup-seconds", "$WarmupSeconds"
      )
      if ($target.WritesCsv -ne $false) {
        $args += @("--csv", $csvPath)
      }
      if ($mode) {
        $args += @("--mode", $mode)
      }

      if ($mode) {
        Write-Host "[$($target.Name)] mode=$mode scene=$scene scenario=$scenario"
      } else {
        Write-Host "[$($target.Name)] scene=$scene scenario=$scenario"
      }
      if ($target.WritesCsv -eq $false) {
        $quotedArgs = @($args | ForEach-Object { Quote-CmdArg -Value "$_" })
        $commandLine = "$(Quote-CmdArg -Value $target.Exe) $($quotedArgs -join ' ')"
        $output = & cmd.exe /d /c "$commandLine 2>&1"
      } else {
        $output = & $target.Exe @args
      }
      if ($LASTEXITCODE -ne 0) {
        if ($mode) {
          throw "[$($target.Name)] mode=$mode scene=$scene scenario=$scenario failed"
        }
        throw "[$($target.Name)] scene=$scene scenario=$scenario failed"
      }
      $outputLines = @($output)
      $outputLines | Tee-Object -FilePath $logPath -Append | Out-Host
      if ($target.WritesCsv -eq $false) {
        $summaryLine = $outputLines | Where-Object { $_ -like "COMPSUMMARY *" } | Select-Object -Last 1
        if (-not $summaryLine) {
          throw "[$($target.Name)] missing COMPSUMMARY for scene=$scene scenario=$scenario"
        }
        $summary = Parse-CompSummaryLine -Line $summaryLine
        if (-not $summary) {
          throw "[$($target.Name)] could not parse COMPSUMMARY for scene=$scene scenario=$scenario"
        }
        Append-CompSummaryCsv -Path $csvPath -Summary $summary
      }
      }
    }
  }
}

$rows = Import-Csv $csvPath | Sort-Object library, mode, scene, scenario
$stygianDetailMap = @{}
if (Test-Path $logPath) {
  foreach ($line in Get-Content $logPath) {
    $detail = Parse-StygianDetailLine -Line $line
    if ($null -eq $detail) {
      continue
    }
    $key = "{0}|{1}|{2}" -f $detail["mode"], $detail["scene"], $detail["scenario"]
    $stygianDetailMap[$key] = $detail
  }
}

$cpuRows = @($rows | Where-Object {
  $_.mode -eq "headless-primitive" -or
  ($_.library -eq "stygian" -and $_.mode -eq "cpu-authoring-full-build")
})
$stygianRows = @($rows | Where-Object {
  $_.library -eq "stygian" -and $_.mode -ne "cpu-authoring-full-build"
})

$lines = @()
$lines += "# Headless Comparison Summary"
$lines += ""
$lines += "## Scope"
$lines += ""
$lines += '- `Count` and `Mutate` are logical scene driver inputs, not actual primitive, glyph, or element counts.'
$lines += '- `headless-primitive` rows are CPU-side scene build only.'
$lines += '- `cpu-authoring-full-build` rows are Stygian full scene authoring every frame with backend render skipped.'
$lines += '- `raw-gpu-resident` rows are Stygian offscreen render passes with real GPU, upload, and text expansion cost.'
$lines += '- `eval-only-replay` rows are Stygian author/eval/replay with replay allowed and backend render skipped.'
$lines += '- These modes should not be collapsed into a single flat leaderboard.'
$lines += ""
$lines += "## CPU Builder Rows"
$lines += ""
$lines += "These are the closest thing to an apples-to-apples CPU churn lane in this harness."
$lines += 'Stygian''s `cpu-authoring-full-build` mode rebuilds both scopes every frame and skips backend rendering.'
$lines += ""
$lines += "| Library | Mode | Scene | Scenario | Logical Items | Mutate | FPS | Avg Frame ms | Avg Commands | Avg Vertices | Avg Indices |"
$lines += "|---------|------|-------|----------|---------------|--------|-----|--------------|--------------|--------------|-------------|"
foreach ($row in $cpuRows) {
  $lines += "| $($row.library) | $($row.mode) | $($row.scene) | $($row.scenario) | $($row.count) | $($row.mutate_count) | $([double]::Parse($row.wall_fps).ToString('F2')) | $([double]::Parse($row.avg_frame_ms).ToString('F4')) | $($row.avg_commands) | $($row.avg_vertices) | $($row.avg_indices) |"
}
$lines += ""
$lines += "## Stygian Native Modes"
$lines += ""
$lines += "These rows show Stygian's native GPU path. `Logical Items` can expand into far more actual Stygian elements."
$lines += "They show Stygian's native strengths, but they are not fair CPU-builder comparisons."
$lines += ""
$lines += "| Mode | Scene | Scenario | Logical Items | Mutate | FPS | Avg Frame ms | Avg Elements | Avg Upload Bytes | Avg Upload Ranges | Replay Hit Rate |"
$lines += "|------|-------|----------|---------------|--------|-----|--------------|--------------|------------------|-------------------|-----------------|"
foreach ($row in $stygianRows) {
  $detailKey = "{0}|{1}|{2}" -f $row.mode, $row.scene, $row.scenario
  $detail = $stygianDetailMap[$detailKey]
  $avgElements = "n/a"
  $avgUploadBytes = "n/a"
  $avgUploadRanges = "n/a"
  $replayHitRate = "n/a"
  if ($detail) {
    $avgElements = $detail["avg_elements"]
    $avgUploadBytes = $detail["avg_upload_bytes"]
    $avgUploadRanges = $detail["avg_upload_ranges"]
    $replayHitRate = ([double]::Parse($detail["avg_replay_hits"]) * 100.0).ToString('F0') + "%"
  }
  $lines += "| $($row.mode) | $($row.scene) | $($row.scenario) | $($row.count) | $($row.mutate_count) | $([double]::Parse($row.wall_fps).ToString('F2')) | $([double]::Parse($row.avg_frame_ms).ToString('F4')) | $avgElements | $avgUploadBytes | $avgUploadRanges | $replayHitRate |"
}
$lines += ""
$lines += "## Interpretation"
$lines += ""
$lines += "- Use `CPU Builder Rows` for cross-library CPU scene-build cost."
$lines += "- Use `Stygian Native Modes` for GPU-native Stygian behavior, including replay and upload cost."
$lines += "- Treat `Logical Items` as scene-driver inputs, not as fully normalized primitive counts."
$lines += ""
$lines += "Artifacts:"
$lines += "- `summary.csv` for machine-readable results"
$lines += "- `summary.log` for raw harness stdout and `STYGIANDETAIL` lines"
$lines += "- `summary.md` separates CPU authoring rows from Stygian native rows"

Set-Content -Path $mdPath -Value $lines

$latestDir = "build/comparison/latest"
if (-not (Test-Path $latestDir)) {
  New-Item -ItemType Directory -Path $latestDir -Force | Out-Null
}
Copy-Item -Force $csvPath (Join-Path $latestDir "summary.csv")
Copy-Item -Force $logPath (Join-Path $latestDir "summary.log")
Copy-Item -Force $mdPath (Join-Path $latestDir "summary.md")

Write-Host "[comparison] results:"
Write-Host "  csv: $csvPath"
Write-Host "  log: $logPath"
Write-Host "  md : $mdPath"
Write-Host "  latest: $latestDir"
