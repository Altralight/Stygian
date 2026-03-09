param(
  [switch]$Force
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$depsRoot = Join-Path $root "build/_cmp_src"

if (-not (Test-Path $depsRoot)) {
  New-Item -ItemType Directory -Path $depsRoot | Out-Null
}

$deps = @(
  @{
    Name = "imgui"
    Url = "https://github.com/ocornut/imgui.git"
    Commit = "41765fbda723d23e04e98afec40447d149d02ec8"
  },
  @{
    Name = "clay"
    Url = "https://github.com/nicbarker/clay.git"
    Commit = "76ec3632d80c145158136fd44db501448e7b17c4"
  },
  @{
    Name = "nuklear"
    Url = "https://github.com/Immediate-Mode-UI/Nuklear.git"
    Commit = "4aff9c7c8a5b56b9cbdd7245e5cbec0db5da6d94"
  },
  @{
    Name = "dvui"
    Url = "https://github.com/david-vanderson/dvui.git"
    Commit = "654ee61a8e55ebac9386c75724ccfbf8ffc0c6da"
  }
)

function Sync-Repo {
  param(
    [string]$Name,
    [string]$Url,
    [string]$Commit
  )

  $repoPath = Join-Path $depsRoot $Name
  if (-not (Test-Path $repoPath)) {
    Write-Host "[$Name] cloning..."
    git clone --filter=blob:none $Url $repoPath
    if ($LASTEXITCODE -ne 0) {
      throw "[$Name] clone failed"
    }
  }

  $current = ""
  if (Test-Path (Join-Path $repoPath ".git")) {
    $current = (git -C $repoPath rev-parse HEAD).Trim()
  }

  if (-not $Force -and $current -eq $Commit) {
    Write-Host "[$Name] already pinned at $Commit"
    return
  }

  # force is fine here, this is a disposable local mirror under build/
  Write-Host "[$Name] syncing to $Commit"
  git -C $repoPath fetch --depth 1 origin $Commit
  if ($LASTEXITCODE -ne 0) {
    throw "[$Name] fetch failed"
  }
  git -C $repoPath checkout --force $Commit
  if ($LASTEXITCODE -ne 0) {
    throw "[$Name] checkout failed"
  }
}

foreach ($dep in $deps) {
  Sync-Repo -Name $dep.Name -Url $dep.Url -Commit $dep.Commit
}

Write-Host "[comparison deps] ready"
