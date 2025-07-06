# CrossPlatform SDK Demo

这是一个现代C++跨平台SDK的示例项目，展示了如何构建高质量、可维护的跨平台C++库。

## 🎯 项目特点

### 跨平台支持
- **Windows** (x86/x64)
- **macOS** (x64/ARM64)
- **Linux** (x64/ARM64)
- **iOS** (ARM64/Simulator)
- **Android** (ARM/ARM64/x86/x64)

### 核心功能
- **线程池** - 高性能任务调度和执行
- **HTTP客户端** - 基于libcurl的网络通信
- **日志系统** - 基于spdlog的结构化日志
- **平台抽象层** - 统一的跨平台API

### 技术特性
- **现代C++17** - 使用最新C++特性
- **CMake构建** - 跨平台构建系统
- **单元测试** - 基于Google Test的完整测试
- **内存安全** - AddressSanitizer/ThreadSanitizer支持
- **CI/CD就绪** - GitHub Actions配置

## 🚀 快速开始

### 系统要求

#### 编译器要求
- **GCC** 7.0+ (Linux)
- **Clang** 8.0+ (macOS/Linux)
- **MSVC** 2017+ (Windows)
- **Android NDK** r21+ (Android)
- **Xcode** 11+ (iOS/macOS)

#### 依赖库
- **CMake** 3.16+
- **libcurl** (自动下载)
- **spdlog** (自动下载)
- **Google Test** (自动下载，仅测试时)

### 构建步骤

#### Linux/macOS
```bash
# 克隆项目
git clone <repository-url>
cd sdk-demo

# 创建构建目录
mkdir build && cd build

# 配置项目
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
make -j$(nproc)

# 运行测试
ctest --output-on-failure

# 运行示例
./examples/basic_example
```

#### Windows
```cmd
# 使用Visual Studio Developer Command Prompt
git clone <repository-url>
cd sdk-demo

# 创建构建目录
mkdir build && cd build

# 配置项目
cmake .. -G "Visual Studio 16 2019" -A x64

# 编译
cmake --build . --config Release

# 运行测试
ctest -C Release --output-on-failure

# 运行示例
.\examples\Release\basic_example.exe
```

#### Android
```bash
# 设置Android NDK环境变量
export ANDROID_NDK_ROOT=/path/to/android-ndk

# 配置Android构建
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-21

# 编译
make -j$(nproc)
```

#### iOS
```bash
# 配置iOS构建
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake \
  -DPLATFORM=OS64 \
  -DCMAKE_BUILD_TYPE=Release

# 编译
make -j$(nproc)
```

## 📁 项目结构

```
sdk-demo/
├── CMakeLists.txt              # 主CMake配置
├── README.md                   # 项目说明
├── cmake/                      # CMake模块
│   ├── Dependencies.cmake      # 依赖管理
│   ├── Install.cmake          # 安装配置
│   ├── Package.cmake          # 打包配置
│   └── StaticAnalysis.cmake   # 静态分析
├── include/                    # 公共头文件
│   └── sdk/
│       ├── sdk_core.h         # SDK核心接口
│       ├── threading/         # 线程相关
│       ├── network/           # 网络相关
│       ├── logging/           # 日志相关
│       └── platform/          # 平台抽象
├── src/                       # 源代码实现
│   ├── platform/             # 平台抽象层实现
│   ├── threading/            # 线程池实现
│   ├── network/              # HTTP客户端实现
│   ├── logging/              # 日志系统实现
│   ├── utils/                # 工具类实现
│   └── sdk_core.cpp          # SDK核心实现
├── examples/                  # 示例程序
│   ├── basic_example.cpp     # 基础使用示例
│   ├── thread_pool_example.cpp
│   ├── http_client_example.cpp
│   └── logging_example.cpp
├── tests/                     # 测试代码
│   ├── unit/                 # 单元测试
│   ├── integration/          # 集成测试
│   ├── performance/          # 性能测试
│   └── platform/             # 平台特定测试
└── docs/                     # 文档
    ├── api/                  # API文档
    ├── design/               # 设计文档
    └── examples/             # 使用示例
```

## 🔧 配置选项

### CMake选项
```bash
# 构建选项
-DBUILD_TESTS=ON/OFF           # 构建测试 (默认: ON)
-DBUILD_EXAMPLES=ON/OFF        # 构建示例 (默认: ON)
-DBUILD_BENCHMARKS=ON/OFF      # 构建性能测试 (默认: OFF)

# 调试选项
-DENABLE_ASAN=ON/OFF          # AddressSanitizer (默认: OFF)
-DENABLE_TSAN=ON/OFF          # ThreadSanitizer (默认: OFF)
-DENABLE_COVERAGE=ON/OFF      # 代码覆盖率 (默认: OFF)

# 功能选项
-DENABLE_JSON_SUPPORT=ON/OFF  # JSON支持 (默认: OFF)
-DENABLE_SSL=ON/OFF           # SSL支持 (默认: ON)
```

