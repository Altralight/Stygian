@echo off
setlocal
cd /d "%~dp0\..\..\.."
powershell -NoProfile -ExecutionPolicy Bypass -File compile\windows\build.ps1 -Target stygianweb_vk -OutputDir showcase %*
exit /b %ERRORLEVEL%
