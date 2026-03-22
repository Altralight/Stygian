@echo off
setlocal
cd /d "%~dp0\..\..\.."
powershell -NoProfile -ExecutionPolicy Bypass -File compile\windows\build.ps1 -Target stygian_editor_component_linkage_smoke -OutputDir editor\build\windows %*
if errorlevel 1 exit /b %ERRORLEVEL%
exit /b %ERRORLEVEL%
