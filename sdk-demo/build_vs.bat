@echo off
setlocal enabledelayedexpansion

REM CrossPlatform SDK - Visual Studio项目生成脚本
REM 支持Windows平台多种架构

REM 设置颜色输出（Windows 10+）
for /f %%A in ('"prompt $H &echo on &for %%B in (1) do rem"') do set BS=%%A

REM 默认参数
set PLATFORM=x64
set CONFIG=Debug
set VS_VERSION=2022
set BUILD_TESTS=OFF
set BUILD_EXAMPLES=ON
set BUILD_PROJECT=false
set OPEN_VS=false
set RUN_EXAMPLE=false
set CLEAN_BUILD=false

REM 解析命令行参数
:parse_args
if "%~1"=="" goto :args_done
if "%~1"=="-p" (
    set PLATFORM=%~2
    shift
    shift
    goto :parse_args
)
if "%~1"=="--platform" (
    set PLATFORM=%~2
    shift
    shift
    goto :parse_args
)
if "%~1"=="-c" (
    set CONFIG=%~2
    shift
    shift
    goto :parse_args
)
if "%~1"=="--config" (
    set CONFIG=%~2
    shift
    shift
    goto :parse_args
)
if "%~1"=="-v" (
    set VS_VERSION=%~2
    shift
    shift
    goto :parse_args
)
if "%~1"=="--vs-version" (
    set VS_VERSION=%~2
    shift
    shift
    goto :parse_args
)
if "%~1"=="-t" (
    set BUILD_TESTS=ON
    shift
    goto :parse_args
)
if "%~1"=="--tests" (
    set BUILD_TESTS=ON
    shift
    goto :parse_args
)
if "%~1"=="-e" (
    set BUILD_EXAMPLES=ON
    shift
    goto :parse_args
)
if "%~1"=="--examples" (
    set BUILD_EXAMPLES=ON
    shift
    goto :parse_args
)
if "%~1"=="-b" (
    set BUILD_PROJECT=true
    shift
    goto :parse_args
)
if "%~1"=="--build" (
    set BUILD_PROJECT=true
    shift
    goto :parse_args
)
if "%~1"=="-o" (
    set OPEN_VS=true
    shift
    goto :parse_args
)
if "%~1"=="--open" (
    set OPEN_VS=true
    shift
    goto :parse_args
)
if "%~1"=="-r" (
    set RUN_EXAMPLE=true
    shift
    goto :parse_args
)
if "%~1"=="--run" (
    set RUN_EXAMPLE=true
    shift
    goto :parse_args
)
if "%~1"=="--clean" (
    set CLEAN_BUILD=true
    shift
    goto :parse_args
)
if "%~1"=="-h" goto :show_help
if "%~1"=="--help" goto :show_help

echo [ERROR] 未知参数: %~1
goto :show_help

:args_done

REM 显示帮助信息
:show_help
echo CrossPlatform SDK - Visual Studio项目生成脚本
echo.
echo 用法: %~nx0 [选项]
echo.
echo 选项:
echo   -p, --platform PLATFORM    目标平台 (x86^|x64^|ARM^|ARM64) [默认: x64]
echo   -c, --config CONFIG         构建配置 (Debug^|Release) [默认: Debug]
echo   -v, --vs-version VERSION    VS版本 (2019^|2022) [默认: 2022]
echo   -t, --tests                 启用测试构建
echo   -e, --examples              启用示例构建
echo   -b, --build                 生成项目后立即构建
echo   -o, --open                  构建完成后打开Visual Studio项目
echo   -r, --run                   构建完成后运行示例程序
echo   --clean                     清理构建目录
echo   -h, --help                  显示此帮助信息
echo.
echo 示例:
echo   %~nx0                       # 生成x64 Debug项目
echo   %~nx0 -p x86 -c Release -b  # 生成并构建x86 Release项目
echo   %~nx0 -t -e -o              # 生成项目，包含测试和示例，并打开VS
echo   %~nx0 --clean               # 清理所有构建目录
if "%~1"=="-h" exit /b 0
if "%~1"=="--help" exit /b 0
exit /b 1

REM 验证参数
if not "%PLATFORM%"=="x86" if not "%PLATFORM%"=="x64" if not "%PLATFORM%"=="ARM" if not "%PLATFORM%"=="ARM64" (
    echo [ERROR] 无效的平台: %PLATFORM%
    exit /b 1
)

if not "%CONFIG%"=="Debug" if not "%CONFIG%"=="Release" (
    echo [ERROR] 无效的配置: %CONFIG%
    exit /b 1
)

