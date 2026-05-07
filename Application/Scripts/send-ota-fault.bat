@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_DIR=%%~fI"

if not defined PYTHON set "PYTHON=python"

if "%~1"=="" (
    echo Usage: send-ota-fault.bat COMx [packet]
    exit /b 1
)

set "PORT=%~1"

if "%~2"=="" (
    set "PACKET=%PROJECT_DIR%\build\Application_fault_ota_packet.bin"
) else (
    set "PACKET=%~2"
)

"%PYTHON%" "%SCRIPT_DIR%send-ota.py" ^
    --port "%PORT%" ^
    --baudrate 115200 ^
    --packet "%PACKET%"

endlocal
