@echo off
setlocal
cd /d "%~dp0"

where cmake >nul 2>nul
if errorlevel 1 (
    echo [错误] 未找到 cmake。请先安装 Visual Studio 2022（勾选"使用 C++ 的桌面开发"）或单独安装 CMake。
    exit /b 1
)

echo === 配置 64 位工程 ===
cmake -S . -B build -A x64
if errorlevel 1 (
    echo [错误] 配置失败。请确认已安装 Visual Studio 2022 的 C++ 桌面开发组件。
    exit /b 1
)

echo === 编译 Release ===
cmake --build build --config Release
if errorlevel 1 (
    echo [错误] 编译失败。
    exit /b 1
)

echo.
echo 编译完成：build\Release\TongjianIME.dll
echo 下一步：右键"以管理员身份运行" install.bat
