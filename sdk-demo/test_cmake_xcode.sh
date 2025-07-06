#!/bin/bash

# 测试CMake Xcode生成器

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

print_info "测试CMake Xcode生成器..."

# 检查必要工具
if ! command -v cmake &> /dev/null; then
    print_error "CMake未安装"
    exit 1
fi

# 检查CMake版本
CMAKE_VERSION=$(cmake --version | head -n1)
print_info "CMake版本: $CMAKE_VERSION"

# 检查可用的生成器
print_info "检查可用的CMake生成器..."
cmake --help | grep -A 20 "Generators" | grep -E "(Xcode|Unix Makefiles|Ninja)"

# 清理测试目录
print_info "清理测试目录..."
rm -rf test-cmake-xcode
mkdir test-cmake-xcode
cd test-cmake-xcode

print_info "当前工作目录: $(pwd)"

# 测试1: 默认生成器
print_info "=== 测试1: 默认生成器 ==="
mkdir default
cd default
if cmake ../.. -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON; then
    print_success "默认生成器配置成功"
    ls -la | head -10
    
    # 查找生成的文件
    if find . -name "*.xcodeproj" -type d | head -1; then
        print_success "找到Xcode项目文件"
    elif find . -name "Makefile" | head -1; then
        print_info "生成了Makefile"
    else
        print_warning "未找到预期的构建文件"
    fi
else
    print_error "默认生成器配置失败"
fi
cd ..

# 测试2: 显式指定Xcode生成器
print_info "=== 测试2: 显式Xcode生成器 ==="
mkdir xcode
cd xcode
if cmake ../.. -G "Xcode" -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON; then
    print_success "Xcode生成器配置成功"
    ls -la | head -10
    
    # 查找Xcode项目
    XCODE_PROJECT=$(find . -name "*.xcodeproj" -type d | head -1)
    if [[ -n "$XCODE_PROJECT" ]]; then
        print_success "找到Xcode项目: $XCODE_PROJECT"
        
        # 检查项目内容
        print_info "项目内容:"
        ls -la "$XCODE_PROJECT"
        
        # 测试xcodebuild
        if command -v xcodebuild &> /dev/null; then
            print_info "测试xcodebuild..."
            if xcodebuild -project "$XCODE_PROJECT" -list; then
                print_success "xcodebuild可以读取项目"
            else
                print_warning "xcodebuild无法读取项目"
            fi
        else
            print_warning "xcodebuild未安装"
        fi
    else
        print_error "未找到Xcode项目文件"
        print_info "生成的文件:"
        find . -type f | head -10
    fi
else
    print_error "Xcode生成器配置失败"
fi
cd ..

# 测试3: Unix Makefiles生成器
print_info "=== 测试3: Unix Makefiles生成器 ==="
mkdir makefiles
cd makefiles
if cmake ../.. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON; then
    print_success "Unix Makefiles生成器配置成功"
    
    if [[ -f "Makefile" ]]; then
        print_success "找到Makefile"
    else
        print_error "未找到Makefile"
    fi
else
    print_error "Unix Makefiles生成器配置失败"
fi
cd ..

# 清理
cd ..
rm -rf test-cmake-xcode

print_success "CMake生成器测试完成"

# 给出建议
print_info "建议:"
print_info "1. 如果Xcode生成器工作正常，build_xcode.sh应该可以正常运行"
print_info "2. 如果Xcode生成器失败，可能需要检查Xcode安装"
print_info "3. 可以尝试使用Unix Makefiles作为备选方案"
