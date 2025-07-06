# 测试策略和自动化

## 概述

本文档定义了C++跨平台SDK项目的测试策略，重点关注自动化测试和质量保证，基于业界真实案例和最佳实践。

## 测试金字塔策略

### 测试层次分布
```
        /\
       /  \
      / UI \     5% - 端到端测试
     /______\
    /        \
   /Integration\ 15% - 集成测试  
  /_____________\
 /               \
/   Unit Tests    \ 80% - 单元测试
/__________________\
```

### 各层测试目标
- **单元测试**: 验证单个函数/类的正确性
- **集成测试**: 验证模块间交互
- **端到端测试**: 验证完整用户场景

## 跨平台测试挑战和解决方案

### 1. 内存管理差异

#### 已知业界问题案例
**案例1: WhatsApp Android崩溃 (2019)**
- **问题**: 在Android ARM64平台上，内存对齐要求导致崩溃
- **原因**: x86平台开发的代码在ARM平台上内存访问未对齐
- **解决方案**: 
```cpp
// ❌ 可能在ARM平台崩溃的代码
struct NetworkPacket {
    uint8_t type;
    uint32_t length;  // 可能未对齐
    char data[];
};

// ✅ 平台安全的代码
struct NetworkPacket {
    uint8_t type;
    uint8_t padding[3];  // 显式填充
    uint32_t length;     // 确保4字节对齐
    char data[];
} __attribute__((packed));
```

**案例2: Chrome浏览器内存泄漏 (2020)**
- **问题**: Windows平台特有的COM对象释放问题
- **原因**: Linux开发的代码未考虑Windows COM生命周期
- **测试策略**: 
```cpp
// 平台特定的内存测试
#ifdef _WIN32
TEST(MemoryTest, COMObjectLifecycle) {
    // 测试COM对象的正确释放
    auto com_obj = CreateCOMObject();
    EXPECT_EQ(1, com_obj->AddRef());
    EXPECT_EQ(0, com_obj->Release());
}
#endif
```

### 2. 线程模型差异

#### 已知业界问题案例
**案例1: Signal Desktop崩溃 (2021)**
- **问题**: macOS上线程优先级设置导致死锁
- **原因**: Linux线程优先级模型与macOS不同
- **测试方案**:
```cpp
class ThreadPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 跨平台线程池测试
        pool_ = std::make_unique<ThreadPool>(4);
    }
    
    void TestConcurrentExecution() {
        std::atomic<int> counter{0};
        std::vector<std::future<void>> futures;
        
        // 提交大量任务测试并发
        for (int i = 0; i < 1000; ++i) {
            futures.push_back(pool_->submit([&counter]() {
                counter.fetch_add(1);
            }));
        }
        
        // 等待所有任务完成
        for (auto& future : futures) {
            future.wait();
        }
        
        EXPECT_EQ(1000, counter.load());
    }
};
```

**案例2: Discord语音通话问题 (2022)**
- **问题**: Android平台线程调度导致音频延迟
- **原因**: 实时线程优先级在不同Android版本表现不一致
- **测试策略**: 平台特定的性能测试

### 3. 网络行为差异

#### 已知业界问题案例
**案例1: Zoom连接问题 (2020)**
- **问题**: iOS平台网络切换时连接断开
- **原因**: iOS网络状态变化处理与其他平台不同
- **测试方案**:
```cpp
class NetworkTest : public ::testing::Test {
protected:
    void TestNetworkResilience() {
        HttpClient client;
        
        // 模拟网络中断
        client.connect("https://api.example.com");
        EXPECT_TRUE(client.isConnected());
        
        // 模拟网络切换
        SimulateNetworkChange();
        
        // 验证自动重连
        EXPECT_TRUE(client.waitForReconnection(5000));
    }
    
private:
    void SimulateNetworkChange() {
#ifdef __APPLE__
        // iOS/macOS特定的网络切换模拟
        CFNotificationCenterPostNotification(
            CFNotificationCenterGetDarwinNotifyCenter(),
            CFSTR("com.apple.system.config.network_change"),
            NULL, NULL, TRUE);
#elif defined(__ANDROID__)
        // Android网络切换模拟
        // 通过JNI调用Android网络管理器
#endif
    }
};
```

