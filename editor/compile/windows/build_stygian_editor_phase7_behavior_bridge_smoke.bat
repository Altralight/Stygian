@echo off
setlocal
cd /d "%~dp0\..\..\.."
powershell -NoProfile -ExecutionPolicy Bypass -File compile\windows\build.ps1 -Target stygian_editor_phase7_behavior_bridge_smoke -OutputDir editor\build\windows %*
if errorlevel 1 exit /b %ERRORLEVEL%
exit /b %ERRORLEVEL%
