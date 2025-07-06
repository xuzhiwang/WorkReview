#!/bin/bash

# CrossPlatform SDK - Xcode项目生成脚本
# 支持macOS和iOS平台

set -e  # 遇到错误立即退出

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印带颜色的消息
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

# 显示帮助信息
show_help() {
    echo "CrossPlatform SDK - Xcode项目生成脚本"
    echo ""
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  -p, --platform PLATFORM    目标平台 (macos|ios|all) [默认: macos]"
    echo "  -c, --config CONFIG         构建配置 (Debug|Release) [默认: Debug]"
    echo "  -t, --tests                 启用测试构建"
    echo "  -e, --examples              启用示例构建"
    echo "  -b, --build                 生成项目后立即构建"
    echo "  -o, --open                  构建完成后打开Xcode项目"
    echo "  -r, --run                   构建完成后运行示例程序"
    echo "  -d, --device DEVICE         iOS设备类型 (simulator|device) [默认: simulator]"
    echo "  --clean                     清理构建目录"
    echo "  -h, --help                  显示此帮助信息"
    echo ""
    echo "示例:"
    echo "  $0                          # 生成macOS Debug项目"
    echo "  $0 -p ios -c Release -b     # 生成并构建iOS Release项目"
    echo "  $0 -p all -t -e -o          # 生成所有平台项目，包含测试和示例，并打开Xcode"
    echo "  $0 --clean                  # 清理所有构建目录"
}

# 默认参数 - 一键构建配置
PLATFORM="macos"
CONFIG="Release"
BUILD_TESTS="OFF"
BUILD_EXAMPLES="ON"
BUILD_PROJECT="true"
OPEN_XCODE="true"
RUN_EXAMPLE="false"
IOS_DEVICE="simulator"
CLEAN_BUILD="false"

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -p|--platform)
            PLATFORM="$2"
            shift 2
            ;;
        -c|--config)
            CONFIG="$2"
            shift 2
            ;;
        -t|--tests)
            BUILD_TESTS="ON"
            shift
            ;;
        -e|--examples)
            BUILD_EXAMPLES="ON"
            shift
            ;;
        -b|--build)
            BUILD_PROJECT="true"
            shift
            ;;
        -o|--open)
            OPEN_XCODE="true"
            shift
            ;;
        -r|--run)
            RUN_EXAMPLE="true"
            shift
            ;;
        -d|--device)
            IOS_DEVICE="$2"
            shift 2
            ;;
        --clean)
            CLEAN_BUILD="true"
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            print_error "未知参数: $1"
            show_help
            exit 1
            ;;
    esac
done

# 验证参数
if [[ "$PLATFORM" != "macos" && "$PLATFORM" != "ios" && "$PLATFORM" != "all" ]]; then
    print_error "无效的平台: $PLATFORM"
    exit 1
fi

if [[ "$CONFIG" != "Debug" && "$CONFIG" != "Release" ]]; then
    print_error "无效的配置: $CONFIG"
    exit 1
fi

if [[ "$IOS_DEVICE" != "simulator" && "$IOS_DEVICE" != "device" ]]; then
    print_error "无效的iOS设备类型: $IOS_DEVICE"
    exit 1
fi

# 获取脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

print_info "CrossPlatform SDK Xcode项目生成器"
print_info "工作目录: $SCRIPT_DIR"

# 检查必要工具
check_requirements() {
    print_info "检查构建环境..."
    
    if ! command -v cmake &> /dev/null; then
        print_error "CMake未安装，请先安装CMake"
        exit 1
    fi
    
    if ! command -v xcodebuild &> /dev/null; then
        print_error "Xcode未安装，请先安装Xcode"
        exit 1
    fi
    
    CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
    print_info "CMake版本: $CMAKE_VERSION"
    
    XCODE_VERSION=$(xcodebuild -version | head -n1)
    print_info "Xcode版本: $XCODE_VERSION"
}

# 清理构建目录
clean_build_dirs() {
    print_info "清理构建目录..."
    
    rm -rf build-macos
    rm -rf build-ios-simulator
    rm -rf build-ios-device
    rm -rf build
    
    print_success "构建目录已清理"
}