## 自动化测试框架

### 1. 单元测试框架

#### Google Test配置
```cmake
# CMakeLists.txt
find_package(GTest REQUIRED)

# 创建测试可执行文件
add_executable(unit_tests
    tests/thread_pool_test.cpp
    tests/http_client_test.cpp
    tests/logger_test.cpp
)

target_link_libraries(unit_tests
    ${PROJECT_NAME}
    GTest::gtest_main
    GTest::gmock_main
)

# 添加测试发现
include(GoogleTest)
gtest_discover_tests(unit_tests)
```

#### 测试覆盖率配置
```cmake
# 启用代码覆盖率
if(ENABLE_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} --coverage")
endif()
```

### 2. 集成测试框架

#### 多进程测试
```cpp
class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 启动测试服务器
        test_server_ = std::make_unique<TestHttpServer>(8080);
        test_server_->start();
        
        // 等待服务器就绪
        ASSERT_TRUE(waitForServerReady(5000));
    }
    
    void TearDown() override {
        test_server_->stop();
    }
    
private:
    std::unique_ptr<TestHttpServer> test_server_;
};

TEST_F(IntegrationTest, HttpClientServerCommunication) {
    HttpClient client;
    auto response = client.get("http://localhost:8080/api/test");
    
    EXPECT_EQ(200, response.status_code);
    EXPECT_EQ("application/json", response.content_type);
}
```

### 3. 性能测试框架

#### Google Benchmark集成
```cpp
#include <benchmark/benchmark.h>

static void BM_ThreadPoolPerformance(benchmark::State& state) {
    ThreadPool pool(state.range(0));
    
    for (auto _ : state) {
        std::vector<std::future<int>> futures;
        
        // 提交任务
        for (int i = 0; i < 1000; ++i) {
            futures.push_back(pool.submit([]() {
                return 42;
            }));
        }
        
        // 等待完成
        for (auto& future : futures) {
            benchmark::DoNotOptimize(future.get());
        }
    }
    
    state.SetItemsProcessed(state.iterations() * 1000);
}

BENCHMARK(BM_ThreadPoolPerformance)->Range(1, 16);
```

## 平台特定测试策略

### 1. Windows平台测试

#### 特有问题和测试重点
```cpp
class WindowsSpecificTest : public ::testing::Test {
protected:
    void TestWindowsPathHandling() {
        // 测试Windows路径分隔符
        std::string path = "C:\\Users\\test\\file.txt";
        auto normalized = NormalizePath(path);
        EXPECT_TRUE(IsValidPath(normalized));
    }
    
    void TestWindowsThreading() {
        // 测试Windows线程优先级
        std::thread t([]() {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGH);
            // 执行高优先级任务
        });
        t.join();
    }
    
    void TestWindowsNetworking() {
        // 测试Windows Winsock初始化
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        EXPECT_EQ(0, result);
        WSACleanup();
    }
};
```

### 2. Linux平台测试

#### 特有问题和测试重点
```cpp
class LinuxSpecificTest : public ::testing::Test {
protected:
    void TestLinuxSignalHandling() {
        // 测试Linux信号处理
        signal(SIGPIPE, SIG_IGN);  // 忽略SIGPIPE
        
        // 测试网络连接中断处理
        HttpClient client;
        // 模拟服务器关闭连接
        EXPECT_NO_THROW(client.get("http://localhost:8080/test"));
    }
    
    void TestLinuxFileDescriptors() {
        // 测试文件描述符限制
        std::vector<int> fds;
        int fd;
        while ((fd = open("/dev/null", O_RDONLY)) != -1) {
            fds.push_back(fd);
            if (fds.size() > 1000) break;  // 防止无限循环
        }
        
        // 清理文件描述符
        for (int fd : fds) {
            close(fd);
        }
        
        EXPECT_GT(fds.size(), 100);  // 至少应该能打开100个文件
    }
};
```

### 3. macOS/iOS平台测试

