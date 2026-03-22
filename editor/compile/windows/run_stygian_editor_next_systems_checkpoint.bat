@echo off
setlocal
cd /d "%~dp0\..\..\.."

call editor\compile\windows\build_stygian_editor_next_systems_checkpoint.bat %*
if errorlevel 1 exit /b %ERRORLEVEL%

set BIN=editor\build\windows

call :run_exe stygian_editor_project_roundtrip_smoke
if errorlevel 1 exit /b %ERRORLEVEL%
call :run_exe stygian_editor_layout_export_diff_smoke
if errorlevel 1 exit /b %ERRORLEVEL%
call :run_exe stygian_editor_variable_binding_parity_smoke
if errorlevel 1 exit /b %ERRORLEVEL%
call :run_exe stygian_editor_component_property_export_smoke
if errorlevel 1 exit /b %ERRORLEVEL%
call :run_exe stygian_editor_shader_effect_batch_e_smoke
if errorlevel 1 exit /b %ERRORLEVEL%
call :run_exe stygian_editor_next_systems_fixture_smoke
if errorlevel 1 exit /b %ERRORLEVEL%

echo [next_systems_checkpoint] PASS
exit /b 0

:run_exe
echo [next_systems_checkpoint] Running %1.exe
"%BIN%\%1.exe"
if errorlevel 1 (
  echo [next_systems_checkpoint] FAIL %1.exe
  exit /b %ERRORLEVEL%
)
exit /b 0