# 生成macOS项目
generate_macos_project() {
    print_info "生成macOS Xcode项目..."

    BUILD_DIR="build-macos"
    print_info "创建构建目录: $BUILD_DIR"
    mkdir -p "$BUILD_DIR"

    print_info "切换到构建目录: $(pwd)/$BUILD_DIR"
    cd "$BUILD_DIR"

    print_info "当前工作目录: $(pwd)"
    print_info "运行CMake配置..."

    cmake .. \
        -G "Xcode" \
        -DCMAKE_BUILD_TYPE="$CONFIG" \
        -DBUILD_TESTS="$BUILD_TESTS" \
        -DBUILD_EXAMPLES="$BUILD_EXAMPLES" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET="10.15" \
        -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DCMAKE_FIND_FRAMEWORK=LAST
    
    CMAKE_EXIT_CODE=$?
    print_info "CMake退出码: $CMAKE_EXIT_CODE"

    if [[ $CMAKE_EXIT_CODE -eq 0 ]]; then
        print_info "CMake配置成功，查找生成的文件..."
        print_info "当前目录内容:"
        ls -la

        # 查找生成的.xcodeproj文件
        XCODE_PROJECT=$(find . -name "*.xcodeproj" -type d | head -n 1)
        print_info "查找到的Xcode项目: '$XCODE_PROJECT'"

        if [[ -n "$XCODE_PROJECT" ]]; then
            print_success "macOS项目生成成功: $BUILD_DIR/$XCODE_PROJECT"
            print_info "所有CMake配置错误已修复"
        else
            print_error "未找到生成的Xcode项目文件"
            print_info "可能的原因: CMake没有使用Xcode生成器"
            print_info "尝试查找其他生成的文件:"
            find . -name "Makefile" -o -name "*.sln" -o -name "build.ninja" | head -5

            # 检查是否生成了Makefile
            if [[ -f "Makefile" ]]; then
                print_warning "检测到Makefile，CMake可能使用了Unix Makefiles生成器"
                print_info "尝试使用make构建..."
                if make -j$(sysctl -n hw.ncpu); then
                    print_success "使用make构建成功"
                    print_info "可执行文件位置:"
                    find . -name "*example*" -type f -executable | head -5
                    return 0
                else
                    print_error "make构建失败"
                fi
            fi

            exit 1
        fi
    else
        print_error "macOS项目生成失败"
        print_error "请检查CMake输出中的错误信息"
        exit 1
    fi
    
    cd ..
}

# 生成iOS项目
generate_ios_project() {
    print_info "生成iOS Xcode项目..."
    
    if [[ "$IOS_DEVICE" == "simulator" ]]; then
        BUILD_DIR="build-ios-simulator"
        IOS_PLATFORM="SIMULATOR64"
        IOS_ARCH="x86_64;arm64"
    else
        BUILD_DIR="build-ios-device"
        IOS_PLATFORM="OS64"
        IOS_ARCH="arm64"
    fi
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # 创建iOS工具链文件
    cat > ios.toolchain.cmake << 'EOF'
set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_SYSTEM_VERSION 14.0)
set(CMAKE_OSX_DEPLOYMENT_TARGET 14.0)

if(PLATFORM STREQUAL "SIMULATOR64")
    set(CMAKE_OSX_SYSROOT iphonesimulator)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
elseif(PLATFORM STREQUAL "OS64")
    set(CMAKE_OSX_SYSROOT iphoneos)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
endif()

set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos;-iphonesimulator")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
EOF
    
    cmake .. \
        -G "Xcode" \
        -DCMAKE_TOOLCHAIN_FILE="ios.toolchain.cmake" \
        -DPLATFORM="$IOS_PLATFORM" \
        -DCMAKE_BUILD_TYPE="$CONFIG" \
        -DBUILD_TESTS="OFF" \
        -DBUILD_EXAMPLES="ON" \
        -DENABLE_BITCODE=OFF
    
    if [[ $? -eq 0 ]]; then
        # 查找生成的.xcodeproj文件
        XCODE_PROJECT=$(find . -name "*.xcodeproj" -type d | head -n 1)
        if [[ -n "$XCODE_PROJECT" ]]; then
            print_success "iOS项目生成成功: $BUILD_DIR/$XCODE_PROJECT"
        else
            print_error "未找到生成的iOS Xcode项目文件"
            exit 1
        fi
    else
        print_error "iOS项目生成失败"
        exit 1
    fi
    
    cd ..
}

