param(
  [int[]]$Counts = @(10000, 50000, 100000),
  [int]$Seconds = 4,
  [double]$WarmupSeconds = 1.0,
  [switch]$Rebuild,
  [switch]$SkipNodeGraph,
  [switch]$SkipPerfSuite,
  [string]$OutputDir
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
if (-not $OutputDir -or $OutputDir.Trim().Length -eq 0) {
  $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
  $OutputDir = Join-Path $root ("build\perf_matrix\" + $stamp)
}

$null = New-Item -ItemType Directory -Force -Path $OutputDir
$logDir = Join-Path $OutputDir "logs"
$csvDir = Join-Path $OutputDir "csv"
$null = New-Item -ItemType Directory -Force -Path $logDir
$null = New-Item -ItemType Directory -Force -Path $csvDir

$targets = @(
  @{
    Name = "perf_pathological_suite"
    BuildScript = "compile\windows\build_perf_pathological_suite.bat"
    ExePath = "build\perf_pathological_suite.exe"
  },
  @{
    Name = "node_graph_demo"
    BuildScript = "compile\windows\build_node_graph_demo.bat"
    ExePath = "build\node_graph_demo.exe"
  }
)

function Invoke-BuildIfNeeded {
  param([hashtable]$Target)
  $exe = Join-Path $root $Target.ExePath
  if ($Rebuild -or -not (Test-Path $exe)) {
    Write-Host "[matrix] building $($Target.Name)"
    Push-Location $root
    try {
      & cmd /c $Target.BuildScript
      if ($LASTEXITCODE -ne 0) {
        throw "build failed for $($Target.Name)"
      }
    } finally {
      Pop-Location
    }
  }
}

function Invoke-Capture {
  param(
    [string]$Name,
    [string]$ExePath,
    [string[]]$CommandArgs,
    [string]$LogPath,
    [int]$TimeoutSeconds = 60
  )
  Write-Host ("[matrix] run {0} {1}" -f $Name, ($CommandArgs -join " "))
  Push-Location $root
  try {
    $stderrPath = [System.IO.Path]::ChangeExtension($LogPath, ".stderr.log")
    Remove-Item -Path $LogPath, $stderrPath -ErrorAction SilentlyContinue

    # GUI demos hate the naive PowerShell pipeline path. Spawn clean and read logs after.
    $proc = Start-Process -FilePath $ExePath -ArgumentList $CommandArgs `
      -WorkingDirectory $root `
      -RedirectStandardOutput $LogPath `
      -RedirectStandardError $stderrPath `
      -PassThru

    if (-not $proc.WaitForExit($TimeoutSeconds * 1000)) {
      Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
      throw "$Name timed out after $TimeoutSeconds second(s)"
    }
    $exitCode = $proc.ExitCode
    if ($null -ne $exitCode -and "$exitCode".Length -gt 0 -and
        [int]$exitCode -ne 0) {
      throw "$Name failed with exit code $exitCode"
    }

    $output = @()
    if (Test-Path $LogPath) {
      $output += Get-Content -Path $LogPath
    }
    if (Test-Path $stderrPath) {
      $stderr = Get-Content -Path $stderrPath
      if ($stderr.Count -gt 0) {
        Add-Content -Path $LogPath -Value $stderr
        $output += $stderr
      }
    }
    return ,@($output | ForEach-Object { $_.ToString() })
  } finally {
    Pop-Location
  }
}

function Parse-KeyValueLine {
  param([string]$Line)
  $map = @{}
  foreach ($m in [regex]::Matches($Line, '(\w+)=([^\s]+)')) {
    $map[$m.Groups[1].Value] = $m.Groups[2].Value
  }
  return $map
}

function Parse-PerfSummary {
  param([string[]]$Lines)
  $line = $Lines | Where-Object { $_ -like "PERFSUMMARY *" } | Select-Object -Last 1
  if (-not $line) {
    return $null
  }
  $row = Parse-KeyValueLine $line
  if ($row.Count -eq 0) {
    return $null
  }
  return [pscustomobject]@{
    kind            = "perf_suite"
    scenario        = $row["scenario"]
    backend         = $row["backend"]
    mode            = $row["mode"]
    count           = [int]$row["count"]
    mutate          = [int]$row["mutate"]
    tick_hz         = [double]$row["tick_hz"]
    capture_seconds = [double]$row["capture_seconds"]
    render_frames   = [int]$row["render"]
    eval_frames     = [int]$row["eval"]
    render_ms       = [double]$row["render_ms"]
    gpu_ms          = [double]$row["gpu_ms"]
    build_ms        = [double]$row["build_ms"]
    submit_ms       = [double]$row["submit_ms"]
    present_ms      = [double]$row["present_ms"]
    upload_bytes    = [double]$row["upload_bytes"]
    upload_ranges   = [double]$row["upload_ranges"]
  }
}

function Parse-NodeBenchSummary {
  param([string[]]$Lines)
  $line = $Lines | Where-Object { $_ -like "NODEBENCH summary *" } | Select-Object -Last 1
  if (-not $line) {
    return $null
  }
  $row = Parse-KeyValueLine $line
  if ($row.Count -eq 0) {
    return $null
  }
  return [pscustomobject]@{
    kind            = "node_graph"
    scenario        = "node_graph"
    backend         = "gl"
    mode            = $row["mode"]
    count           = 64
    mutate          = 0
    tick_hz         = 0.0
    capture_seconds = [double]$row["seconds"]
    render_frames   = [int]$row["render"]
    eval_frames     = 0
    render_ms       = [double]$row["render_ms"]
    gpu_ms          = [double]$row["gpu_ms"]
    build_ms        = [double]$row["build_ms"]
    submit_ms       = [double]$row["submit_ms"]
    present_ms      = 0.0
    upload_bytes    = 0.0
    upload_ranges   = 0.0
  }
}

function Parse-Renderer {
  param([string[]]$Lines)
  $line = $Lines | Where-Object { $_ -match "Renderer:\s*(.+)$" } | Select-Object -First 1
  if (-not $line) {
    return ""
  }
  return ([regex]::Match($line, "Renderer:\s*(.+)$")).Groups[1].Value
}

function Write-MarkdownSummary {
  param(
    [string]$Path,
    [string]$Renderer,
    [object[]]$Rows
  )
  $lines = @()
  $lines += "# Stygian Perf Matrix"
  $lines += ""
  if ($Renderer) {
    $lines += "Renderer: $Renderer"
    $lines += ""
  }
  $lines += "| Kind | Scenario | Mode | Count | Mutate | Render ms | Build ms | Submit ms | GPU ms | Upload Bytes |"
  $lines += "|------|----------|------|-------|--------|-----------|----------|-----------|--------|--------------|"
  foreach ($row in $Rows) {
    $lines += ("| {0} | {1} | {2} | {3} | {4} | {5:N4} | {6:N4} | {7:N4} | {8:N4} | {9:N0} |" -f `
      $row.kind, $row.scenario, $row.mode, $row.count, $row.mutate,
      $row.render_ms, $row.build_ms, $row.submit_ms, $row.gpu_ms,
      $row.upload_bytes)
  }
  $lines | Set-Content -Path $Path -Encoding UTF8
}

Invoke-BuildIfNeeded $targets[0]
if (-not $SkipNodeGraph) {
  Invoke-BuildIfNeeded $targets[1]
}

$results = New-Object System.Collections.Generic.List[object]
$renderer = ""

if (-not $SkipPerfSuite) {
  $perfExe = Join-Path $root "build\perf_pathological_suite.exe"
  foreach ($count in $Counts) {
    $sparseMutate = [Math]::Max([int]($count / 100), 1)
    foreach ($scenario in @("static", "sparse", "fullhot")) {
      $csvPath = Join-Path $csvDir ("{0}_{1}.csv" -f $scenario, $count)
      $logPath = Join-Path $logDir ("{0}_{1}.log" -f $scenario, $count)
      $args = @("--scenario", $scenario, "--raw", "--seconds", "$Seconds",
                "--warmup-seconds", "$WarmupSeconds", "--count", "$count",
                "--csv", $csvPath)
      if ($scenario -eq "sparse") {
        $args += @("--mutate-count", "$sparseMutate")
      }
      $output = Invoke-Capture -Name "$scenario-$count" -ExePath $perfExe `
        -CommandArgs $args -LogPath $logPath
      if (-not $renderer) {
        $renderer = Parse-Renderer $output
      }
      $summary = Parse-PerfSummary $output
      if (-not $summary) {
        throw "missing PERFSUMMARY for $scenario-$count"
      }
      $results.Add($summary)
    }
  }

  foreach ($spec in @(
      @{ Scenario = "text_static"; TickHz = 0; Name = "text_static" },
      @{ Scenario = "text"; TickHz = 30; Name = "text_tick30" }
    )) {
    $csvPath = Join-Path $csvDir ($spec.Name + ".csv")
    $logPath = Join-Path $logDir ($spec.Name + ".log")
    $args = @("--scenario", $spec.Scenario, "--raw", "--seconds", "$Seconds",
              "--warmup-seconds", "$WarmupSeconds", "--csv", $csvPath)
    if ($spec.TickHz -gt 0) {
      $args += @("--tick-hz", "$($spec.TickHz)")
    }
    $output = Invoke-Capture -Name $spec.Name -ExePath $perfExe -CommandArgs $args `
      -LogPath $logPath
    if (-not $renderer) {
      $renderer = Parse-Renderer $output
    }
    $summary = Parse-PerfSummary $output
    if (-not $summary) {
      throw "missing PERFSUMMARY for $($spec.Name)"
    }
    $results.Add($summary)
  }
}

if (-not $SkipNodeGraph) {
  $nodeExe = Join-Path $root "build\node_graph_demo.exe"
  foreach ($mode in @(
      @{ Name = "node_pretty"; Args = @("--no-perf", "--bench-seconds", "$Seconds") },
      @{ Name = "node_benchmark"; Args = @("--benchmark-mode", "--no-perf", "--bench-seconds", "$Seconds") }
    )) {
    $logPath = Join-Path $logDir ($mode.Name + ".log")
    $output = Invoke-Capture -Name $mode.Name -ExePath $nodeExe -CommandArgs $mode.Args `
      -LogPath $logPath
    if (-not $renderer) {
      $renderer = Parse-Renderer $output
    }
    $summary = Parse-NodeBenchSummary $output
    if (-not $summary) {
      throw "missing NODEBENCH summary for $($mode.Name)"
    }
    $results.Add($summary)
  }
}

$summaryCsv = Join-Path $OutputDir "summary.csv"
$summaryMd = Join-Path $OutputDir "summary.md"
$results | Sort-Object kind, scenario, mode, count | Export-Csv -Path $summaryCsv -NoTypeInformation
Write-MarkdownSummary -Path $summaryMd -Renderer $renderer -Rows @($results | Sort-Object kind, scenario, mode, count)

Write-Host "[matrix] wrote $summaryCsv"
Write-Host "[matrix] wrote $summaryMd"
Write-Host "[matrix] output dir: $OutputDir"
