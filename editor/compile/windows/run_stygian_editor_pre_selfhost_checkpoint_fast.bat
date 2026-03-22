@echo off
setlocal
cd /d "%~dp0\..\..\.."

call editor\compile\windows\build_stygian_editor_quick_checkpoint.bat %*
if errorlevel 1 exit /b %ERRORLEVEL%

set BIN=editor\build\windows

call :run_exe stygian_editor_core_unit_suite
call :run_exe stygian_editor_project_roundtrip_smoke
call :run_exe stygian_editor_export_determinism_smoke
call :run_exe stygian_editor_export_diff_quality_smoke
call :run_exe stygian_editor_phase7_behavior_bridge_smoke
call :run_exe stygian_editor_self_host_daily_flow_smoke

echo [pre_selfhost_checkpoint_fast] PASS
exit /b 0

:run_exe
echo [pre_selfhost_checkpoint_fast] Running %1.exe
"%BIN%\%1.exe"
if errorlevel 1 (
  echo [pre_selfhost_checkpoint_fast] FAIL %1.exe
  exit /b %ERRORLEVEL%
)
exit /b 0
