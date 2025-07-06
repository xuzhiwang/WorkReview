# 构建指南

本文档详细说明了如何在不同平台上构建CrossPlatform SDK。

## 🎯 快速开始

### 一键构建（推荐）

最简单的构建方式：

```bash
# macOS/Linux
./quick_build.sh

# Windows
quick_build.bat
```

这些脚本会自动检测平台并使用最佳的构建配置。

## 🛠️ 平台特定构建

### macOS - Xcode项目

使用 `build_xcode.sh` 脚本生成Xcode项目：

```bash
# 显示帮助
./build_xcode.sh --help

# 基础用法
./build_xcode.sh                    # 生成macOS Debug项目
./build_xcode.sh -c Release         # 生成Release版本
./build_xcode.sh -b                 # 生成后立即构建
./build_xcode.sh -o                 # 构建后打开Xcode

# iOS项目
./build_xcode.sh -p ios             # 生成iOS模拟器项目
./build_xcode.sh -p ios -d device   # 生成iOS设备项目

# 高级选项
./build_xcode.sh -p all -t -e -b -o # 生成所有平台，包含测试和示例，构建并打开

# 清理
./build_xcode.sh --clean            # 清理所有构建目录
```

#### 生成的项目位置：
- macOS: `build-macos/CrossPlatformSDK.xcodeproj`
- iOS模拟器: `build-ios-simulator/CrossPlatformSDK.xcodeproj`
- iOS设备: `build-ios-device/CrossPlatformSDK.xcodeproj`

### Windows - Visual Studio项目

使用 `build_vs.bat` 脚本生成Visual Studio项目：

```cmd
# 显示帮助
build_vs.bat --help

# 基础用法
build_vs.bat                        # 生成x64 Debug项目
build_vs.bat -c Release             # 生成Release版本
build_vs.bat -p x86                 # 生成x86项目
build_vs.bat -b                     # 生成后立即构建
build_vs.bat -o                     # 构建后打开Visual Studio

# 高级选项
build_vs.bat -v 2019 -p ARM64 -t -e # 使用VS2019生成ARM64项目，包含测试

# 清理
build_vs.bat --clean                # 清理构建目录
```

#### 生成的项目位置：
- `build-windows/CrossPlatformSDK.sln`

### Linux - Makefile

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON -DBUILD_TESTS=ON
make -j$(nproc)
```

## 📱 iOS应用开发

### 生成iOS项目

```bash
# iOS模拟器（推荐用于开发）
./build_xcode.sh -p ios -d simulator -o

# iOS设备（需要开发者证书）
./build_xcode.sh -p ios -d device -o
```

### iOS App特性

项目包含一个完整的iOS测试应用 (`examples/ios-app/`)：

- **原生UI界面**: 使用UIKit构建的测试界面
- **SDK集成**: 展示C++ SDK与Objective-C的集成
- **功能测试**: 
  - SDK初始化和配置
  - 线程池任务管理
  - HTTP网络请求
  - 日志系统
- **实时监控**: 实时显示SDK运行状态和日志

### 在Xcode中运行iOS App

1. 打开生成的iOS项目
2. 选择 `SDKTestApp` scheme
3. 选择目标设备（模拟器或真机）
4. 点击运行按钮

## 🔧 构建选项

### CMake选项

```bash
# 基础选项
-DCMAKE_BUILD_TYPE=Debug|Release     # 构建类型
-DBUILD_TESTS=ON|OFF                 # 构建测试
-DBUILD_EXAMPLES=ON|OFF              # 构建示例
-DBUILD_BENCHMARKS=ON|OFF            # 构建性能测试

# 调试选项
-DENABLE_ASAN=ON|OFF                 # AddressSanitizer
-DENABLE_TSAN=ON|OFF                 # ThreadSanitizer
-DENABLE_COVERAGE=ON|OFF             # 代码覆盖率

# 功能选项
-DENABLE_JSON_SUPPORT=ON|OFF         # JSON支持
-DENABLE_SSL=ON|OFF                  # SSL支持
```

### 脚本选项

#### build_xcode.sh 选项

| 选项 | 说明 | 默认值 |
|------|------|--------|
| `-p, --platform` | 目标平台 (macos\|ios\|all) | macos |
| `-c, --config` | 构建配置 (Debug\|Release) | Debug |
| `-d, --device` | iOS设备类型 (simulator\|device) | simulator |
| `-t, --tests` | 启用测试构建 | OFF |
| `-e, --examples` | 启用示例构建 | ON |
| `-b, --build` | 生成后立即构建 | false |
| `-o, --open` | 构建后打开Xcode | false |
| `-r, --run` | 构建后运行示例 | false |
| `--clean` | 清理构建目录 | false |

#### build_vs.bat 选项

| 选项 | 说明 | 默认值 |
|------|------|--------|
| `-p, --platform` | 目标平台 (x86\|x64\|ARM\|ARM64) | x64 |
| `-c, --config` | 构建配置 (Debug\|Release) | Debug |
| `-v, --vs-version` | VS版本 (2019\|2022) | 2022 |
| `-t, --tests` | 启用测试构建 | OFF |
| `-e, --examples` | 启用示例构建 | ON |
| `-b, --build` | 生成后立即构建 | false |
| `-o, --open` | 构建后打开VS | false |
| `-r, --run` | 构建后运行示例 | false |
| `--clean` | 清理构建目录 | false |

## 🚀 使用示例

### 开发工作流

```bash
# 1. 克隆项目
git clone <repository-url>
cd sdk-demo

# 2. 快速构建和测试
./quick_build.sh

# 3. 开发时使用IDE
# macOS
./build_xcode.sh -p macos -t -e -o

# Windows
build_vs.bat -t -e -o

# 4. 测试iOS集成
./build_xcode.sh -p ios -o
```

### CI/CD集成

```bash
# 自动化构建脚本
#!/bin/bash
set -e

# 清理
./build_xcode.sh --clean

# 构建所有平台
./build_xcode.sh -p all -c Release -t -e -b

# 运行测试
cd build-macos && ctest -C Release --output-on-failure
```

## 🔍 故障排除

### 常见问题

1. **CMake版本过低**
   ```bash
   # 升级CMake
   brew install cmake  # macOS
   ```

2. **依赖库下载失败**
   ```bash
   # 清理CMake缓存
   rm -rf build/_deps
   ```

3. **Xcode项目生成失败**
   ```bash
   # 检查Xcode命令行工具
   xcode-select --install
   ```

4. **iOS构建失败**
   ```bash
   # 确保iOS SDK可用
   xcodebuild -showsdks
   ```

### 调试构建

```bash
# 详细输出
cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON

# 调试CMake
cmake .. --debug-output

# 检查生成的项目
cmake .. --trace
```

## 📊 性能优化

### 并行构建

```bash
# macOS
make -j$(sysctl -n hw.ncpu)

# Linux
make -j$(nproc)

# Windows
cmake --build . --parallel
```

### 缓存优化

```bash
# 使用ccache (macOS/Linux)
export CMAKE_CXX_COMPILER_LAUNCHER=ccache
```

## 📝 注意事项

1. **iOS开发**: 需要macOS和Xcode
2. **代码签名**: iOS设备构建需要开发者证书
3. **依赖管理**: 所有依赖库会自动下载
4. **磁盘空间**: 完整构建需要约2GB空间
5. **网络连接**: 首次构建需要下载依赖库

## 🆘 获取帮助

如果遇到构建问题：

1. 查看构建日志中的错误信息
2. 检查系统要求是否满足
3. 尝试清理构建目录重新构建
4. 查看项目Issues或创建新Issue
