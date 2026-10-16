@echo off
setlocal
cd /d "%~dp0"

net session >nul 2>nul
if errorlevel 1 (
    echo [ERROR] Administrator rights required. Right-click and "Run as administrator".
    exit /b 1
)

set INSTALLDIR=%ProgramFiles%\TongjianIME

echo === Unregistering ===
if exist "%INSTALLDIR%\TongjianIME.dll" (
    regsvr32 /u /s "%INSTALLDIR%\TongjianIME.dll"
)

if exist "%INSTALLDIR%\TongjianIME.dll" del /q "%INSTALLDIR%\TongjianIME.dll"
rmdir "%INSTALLDIR%" >nul 2>nul

echo.
echo Uninstalled. Remove the "Tongjian" keyboard in Settings if still listed.
pause
