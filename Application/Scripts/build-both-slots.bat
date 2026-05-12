@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_DIR=%%~fI"

cd /d "%PROJECT_DIR%" || exit /b 1

cmake --preset SlotA-Debug || exit /b 1
cmake --build --preset SlotA-Debug || exit /b 1

cmake --preset SlotB-Debug || exit /b 1
cmake --build --preset SlotB-Debug || exit /b 1

call "%SCRIPT_DIR%combine-ota.bat" || exit /b 1

endlocal
