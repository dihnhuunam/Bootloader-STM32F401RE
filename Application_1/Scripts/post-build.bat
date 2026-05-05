@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_DIR=%%~fI"

if "%~1"=="" (
    set "ELF_FILE=%PROJECT_DIR%\build\Debug\Application_1.elf"
) else (
    set "ELF_FILE=%~1"
)

if "%~2"=="" (
    for %%I in ("%ELF_FILE%") do set "OUT_DIR=%%~dpI"
) else (
    set "OUT_DIR=%~2"
)

if "%~3"=="" (
    set "TARGET_NAME=Application_1"
) else (
    set "TARGET_NAME=%~3"
)

if not defined OBJCOPY set "OBJCOPY=arm-none-eabi-objcopy"
if not defined PYTHON set "PYTHON=python"

set "RAW_BIN=%OUT_DIR%\%TARGET_NAME%_raw.bin"
set "PAYLOAD_BIN=%OUT_DIR%\%TARGET_NAME%_payload.bin"
set "PATCHED_BIN=%OUT_DIR%\%TARGET_NAME%_crc.bin"
set "PATCHED_ELF=%OUT_DIR%\%TARGET_NAME%_crc.elf"
set "HEADER_BIN=%OUT_DIR%\%TARGET_NAME%_app_header.bin"
set "VERSION_JSON=%PROJECT_DIR%\version.json"

if not exist "%ELF_FILE%" (
    echo ERROR: ELF file not found: "%ELF_FILE%"
    exit /b 1
)

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

"%OBJCOPY%" -O binary "%ELF_FILE%" "%RAW_BIN%" || exit /b 1
"%OBJCOPY%" -O binary --remove-section=.app_header "%ELF_FILE%" "%PAYLOAD_BIN%" || exit /b 1

"%PYTHON%" "%SCRIPT_DIR%crc32.py" ^
    --payload-bin "%PAYLOAD_BIN%" ^
    --raw-bin "%RAW_BIN%" ^
    --patched-bin "%PATCHED_BIN%" ^
    --header-bin "%HEADER_BIN%" || exit /b 1

"%PYTHON%" "%SCRIPT_DIR%version.py" ^
    --version-json "%VERSION_JSON%" ^
    --patched-bin "%PATCHED_BIN%" ^
    --header-bin "%HEADER_BIN%" || exit /b 1

"%OBJCOPY%" --update-section ".app_header=%HEADER_BIN%" "%ELF_FILE%" "%PATCHED_ELF%" || exit /b 1

echo CRC-patched BIN: "%PATCHED_BIN%"
echo CRC-patched ELF: "%PATCHED_ELF%"
echo Flash BIN with:
echo STM32_Programmer_CLI -c port=SWD -w "%PATCHED_BIN%" 0x08010000

endlocal