if not "%VS_VERSION%"=="2019" if not "%VS_VERSION%"=="2022" (
    echo [ERROR] 无效的VS版本: %VS_VERSION%
    exit /b 1
)

REM 获取脚本目录
set SCRIPT_DIR=%~dp0
cd /d "%SCRIPT_DIR%"

echo [INFO] CrossPlatform SDK Visual Studio项目生成器
echo [INFO] 工作目录: %SCRIPT_DIR%

REM 检查必要工具
echo [INFO] 检查构建环境...

where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake未安装，请先安装CMake
    exit /b 1
)

REM 检查Visual Studio
if "%VS_VERSION%"=="2022" (
    set VS_GENERATOR=Visual Studio 17 2022
    set VS_PATH=%ProgramFiles%\Microsoft Visual Studio\2022
) else (
    set VS_GENERATOR=Visual Studio 16 2019
    set VS_PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\2019
)

if not exist "%VS_PATH%" (
    echo [ERROR] Visual Studio %VS_VERSION% 未安装
    exit /b 1
)

for /f "delims=" %%i in ('cmake --version') do (
    echo [INFO] CMake版本: %%i
    goto :cmake_version_done
)
:cmake_version_done

echo [INFO] Visual Studio版本: %VS_VERSION%

REM 清理构建目录
if "%CLEAN_BUILD%"=="true" (
    echo [INFO] 清理构建目录...
    if exist build-windows rmdir /s /q build-windows
    if exist build rmdir /s /q build
    echo [SUCCESS] 构建目录已清理
    exit /b 0
)

REM 显示构建配置
echo [INFO] 构建配置:
echo [INFO]   平台: %PLATFORM%
echo [INFO]   配置: %CONFIG%
echo [INFO]   VS版本: %VS_VERSION%
echo [INFO]   测试: %BUILD_TESTS%
echo [INFO]   示例: %BUILD_EXAMPLES%
echo.

REM 生成Visual Studio项目
echo [INFO] 生成Visual Studio项目...

set BUILD_DIR=build-windows
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

cd "%BUILD_DIR%"

REM 设置平台架构
if "%PLATFORM%"=="x86" (
    set CMAKE_ARCH=Win32
) else (
    set CMAKE_ARCH=%PLATFORM%
)

cmake .. ^
    -G "%VS_GENERATOR%" ^
    -A %CMAKE_ARCH% ^
    -DCMAKE_BUILD_TYPE=%CONFIG% ^
    -DBUILD_TESTS=%BUILD_TESTS% ^
    -DBUILD_EXAMPLES=%BUILD_EXAMPLES% ^
    -DCMAKE_INSTALL_PREFIX=install

if errorlevel 1 (
    echo [ERROR] Visual Studio项目生成失败
    exit /b 1
)

echo [SUCCESS] Visual Studio项目生成成功: %BUILD_DIR%\CrossPlatformSDK.sln

cd ..

REM 构建项目
if "%BUILD_PROJECT%"=="true" (
    echo [INFO] 构建项目...
    
    cd "%BUILD_DIR%"
    
    cmake --build . --config %CONFIG% --parallel
    
    if errorlevel 1 (
        echo [ERROR] 项目构建失败
        exit /b 1
    )
    
    echo [SUCCESS] 项目构建成功
    
    cd ..
)

REM 运行示例程序
if "%RUN_EXAMPLE%"=="true" (
    echo [INFO] 运行示例程序...
    
    set EXAMPLE_PATH=%BUILD_DIR%\examples\%CONFIG%\c_api_example.exe
    
    if exist "!EXAMPLE_PATH!" (
        echo [INFO] 运行C API示例...
        "!EXAMPLE_PATH!"
    ) else (
        echo [WARNING] 示例程序未找到: !EXAMPLE_PATH!
    )
)

REM 打开Visual Studio项目
if "%OPEN_VS%"=="true" (
    echo [INFO] 打开Visual Studio项目...
    start "" "%BUILD_DIR%\CrossPlatformSDK.sln"
)

echo [SUCCESS] 所有操作完成！
echo.
echo [INFO] 后续步骤:
echo   Visual Studio项目: %BUILD_DIR%\CrossPlatformSDK.sln
echo.
echo   手动构建: cmake --build %BUILD_DIR% --config %CONFIG%
echo   运行测试: ctest --test-dir %BUILD_DIR% -C %CONFIG%
echo   安装: cmake --install %BUILD_DIR% --config %CONFIG%
echo   打开项目: start %BUILD_DIR%\CrossPlatformSDK.sln

exit /b 0
