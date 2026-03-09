@echo off
setlocal
cd /d "%~dp0\..\.."
powershell -NoProfile -ExecutionPolicy Bypass -File compile\windows\build.ps1 -Target quickwindow_capture_smoke -EnableCapture %*
exit /b %ERRORLEVEL%
