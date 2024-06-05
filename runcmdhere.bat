@echo off
setlocal

rem Check if the script is already running
set "currentScript=%~n0"
tasklist /FI "IMAGENAME eq cmd.exe" /FO TABLE /NH | find /I "%currentScript%.bat" >nul && goto :end

cd /d %~dp0
cmd /k echo hello

:end
endlocal