### 运行时配置
```cpp
sdk::SDKConfig config;
config.thread_pool_size = 8;           // 线程池大小
config.connection_timeout_ms = 5000;   // 连接超时
config.log_level = "debug";            // 日志级别
config.enable_console_log = true;      // 控制台日志
```

## 📚 使用示例

### 基础使用
```cpp
#include <sdk/sdk_core.h>

int main() {
    // 初始化SDK
    sdk::SDKConfig config;
    auto result = sdk::api::init(config);
    
    if (result == sdk::InitResult::SUCCESS) {
        // 使用线程池
        auto thread_pool = sdk::api::threadPool();
        auto future = thread_pool->submit([]() {
            return 42;
        });
        
        // 使用HTTP客户端
        auto http_client = sdk::api::httpClient();
        auto response = http_client->get("https://httpbin.org/get");
        
        // 使用日志系统
        auto logger = sdk::api::logger();
        logger->info("SDK initialized successfully");
        
        // 关闭SDK
        sdk::api::shutdown();
    }
    
    return 0;
}
```

### 线程池使用
```cpp
#include <sdk/threading/thread_pool.h>

// 创建线程池
sdk::ThreadPool pool(4);

// 提交任务
auto future = pool.submit([]() {
    return "Hello from thread pool!";
});

// 带优先级的任务
auto high_priority_future = pool.submit(
    sdk::TaskPriority::HIGH, 
    []() { return 42; }
);

// 等待结果
std::string result = future.get();
int value = high_priority_future.get();
```

### HTTP客户端使用
```cpp
#include <sdk/network/http_client.h>

// 创建HTTP客户端
sdk::HttpClient client;

// GET请求
auto response = client.get("https://api.example.com/data");
if (response.isSuccess()) {
    std::cout << "Response: " << response.getBody() << std::endl;
}

// POST请求
auto post_response = client.post(
    "https://api.example.com/submit",
    "{\"key\": \"value\"}"
);

// 异步请求
auto future = client.getAsync("https://api.example.com/async");
auto async_response = future.get();
```

### 日志系统使用
```cpp
#include <sdk/logging/logger.h>

// 获取日志器
auto logger = sdk::LogManager::getInstance().getLogger("MyApp");

// 基础日志
logger->info("Application started");
logger->warn("This is a warning");
logger->error("An error occurred");

// 格式化日志
logger->info("User {} logged in with ID {}", username, user_id);

// 条件日志
logger->log_if(debug_mode, sdk::LogLevel::DEBUG, "Debug information");
```

## 🧪 测试

### 运行所有测试
```bash
cd build
ctest --output-on-failure
```

### 运行特定测试
```bash
# 单元测试
ctest -L unit

# 集成测试
ctest -L integration

# 性能测试
ctest -L performance

# 平台特定测试
ctest -L platform
```

### 内存检查
```bash
# 使用AddressSanitizer
cmake .. -DENABLE_ASAN=ON
make && ctest

# 使用Valgrind (Linux)
valgrind --tool=memcheck --leak-check=full ./tests/unit_tests
```

## 📊 性能基准

在现代硬件上的典型性能指标：

### 线程池性能
- **任务吞吐量**: >100,000 tasks/second
- **任务延迟**: <1ms (P99)
- **内存开销**: <1MB (4线程池)

### HTTP客户端性能
- **并发连接**: >1,000 connections
- **请求吞吐量**: >10,000 requests/second
- **内存使用**: <50MB (1000并发)

### 日志系统性能
- **日志吞吐量**: >1,000,000 logs/second
- **异步延迟**: <100μs (P99)
- **文件I/O**: >100MB/s

## 🔍 故障排除

### 常见问题

#### 编译错误
```bash
# 确保C++17支持
cmake .. -DCMAKE_CXX_STANDARD=17

# 清理构建缓存
rm -rf build && mkdir build && cd build
```

#### 依赖问题
```bash
# 手动安装libcurl (Ubuntu)
sudo apt-get install libcurl4-openssl-dev

# 手动安装libcurl (macOS)
brew install curl
```

#### 测试失败
```bash
# 详细测试输出
ctest --verbose --output-on-failure

# 单独运行失败的测试
./tests/unit_tests --gtest_filter=ThreadPoolTest.BasicFunctionality
```

## 📄 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 🤝 贡献

欢迎贡献代码！请查看 [CONTRIBUTING.md](CONTRIBUTING.md) 了解贡献指南。

## 📞 支持

如有问题或建议，请：
1. 查看 [FAQ](docs/FAQ.md)
2. 搜索 [Issues](https://github.com/xuzhiwang/WorkReview/issues)
3. 创建新的 Issue
4. 联系维护者：1311783245@qq.com
