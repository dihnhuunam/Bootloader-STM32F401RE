@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_DIR=%%~fI"

if not defined PYTHON set "PYTHON=python"

if "%~1"=="" (
    set "SLOT_A_BIN=%PROJECT_DIR%\build\SlotA-Debug\Application_3_SlotA_crc.bin"
) else (
    set "SLOT_A_BIN=%~1"
)

if "%~2"=="" (
    set "SLOT_B_BIN=%PROJECT_DIR%\build\SlotB-Debug\Application_3_SlotB_crc.bin"
) else (
    set "SLOT_B_BIN=%~2"
)

if "%~3"=="" (
    set "OUTPUT_BIN=%PROJECT_DIR%\build\Application_3_ota_packet.bin"
) else (
    set "OUTPUT_BIN=%~3"
)

"%PYTHON%" "%SCRIPT_DIR%combine-ota.py" ^
    --slot-a-bin "%SLOT_A_BIN%" ^
    --slot-b-bin "%SLOT_B_BIN%" ^
    --version-json "%PROJECT_DIR%\version.json" ^
    --output "%OUTPUT_BIN%"

endlocal
