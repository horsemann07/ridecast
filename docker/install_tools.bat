@echo off
setlocal EnableExtensions DisableDelayedExpansion

rem ============================================================================
rem File        : install_ridecast_tools.bat
rem Description : Set up WSL 2, Docker Desktop, Python 3.12 and pip on Windows
rem               and add their executable folders to the USER PATH.
rem
rem Requirements: Windows 10/11 x64, internet, winget (App Installer), admin.
rem Notes       : Does NOT force a reboot or launch/install a Linux distro.
rem               Run again after reboot if WSL reported a pending restart.
rem ============================================================================

rem Request administrator privileges when necessary.
fltmc >nul 2>&1
if errorlevel 1 (
    echo [RideCast] Administrator access is required. Requesting permission...
    powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "Start-Process -FilePath '%~f0' -Verb RunAs -Wait"
    exit /b %errorlevel%
)

echo.
echo ============================================================
echo  RideCast Windows setup: WSL 2 + Docker + Python + pip
echo ============================================================
echo.
echo This installs software and enables Windows virtualization features.
echo A reboot and initial Docker Desktop setup may still be required.
echo.

where winget.exe >nul 2>&1
if errorlevel 1 (
    echo [ERROR] winget is unavailable. Install/update Microsoft App Installer.
    echo         https://aka.ms/getwinget
    goto :failed
)

rem ---------------------------------------------------------------------------
rem 1. Enable Windows components required by the WSL 2 backend.
rem ---------------------------------------------------------------------------
echo [1/5] Enabling Windows features for WSL 2...
dism.exe /Online /Enable-Feature /FeatureName:Microsoft-Windows-Subsystem-Linux /All /NoRestart
if errorlevel 1 if not "%errorlevel%"=="3010" (
    echo [ERROR] Could not enable Windows Subsystem for Linux.
    goto :failed
)
dism.exe /Online /Enable-Feature /FeatureName:VirtualMachinePlatform /All /NoRestart
if errorlevel 1 if not "%errorlevel%"=="3010" (
    echo [ERROR] Could not enable Virtual Machine Platform.
    goto :failed
)

rem Docker Desktop creates its own WSL distro; Ubuntu is not required.
echo [2/5] Installing/updating WSL without a separate Linux distribution...
wsl.exe --version >nul 2>&1
if errorlevel 1 (
    wsl.exe --install --no-distribution
    if errorlevel 1 (
        echo [WARN] WSL install could not finish now. A reboot may be required.
    )
) else (
    echo [OK] WSL is already installed.
)
wsl.exe --update
if errorlevel 1 echo [WARN] WSL update was deferred or failed. Retry after restarting Windows.
wsl.exe --set-default-version 2
if errorlevel 1 echo [WARN] Could not set WSL 2 default yet. Retry after restarting Windows.

rem ---------------------------------------------------------------------------
rem 2. Install Docker Desktop and Python with official winget packages.
rem ---------------------------------------------------------------------------
echo.
echo [3/5] Installing Docker Desktop...
set "DOCKER_BIN="
if exist "%ProgramFiles%\Docker\Docker\resources\bin\docker.exe" set "DOCKER_BIN=%ProgramFiles%\Docker\Docker\resources\bin"
if not defined DOCKER_BIN if exist "%LOCALAPPDATA%\Programs\DockerDesktop\resources\bin\docker.exe" set "DOCKER_BIN=%LOCALAPPDATA%\Programs\DockerDesktop\resources\bin"
if not defined DOCKER_BIN (
    winget.exe install --id Docker.DockerDesktop --exact --source winget --accept-source-agreements --accept-package-agreements
    if errorlevel 1 echo [WARN] Docker installer reported an error; checking installed files...
)
if exist "%ProgramFiles%\Docker\Docker\resources\bin\docker.exe" set "DOCKER_BIN=%ProgramFiles%\Docker\Docker\resources\bin"
if not defined DOCKER_BIN if exist "%LOCALAPPDATA%\Programs\DockerDesktop\resources\bin\docker.exe" set "DOCKER_BIN=%LOCALAPPDATA%\Programs\DockerDesktop\resources\bin"
if defined DOCKER_BIN (
    call :AddUserPath "%DOCKER_BIN%"
) else (
    echo [WARN] Docker CLI not found in standard installation locations.
    echo        Check Docker Desktop installation after reboot.
)

