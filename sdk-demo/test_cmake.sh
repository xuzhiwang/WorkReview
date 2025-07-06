#!/bin/bash

# 测试CMake配置脚本

set -e

echo "Testing CMake configuration..."

# 检查CMake是否可用
if ! command -v cmake &> /dev/null; then
    echo "CMake not found in current environment"
    echo "This test should be run on a system with CMake installed"
    exit 0
fi

# 获取脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# 清理测试目录
rm -rf build-test
mkdir build-test
cd build-test

echo "Running CMake configuration test..."

# 测试基本配置
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_EXAMPLES=ON -DBUILD_TESTS=OFF

if [ $? -eq 0 ]; then
    echo "✅ CMake configuration successful!"
    echo "Available targets:"
    make help | grep -E "(format|static-analysis|code-quality)" || echo "No code quality targets found"
else
    echo "❌ CMake configuration failed!"
    exit 1
fi

# 清理
cd ..
rm -rf build-test

echo "✅ CMake test completed successfully!"
