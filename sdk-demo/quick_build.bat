@echo off
setlocal enabledelayedexpansion

REM 快速构建脚本 - Windows版本

echo [INFO] CrossPlatform SDK 快速构建工具 (Windows)

REM 检查CMake
where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake未安装，请先安装CMake
    pause
    exit /b 1
)

REM 获取脚本目录
set SCRIPT_DIR=%~dp0
cd /d "%SCRIPT_DIR%"

echo [INFO] 开始构建...

REM 创建构建目录
if not exist build mkdir build
cd build

REM 配置项目
echo [INFO] 配置CMake项目...
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON

if errorlevel 1 (
    echo [ERROR] CMake配置失败
    pause
    exit /b 1
)

REM 构建项目
echo [INFO] 构建项目...
cmake --build . --config Release --parallel

if errorlevel 1 (
    echo [ERROR] 构建失败
    pause
    exit /b 1
)

echo [SUCCESS] 构建完成！
echo [INFO] 可执行文件位置: build\examples\Release\
echo [INFO] 运行示例: build\examples\Release\c_api_example.exe

REM 询问是否运行示例
set /p RUN_EXAMPLE="是否运行C API示例? (y/n): "
if /i "%RUN_EXAMPLE%"=="y" (
    echo [INFO] 运行C API示例...
    examples\Release\c_api_example.exe
)

pause