echo.
echo [4/5] Installing Python 3.12 with pip...
set "PY_DIR="
if exist "%LOCALAPPDATA%\Programs\Python\Python312\python.exe" set "PY_DIR=%LOCALAPPDATA%\Programs\Python\Python312"
if not defined PY_DIR if exist "%ProgramFiles%\Python312\python.exe" set "PY_DIR=%ProgramFiles%\Python312"
if not defined PY_DIR (
    winget.exe install --id Python.Python.3.12 --exact --source winget --accept-source-agreements --accept-package-agreements
    if errorlevel 1 echo [WARN] Python installer reported an error; checking installed files...
)
if exist "%LOCALAPPDATA%\Programs\Python\Python312\python.exe" set "PY_DIR=%LOCALAPPDATA%\Programs\Python\Python312"
if not defined PY_DIR if exist "%ProgramFiles%\Python312\python.exe" set "PY_DIR=%ProgramFiles%\Python312"
if not defined PY_DIR (
    rem Resolve an existing/custom install using the Python launcher if available.
    for /f "delims=" %%P in ('py -3.12 -c "import os,sys; print(os.path.dirname(sys.executable))" 2^>nul') do set "PY_DIR=%%P"
)
if defined PY_DIR (
    if exist "%PY_DIR%\python.exe" (
        call :AddUserPath "%PY_DIR%"
        call :AddUserPath "%PY_DIR%\Scripts"
        "%PY_DIR%\python.exe" -m ensurepip --upgrade
        if errorlevel 1 echo [WARN] Could not bootstrap pip.
        "%PY_DIR%\python.exe" -m pip --version
        if errorlevel 1 echo [WARN] pip is not yet available.
    ) else (
        echo [WARN] Python executable not located; check the installation.
    )
) else (
    echo [WARN] Python executable not located; check the installation.
)

rem ---------------------------------------------------------------------------
rem 3. Show results and next steps.
rem ---------------------------------------------------------------------------
echo.
echo [5/5] Checking installed command-line tools...
if defined DOCKER_BIN (
    "%DOCKER_BIN%\docker.exe" --version
    "%DOCKER_BIN%\docker.exe" compose version
)
if defined PY_DIR if exist "%PY_DIR%\python.exe" (
    "%PY_DIR%\python.exe" --version
    "%PY_DIR%\python.exe" -m pip --version
)

echo.
echo ============================================================
echo  Installation steps completed. Review any WARN messages.
echo ============================================================
echo.
echo 1. RESTART WINDOWS if WSL/virtualization changes were applied.
echo 2. Open Docker Desktop and finish its first-run setup.
echo 3. Select Linux containers / WSL 2 backend.
echo 4. Open a NEW PowerShell and run:
echo.
echo       wsl --status
echo       docker --version
echo       docker compose version
echo       docker info
echo       python --version
echo       python -m pip --version
echo.
echo 5. From the RideCast repository, run:
echo.
echo       python docker/build.py --image-build
echo       python docker/build.py --build
echo.
echo A working docker CLI does not mean the Docker engine is started.
echo If `docker info` fails, open Docker Desktop and verify its status.
echo.
pause
exit /b 0

rem Add a discovered directory to the CURRENT session and persistent USER PATH.
rem PowerShell is used instead of SETX to avoid truncating existing PATH values.
:AddUserPath
if not exist "%~1" exit /b 0
set "RC_PATH_DIR=%~1"
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$item=$env:RC_PATH_DIR; $old=[Environment]::GetEnvironmentVariable('Path','User'); $parts=@($old -split ';' | Where-Object { $_ -and ($_ -ine $item) }); [Environment]::SetEnvironmentVariable('Path', ((@($item) + $parts) -join ';'), 'User')"
if errorlevel 1 (
    echo [WARN] Could not persist PATH entry: %~1
) else (
    echo [OK] User PATH includes: %~1
)
set "PATH=%~1;%PATH%"
exit /b 0

:failed
echo.
echo [RideCast] Setup stopped. Review the error above.
pause
exit /b 1