# 构建项目
build_project() {
    local build_dir=$1
    local platform_name=$2

    print_info "构建$platform_name项目..."

    cd "$build_dir"

    # 查找.xcodeproj文件
    local xcode_project=$(find . -name "*.xcodeproj" -type d | head -n 1)
    if [[ -z "$xcode_project" ]]; then
        print_error "未找到Xcode项目文件"
        exit 1
    fi

    print_info "使用项目文件: $xcode_project"

    xcodebuild -project "$xcode_project" \
               -scheme ALL_BUILD \
               -configuration "$CONFIG" \
               build
    
    if [[ $? -eq 0 ]]; then
        print_success "$platform_name构建成功"
    else
        print_error "$platform_name构建失败"
        exit 1
    fi
    
    cd ..
}

# 运行示例程序
run_example() {
    local build_dir=$1
    
    print_info "运行示例程序..."
    
    EXAMPLE_PATH="$build_dir/$CONFIG/c_api_example"
    
    if [[ -f "$EXAMPLE_PATH" ]]; then
        print_info "运行C API示例..."
        "$EXAMPLE_PATH"
    else
        print_warning "示例程序未找到: $EXAMPLE_PATH"
    fi
}

# 打开Xcode项目
open_xcode_project() {
    local build_dir=$1
    local platform_name=$2

    print_info "打开$platform_name Xcode项目..."

    cd "$build_dir"
    local xcode_project=$(find . -name "*.xcodeproj" -type d | head -n 1)
    if [[ -n "$xcode_project" ]]; then
        print_info "打开项目: $xcode_project"
        open "$xcode_project"
    else
        print_error "未找到Xcode项目文件"
    fi
    cd ..
}

# 主执行流程
main() {
    check_requirements
    
    if [[ "$CLEAN_BUILD" == "true" ]]; then
        clean_build_dirs
        exit 0
    fi
    
    print_info "构建配置:"
    print_info "  平台: $PLATFORM"
    print_info "  配置: $CONFIG"
    print_info "  测试: $BUILD_TESTS"
    print_info "  示例: $BUILD_EXAMPLES"
    print_info "  iOS设备: $IOS_DEVICE"
    echo ""
    
    # 生成项目
    if [[ "$PLATFORM" == "macos" || "$PLATFORM" == "all" ]]; then
        generate_macos_project
        
        if [[ "$BUILD_PROJECT" == "true" ]]; then
            build_project "build-macos" "macOS"
        fi
        
        if [[ "$RUN_EXAMPLE" == "true" ]]; then
            run_example "build-macos"
        fi
        
        if [[ "$OPEN_XCODE" == "true" ]]; then
            open_xcode_project "build-macos" "macOS"
        fi
    fi
    
    if [[ "$PLATFORM" == "ios" || "$PLATFORM" == "all" ]]; then
        generate_ios_project
        
        if [[ "$BUILD_PROJECT" == "true" ]]; then
            if [[ "$IOS_DEVICE" == "simulator" ]]; then
                build_project "build-ios-simulator" "iOS Simulator"
            else
                build_project "build-ios-device" "iOS Device"
            fi
        fi
        
        if [[ "$OPEN_XCODE" == "true" ]]; then
            if [[ "$IOS_DEVICE" == "simulator" ]]; then
                open_xcode_project "build-ios-simulator" "iOS Simulator"
            else
                open_xcode_project "build-ios-device" "iOS Device"
            fi
        fi
    fi
    
    print_success "所有操作完成！"
    
    # 显示后续步骤
    echo ""
    print_info "后续步骤:"
    if [[ "$PLATFORM" == "macos" || "$PLATFORM" == "all" ]]; then
        echo "  macOS项目: build-macos/CrossPlatformSDK.xcodeproj"
    fi
    if [[ "$PLATFORM" == "ios" || "$PLATFORM" == "all" ]]; then
        if [[ "$IOS_DEVICE" == "simulator" ]]; then
            echo "  iOS项目: build-ios-simulator/CrossPlatformSDK.xcodeproj"
        else
            echo "  iOS项目: build-ios-device/CrossPlatformSDK.xcodeproj"
        fi
    fi
    echo ""
    echo "  手动构建: xcodebuild -project <项目路径> -scheme ALL_BUILD build"
    echo "  运行测试: xcodebuild -project <项目路径> -scheme RUN_TESTS build"
    echo "  打开项目: open <项目路径>"
}

# 执行主函数
main