#### 特有问题和测试重点
```cpp
class ApplePlatformTest : public ::testing::Test {
protected:
    void TestAppleNetworkFramework() {
        // 测试Apple Network Framework
#ifdef __APPLE__
        // 测试网络可达性
        SCNetworkReachabilityRef reachability = 
            SCNetworkReachabilityCreateWithName(NULL, "www.apple.com");
        
        SCNetworkReachabilityFlags flags;
        Boolean valid = SCNetworkReachabilityGetFlags(reachability, &flags);
        
        EXPECT_TRUE(valid);
        CFRelease(reachability);
#endif
    }
    
    void TestAppleMemoryPressure() {
        // 测试iOS内存压力处理
#ifdef __APPLE__
        dispatch_source_t memorySource = dispatch_source_create(
            DISPATCH_SOURCE_TYPE_MEMORYPRESSURE, 0, 
            DISPATCH_MEMORYPRESSURE_WARN, dispatch_get_main_queue());
        
        dispatch_source_set_event_handler(memorySource, ^{
            // 处理内存压力警告
            NSLog(@"Memory pressure warning received");
        });
        
        dispatch_resume(memorySource);
        // 测试内存压力响应
#endif
    }
};
```

### 4. Android平台测试

#### 特有问题和测试重点
```cpp
class AndroidSpecificTest : public ::testing::Test {
protected:
    void TestAndroidJNI() {
        // 测试JNI调用
#ifdef __ANDROID__
        JavaVM* jvm = GetJavaVM();
        JNIEnv* env = nullptr;
        
        int result = jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
        EXPECT_EQ(JNI_OK, result);
        
        // 测试Java方法调用
        jclass clazz = env->FindClass("java/lang/System");
        EXPECT_NE(nullptr, clazz);
#endif
    }
    
    void TestAndroidPermissions() {
        // 测试Android权限检查
#ifdef __ANDROID__
        // 检查网络权限
        bool hasNetworkPermission = CheckNetworkPermission();
        EXPECT_TRUE(hasNetworkPermission);
        
        // 检查存储权限
        bool hasStoragePermission = CheckStoragePermission();
        EXPECT_TRUE(hasStoragePermission);
#endif
    }
};
```

## 自动化测试报告

### 1. 测试报告生成

#### JUnit XML格式输出
```cmake
# 生成JUnit格式的测试报告
set(GTEST_OUTPUT "xml:${CMAKE_BINARY_DIR}/test_results.xml")
```

#### 覆盖率报告生成
```bash
#!/bin/bash
# scripts/generate_coverage_report.sh

# 运行测试
./unit_tests

# 生成覆盖率数据
lcov --capture --directory . --output-file coverage.info

# 过滤系统文件
lcov --remove coverage.info '/usr/*' --output-file coverage.info
lcov --remove coverage.info '*/tests/*' --output-file coverage.info

# 生成HTML报告
genhtml coverage.info --output-directory coverage_report

echo "Coverage report generated in coverage_report/index.html"
```

### 2. 持续集成报告

#### GitHub Actions集成
```yaml
name: Test Report

on: [push, pull_request]

jobs:
  test:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest, macos-latest]
        
    steps:
      - uses: actions/checkout@v2
      
      - name: Run Tests
        run: |
          mkdir build && cd build
          cmake -DENABLE_COVERAGE=ON ..
          make -j$(nproc)
          ctest --output-on-failure
          
      - name: Generate Coverage Report
        if: matrix.os == 'ubuntu-latest'
        run: |
          ./scripts/generate_coverage_report.sh
          
      - name: Upload Coverage to Codecov
        if: matrix.os == 'ubuntu-latest'
        uses: codecov/codecov-action@v1
        with:
          file: ./coverage.info
          
      - name: Publish Test Results
        uses: EnricoMi/publish-unit-test-result-action@v1
        if: always()
        with:
          files: build/test_results.xml
```

## 质量门禁标准

### 1. 代码覆盖率要求
- **单元测试覆盖率**: ≥ 85%
- **分支覆盖率**: ≥ 80%
- **函数覆盖率**: ≥ 90%

### 2. 性能基准要求
- **内存使用**: 不超过基线的110%
- **CPU使用**: 不超过基线的120%
- **响应时间**: 不超过基线的115%

### 3. 平台兼容性要求
- **所有目标平台**: 测试通过率100%
- **内存泄漏**: 零容忍
- **崩溃率**: < 0.01%

---

> 📊 **持续改进**: 定期评估测试策略效果，根据发现的问题调整测试重点和方法。
