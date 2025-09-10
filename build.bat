@echo off
rem Wrapper to run the PowerShell build script with ExecutionPolicy Bypass
setlocal
set SCRIPT=%~dp0build.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT%" %*
endlocal
