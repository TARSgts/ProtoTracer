@echo off
setlocal EnableDelayedExpansion

set "REPO_DIR=%~dp0.."
pushd "%REPO_DIR%" >nul

set "DISPLAY_MODE="
if /I "%~1"=="extend" (
    set "DISPLAY_MODE=/extend"
) else if /I "%~1"=="mirror" (
    set "DISPLAY_MODE=/clone"
) else if /I "%~1"=="clone" (
    set "DISPLAY_MODE=/clone"
) else if /I "%~1"=="second" (
    set "DISPLAY_MODE=/external"
) else if /I "%~1"=="external" (
    set "DISPLAY_MODE=/external"
) else if /I "%~1"=="primary" (
    set "DISPLAY_MODE=/internal"
)

set "PYTHON_CMD="
where py >nul 2>&1
if %ERRORLEVEL%==0 (
    set "PYTHON_CMD=py -3"
) else (
    where python >nul 2>&1
    if %ERRORLEVEL%==0 (
        set "PYTHON_CMD=python"
    )
)

if not defined PYTHON_CMD (
    echo Python was not found on PATH.
    echo Install Python, then run: python -m pip install -r tools\usb_video_streamer_requirements.txt
    pause
    popd >nul
    exit /b 1
)

if defined DISPLAY_MODE (
    if /I "%DISPLAY_MODE%"=="/extend" (
        echo Ensuring virtual display driver is installed and enabled...
        echo Accept the Administrator prompt if shown.
        powershell -NoProfile -ExecutionPolicy Bypass -File tools\ensure_virtual_display.ps1
        if not "%ERRORLEVEL%"=="0" (
            echo Virtual display setup failed. Cannot continue with extend mode.
            pause
            popd >nul
            exit /b 1
        )
    )
    if /I "%DISPLAY_MODE%"=="/external" (
        echo Ensuring virtual display driver is installed and enabled...
        echo Accept the Administrator prompt if shown.
        powershell -NoProfile -ExecutionPolicy Bypass -File tools\ensure_virtual_display.ps1
        if not "%ERRORLEVEL%"=="0" (
            echo Virtual display setup failed. Cannot continue with second-screen-only mode.
            pause
            popd >nul
            exit /b 1
        )
    )

    echo Applying Windows display mode %DISPLAY_MODE%...
    "%SystemRoot%\System32\DisplaySwitch.exe" %DISPLAY_MODE% 1>nul 2>nul
    timeout /t 3 /nobreak >nul

    if /I "%DISPLAY_MODE%"=="/extend" (
        set "MONITOR_COUNT="
        for /f %%C in ('%PYTHON_CMD% tools\usb_video_streamer.py --list-monitors ^| findstr /R /C:"^  [0-9][0-9]*:" ^| find /C ":"') do set "MONITOR_COUNT=%%C"
        if not defined MONITOR_COUNT set "MONITOR_COUNT=0"
        if !MONITOR_COUNT! LSS 2 (
            echo.
            echo Extend mode requested, but Windows still reports only one active monitor.
            echo Open VDD Control and click Install/Enable, then retry.
            pause
            popd >nul
            exit /b 1
        )
    )
    if /I "%DISPLAY_MODE%"=="/external" (
        set "MONITOR_COUNT="
        for /f %%C in ('%PYTHON_CMD% tools\usb_video_streamer.py --list-monitors ^| findstr /R /C:"^  [0-9][0-9]*:" ^| find /C ":"') do set "MONITOR_COUNT=%%C"
        if not defined MONITOR_COUNT set "MONITOR_COUNT=0"
        if !MONITOR_COUNT! LSS 2 (
            echo.
            echo Second-screen-only requested, but Windows still reports only one active monitor.
            echo Open VDD Control and click Install/Enable, then retry.
            pause
            popd >nul
            exit /b 1
        )
    )
)

set "STREAM_ARGS=--source screen --monitor auto --fps 30"
if /I "%DISPLAY_MODE%"=="/clone" (
    set "STREAM_ARGS=--source screen --monitor 1 --fps 30"
)
if /I "%DISPLAY_MODE%"=="/extend" (
    set "STREAM_ARGS=--source screen --monitor 2 --fps 30"
)
if /I "%DISPLAY_MODE%"=="/external" (
    set "STREAM_ARGS=--source screen --monitor 2 --fps 30"
)

set "USER_ARGS="
set "SKIP_FIRST="
if defined DISPLAY_MODE set "SKIP_FIRST=1"
for %%A in (%*) do (
    if defined SKIP_FIRST (
        set "SKIP_FIRST="
    ) else (
        set "USER_ARGS=!USER_ARGS! %%~A"
    )
)
if defined USER_ARGS set "STREAM_ARGS=!USER_ARGS!"

echo Launching ProtoTracer USB streamer...
echo Args: %STREAM_ARGS%
call %PYTHON_CMD% tools\usb_video_streamer.py %STREAM_ARGS%
set "ERR=%ERRORLEVEL%"

if /I "%DISPLAY_MODE%"=="/extend" (
    echo Restoring primary display mode...
    "%SystemRoot%\System32\DisplaySwitch.exe" /internal 1>nul 2>nul
)
if /I "%DISPLAY_MODE%"=="/external" (
    echo Restoring primary display mode...
    "%SystemRoot%\System32\DisplaySwitch.exe" /internal 1>nul 2>nul
)

if not "%ERR%"=="0" (
    echo.
    echo Streamer exited with error code %ERR%.
    pause
)

popd >nul
exit /b %ERR%
