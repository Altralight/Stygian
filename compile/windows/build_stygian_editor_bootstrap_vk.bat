@echo off
setlocal
cd /d "%~dp0\..\.."
powershell -NoProfile -ExecutionPolicy Bypass -File compile\windows\build.ps1 -Target stygian_editor_bootstrap_vk %*
exit /b %ERRORLEVEL%
