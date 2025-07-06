#!/bin/bash

# 简化版构建脚本 - 确保能够成功构建

set -e

# 颜色输出
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
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

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 获取脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

print_info "CrossPlatform SDK 简化构建脚本"

# 检查必要工具
if ! command -v cmake &> /dev/null; then
    print_error "CMake未安装"
    exit 1
fi

# 清理之前的构建
print_info "清理之前的构建..."
rm -rf build-simple CMakeCache.txt CMakeFiles/

# 创建构建目录
print_info "创建构建目录..."
mkdir build-simple
cd build-simple

print_info "当前工作目录: $(pwd)"

# 尝试不同的生成器
print_info "尝试生成项目..."

# 首先尝试Xcode生成器
if command -v xcodebuild &> /dev/null; then
    print_info "尝试Xcode生成器..."
    if cmake .. -G "Xcode" -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON; then
        print_success "Xcode项目生成成功"
        
        # 查找Xcode项目
        XCODE_PROJECT=$(find . -name "*.xcodeproj" -type d | head -1)
        if [[ -n "$XCODE_PROJECT" ]]; then
            print_success "找到Xcode项目: $XCODE_PROJECT"
            
            # 尝试构建
            print_info "尝试构建Xcode项目..."
            if xcodebuild -project "$XCODE_PROJECT" -scheme ALL_BUILD -configuration Release build; then
                print_success "Xcode项目构建成功"
                
                # 查找生成的可执行文件
                print_info "查找生成的可执行文件..."
                find . -name "*example*" -type f | head -5
                
                # 打开Xcode项目
                print_info "打开Xcode项目..."
                open "$XCODE_PROJECT"
                
                print_success "构建完成！Xcode项目已打开"
                exit 0
            else
                print_warning "Xcode项目构建失败，尝试其他方法..."
            fi
        else
            print_warning "未找到Xcode项目文件，尝试其他生成器..."
        fi
    else
        print_warning "Xcode生成器失败，尝试其他生成器..."
    fi
fi

# 清理并尝试Unix Makefiles
print_info "清理并尝试Unix Makefiles生成器..."
cd ..
rm -rf build-simple
mkdir build-simple
cd build-simple

if cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON; then
    print_success "Unix Makefiles项目生成成功"
    
    # 构建项目
    print_info "使用make构建项目..."
    if make -j$(sysctl -n hw.ncpu 2>/dev/null || echo 4); then
        print_success "make构建成功"
        
        # 查找生成的可执行文件
        print_info "查找生成的可执行文件..."
        find . -name "*example*" -type f -executable | head -5
        
        # 尝试运行一个示例
        EXAMPLE=$(find . -name "c_api_example" -type f -executable | head -1)
        if [[ -n "$EXAMPLE" ]]; then
            print_info "尝试运行示例程序: $EXAMPLE"
            if "$EXAMPLE"; then
                print_success "示例程序运行成功"
            else
                print_warning "示例程序运行失败，但构建成功"
            fi
        fi
        
        print_success "构建完成！可执行文件在 build-simple/ 目录中"
        exit 0
    else
        print_error "make构建失败"
    fi
else
    print_error "Unix Makefiles生成器也失败了"
fi

# 最后尝试默认生成器
print_info "尝试默认生成器..."
cd ..
rm -rf build-simple
mkdir build-simple
cd build-simple

if cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON; then
    print_success "默认生成器配置成功"
    
    # 检查生成了什么
    if [[ -f "Makefile" ]]; then
        print_info "生成了Makefile，尝试构建..."
        if make -j$(sysctl -n hw.ncpu 2>/dev/null || echo 4); then
            print_success "默认生成器构建成功"
            
            # 查找生成的可执行文件
            print_info "查找生成的可执行文件..."
            find . -name "*example*" -type f -executable | head -5
            
            print_success "构建完成！"
            exit 0
        fi
    elif find . -name "*.xcodeproj" -type d | head -1; then
        XCODE_PROJECT=$(find . -name "*.xcodeproj" -type d | head -1)
        print_info "生成了Xcode项目: $XCODE_PROJECT"
        open "$XCODE_PROJECT"
        print_success "Xcode项目已打开"
        exit 0
    fi
fi

print_error "所有构建方法都失败了"
print_info "请检查:"
print_info "1. CMake版本是否兼容"
print_info "2. Xcode是否正确安装"
print_info "3. 依赖库是否可用"

exit 1
