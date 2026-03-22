@echo off
setlocal
cd /d "%~dp0\..\..\.."
powershell -NoProfile -ExecutionPolicy Bypass -File compile\windows\build.ps1 -Target stygian_editor_phase6_components_styles_variables_smoke -OutputDir editor\build\windows %*
if errorlevel 1 exit /b %ERRORLEVEL%
exit /b %ERRORLEVEL%
