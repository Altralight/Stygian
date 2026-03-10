@echo off
setlocal
cd /d "%~dp0\..\.."
powershell -NoProfile -ExecutionPolicy Bypass -File compile\windows\build.ps1 -Target node_graph_demo_vk %*
exit /b %ERRORLEVEL%
