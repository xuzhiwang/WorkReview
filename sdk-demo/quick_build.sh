#!/bin/bash

# 快速构建脚本 - 一键生成和构建项目

set -e

# 颜色输出
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# 检测平台
detect_platform() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "linux"
    elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
        echo "windows"
    else
        echo "unknown"
    fi
}

PLATFORM=$(detect_platform)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

print_info "CrossPlatform SDK 快速构建工具"
print_info "检测到平台: $PLATFORM"

case $PLATFORM in
    "macos")
        print_info "在macOS上构建..."
        
        # 检查Xcode
        if ! command -v xcodebuild &> /dev/null; then
            print_warning "Xcode未安装，使用Unix Makefiles生成器"
            
            mkdir -p build
            cd build
            cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON
            make -j$(sysctl -n hw.ncpu)
            
            print_success "构建完成！可执行文件在 build/examples/ 目录"
            print_info "运行示例: ./build/examples/c_api_example"
        else
            print_info "使用Xcode构建..."
            ./build_xcode.sh -c Release -b
            
            print_success "Xcode项目构建完成！"
            print_info "项目位置: build-macos/CrossPlatformSDK.xcodeproj"
            print_info "运行示例: ./build-macos/Release/c_api_example"
        fi
        ;;
        
    "linux")
        print_info "在Linux上构建..."
        
        mkdir -p build
        cd build
        cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON
        make -j$(nproc)
        
        print_success "构建完成！可执行文件在 build/examples/ 目录"
        print_info "运行示例: ./build/examples/c_api_example"
        ;;
        
    "windows")
        print_info "在Windows上构建..."
        
        if command -v cmake &> /dev/null; then
            mkdir -p build
            cd build
            cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON
            cmake --build . --config Release --parallel
            
            print_success "构建完成！可执行文件在 build/examples/Release/ 目录"
            print_info "运行示例: ./build/examples/Release/c_api_example.exe"
        else
            print_warning "请使用 build_vs.bat 脚本在Windows上构建"
        fi
        ;;
        
    *)
        print_warning "未知平台，尝试通用构建..."
        
        mkdir -p build
        cd build
        cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON
        cmake --build . --config Release
        
        print_success "构建完成！"
        ;;
esac

print_success "快速构建完成！"
