@echo off
setlocal enabledelayedexpansion

:: Define the absolute path to your PlatformIO executable
set "PIO_PATH=C:\Users\aragasa\.platformio\penv\Scripts\pio.exe"

:: Initialize variables to track flags
set "DO_CLEAN=0"
set "DO_ERASE=0"
set "DO_UPLOAD=0"
set "DO_MONITOR=0"

:: Loop through all provided arguments
:parse_args
if "%~1"=="" goto run_commands
if /I "%~1"=="c" set "DO_CLEAN=1"
if /I "%~1"=="e" set "DO_ERASE=1"
if /I "%~1"=="u" set "DO_UPLOAD=1"
if /I "%~1"=="m" set "DO_MONITOR=1"
shift
goto parse_args

:run_commands
:: 1. Clean goes first if requested
if "!DO_CLEAN!"=="1" (
    echo [*] Cleaning project...
    "%PIO_PATH%" run -t clean
    if !errorlevel! neq 0 goto error_exit
)

:: 2. Erase goes second if requested
if "!DO_ERASE!"=="1" (
    echo [*] Erasing flash...
    "%PIO_PATH%" run -t erase
    if !errorlevel! neq 0 goto error_exit
)

:: 3. Decide to Build OR Upload
if "!DO_UPLOAD!"=="1" (
    echo [*] Building and Uploading firmware...
    "%PIO_PATH%" run -t upload
    if !errorlevel! neq 0 goto error_exit
) else (
    echo [*] Building project only...
    "%PIO_PATH%" run
    if !errorlevel! neq 0 goto error_exit
)

:: 4. Monitor goes last if requested
if "!DO_MONITOR!"=="1" (
    echo [*] Starting Serial Monitor...
    "%PIO_PATH%" device monitor