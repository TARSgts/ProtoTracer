@echo off
setlocal

for %%I in ("%~dp0..") do set "REPO_DIR=%%~fI"
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
    popd >nul
    exit /b 1
)

echo Installing build/runtime dependencies...
call %PYTHON_CMD% -m pip install --upgrade pip >nul
if not "%ERRORLEVEL%"=="0" (
    echo Failed to update pip.
    popd >nul
    exit /b 1
)

call %PYTHON_CMD% -m pip install -r tools\usb_video_streamer_requirements.txt pyinstaller
if not "%ERRORLEVEL%"=="0" (
    echo Failed to install dependencies.
    popd >nul
    exit /b 1
)

if not exist "tools\dist" mkdir "tools\dist"
if not exist "tools\build\pyinstaller" mkdir "tools\build\pyinstaller"

echo Building ProtoTracerUSBMirror.exe...
call %PYTHON_CMD% -m PyInstaller ^
    --noconfirm ^
    --clean ^
    --onefile ^
    --name ProtoTracerUSBMirror ^
    --distpath tools\dist ^
    --workpath tools\build\pyinstaller ^
    --specpath tools\build\pyinstaller ^
    tools\usb_mirror_app.py

if not "%ERRORLEVEL%"=="0" (
    echo Build failed.
    popd >nul
    exit /b 1
)

echo.
echo Done.
echo EXE: %REPO_DIR%\tools\dist\ProtoTracerUSBMirror.exe

popd >nul
exit /b 0
