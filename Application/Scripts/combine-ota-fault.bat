@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_DIR=%%~fI"

if not defined PYTHON set "PYTHON=python"

set "SLOT_A_FAULT_BIN=%PROJECT_DIR%\build\SlotA-Debug-Fault\Application_SlotA_crc.bin"
set "SLOT_B_FAULT_BIN=%PROJECT_DIR%\build\SlotB-Debug-Fault\Application_SlotB_crc.bin"

set "FAULT_PACKET=%PROJECT_DIR%\build\Application_fault_ota_packet.bin"

"%PYTHON%" "%SCRIPT_DIR%combine-ota.py" ^
    --slot-a-bin "%SLOT_A_FAULT_BIN%" ^
    --slot-b-bin "%SLOT_B_FAULT_BIN%" ^
    --version-json "%PROJECT_DIR%\version.json" ^
    --output "%FAULT_PACKET%" || exit /b 1

echo Fault OTA packet: "%FAULT_PACKET%"

endlocal
