@echo off
setlocal
cd /d "%~dp0"

set DLL=build\Release\TongjianIME.dll
if not exist "%DLL%" (
    echo [ERROR] %DLL% not found. Run build.bat first.
    exit /b 1
)

net session >nul 2>nul
if errorlevel 1 (
    echo [ERROR] Administrator rights required. Right-click and "Run as administrator".
    exit /b 1
)

set INSTALLDIR=%ProgramFiles%\TongjianIME
mkdir "%INSTALLDIR%" >nul 2>nul
copy /Y "%CD%\%DLL%" "%INSTALLDIR%\TongjianIME.dll" >nul
if errorlevel 1 ( echo [ERROR] copy failed & exit /b 1 )

echo === Registering Tongjian IME ===
regsvr32 /s "%INSTALLDIR%\TongjianIME.dll"
if errorlevel 1 ( echo [ERROR] registration failed, code %errorlevel% & exit /b 1 )

echo.
echo Installed! DLL is at %INSTALLDIR%
echo Add it: Settings ^> Time ^& Language ^> Language ^& region
echo         ^> Chinese (Simplified) ^> ... ^> Language options
echo         ^> Keyboards ^> Add a keyboard ^> "Tongjian"
echo Switch: Win+Space or Ctrl+Shift
echo If not listed, sign out and back in.
pause
