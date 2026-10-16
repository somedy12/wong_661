@echo off
setlocal
cd /d "%~dp0"

echo ==================================================
echo   Tongjian IME - one-click build and install
echo ==================================================
echo.

rem ---- 1. check CMake / Visual Studio ----
where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] CMake not found.
    echo Install Visual Studio 2022 Community with the
    echo "Desktop development with C++" workload first:
    echo   https://visualstudio.microsoft.com/vs/community/
    pause
    exit /b 1
)

rem ---- 2. check admin ----
net session >nul 2>nul
if errorlevel 1 (
    echo [ERROR] Administrator rights required. Right-click and "Run as administrator".
    pause
    exit /b 1
)

rem ---- 3. build ----
echo [1/3] Configuring x64 project...
cmake -S . -B build -A x64
if errorlevel 1 ( echo [ERROR] configure failed & pause & exit /b 1 )

echo [2/3] Building Release...
cmake --build build --config Release
if errorlevel 1 ( echo [ERROR] build failed & pause & exit /b 1 )

rem ---- 4. install ----
echo [3/3] Installing and registering...
set INSTALLDIR=%ProgramFiles%\TongjianIME
mkdir "%INSTALLDIR%" >nul 2>nul
copy /Y "build\Release\TongjianIME.dll" "%INSTALLDIR%\TongjianIME.dll" >nul
if errorlevel 1 ( echo [ERROR] copy failed & pause & exit /b 1 )

regsvr32 /s "%INSTALLDIR%\TongjianIME.dll"
if errorlevel 1 ( echo [ERROR] registration failed & pause & exit /b 1 )

echo.
echo ==================================================
echo   Installed successfully!
echo   Add it: Settings ^> Time ^& Language ^> Language ^& region
echo           ^> Chinese (Simplified) ^> ... ^> Language options
echo           ^> Keyboards ^> Add a keyboard ^> "Tongjian"
echo   Switch: Win+Space or Ctrl+Shift
echo   If not listed, sign out and back in.
echo ==================================================
pause
