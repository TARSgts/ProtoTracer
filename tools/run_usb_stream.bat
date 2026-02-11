@echo off
setlocal

set "REPO_DIR=%~dp0.."
pushd "%REPO_DIR%" >nul

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

set "STREAM_ARGS=--source screen --monitor 1 --fps 30"
if not "%~1"=="" (
    set "STREAM_ARGS=%*"
)

echo Launching ProtoTracer USB streamer...
echo Args: %STREAM_ARGS%
call %PYTHON_CMD% tools\usb_video_streamer.py %STREAM_ARGS%
set "ERR=%ERRORLEVEL%"
if not "%ERR%"=="0" (
    echo.
    echo Streamer exited with error code %ERR%.
    pause
)

popd >nul
exit /b %ERR%
