@echo off
setlocal
cd /d "%~dp0\..\.."

set PSH=powershell -NoProfile -ExecutionPolicy Bypass

if "%1"=="" (
  %PSH% -File tests\run_perf_matrix.ps1
) else (
  %PSH% -File tests\run_perf_matrix.ps1 %*
)

exit /b %ERRORLEVEL%
