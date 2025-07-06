# SDK使用指南

## 概述

本指南详细介绍了如何使用CrossPlatform SDK的各个功能模块，包括完整的代码示例和最佳实践。

## 安装和配置

### 系统要求

- **编译器**: GCC 7+, Clang 8+, MSVC 2017+
- **CMake**: 3.16+
- **依赖库**: libcurl, spdlog (自动下载)

### 集成到项目

#### 方式1: 使用CMake FetchContent

```cmake
include(FetchContent)

FetchContent_Declare(
    CrossPlatformSDK
    GIT_REPOSITORY https://github.com/xuzhiwang/WorkReview.git
    GIT_TAG main
    SOURCE_SUBDIR sdk-demo
)

FetchContent_MakeAvailable(CrossPlatformSDK)

target_link_libraries(your_target PRIVATE CrossPlatformSDK)
```

#### 方式2: 作为子模块

```bash
git submodule add https://github.com/xuzhiwang/WorkReview.git third_party/WorkReview
```

```cmake
add_subdirectory(third_party/WorkReview/sdk-demo)
target_link_libraries(your_target PRIVATE CrossPlatformSDK)
```

## 核心功能使用

### 1. SDK初始化和配置

#### 基础初始化

```c
#include <sdk/sdk_c_api.h>

int main() {
    // 使用默认配置初始化
    if (sdk_init(NULL) != SDK_INIT_SUCCESS) {
        printf("Failed to initialize SDK\n");
        return 1;
    }
    
    // 使用SDK...
    
    sdk_shutdown();
    return 0;
}
```

#### 自定义配置

```c
sdk_config_t config = {0};

// 线程池配置
config.thread_pool_size = 8;              // 8个工作线程
config.enable_hyperthreading = true;      // 启用超线程

// HTTP客户端配置
strcpy(config.user_agent, "MyApp/1.0");
config.connection_timeout_ms = 5000;      // 5秒连接超时
config.request_timeout_ms = 30000;        // 30秒请求超时
config.max_concurrent_requests = 20;      // 最大并发请求数

// 日志配置
strcpy(config.log_level, "debug");        // 日志级别
strcpy(config.log_file_path, "app.log");  // 日志文件路径
config.enable_console_log = true;         // 启用控制台日志
config.max_log_file_size = 50 * 1024 * 1024; // 50MB日志文件大小
config.max_log_files = 10;                // 保留10个日志文件

// 其他配置
config.enable_metrics = true;             // 启用指标收集

sdk_init_result_t result = sdk_init(&config);
if (result != SDK_INIT_SUCCESS) {
    printf("SDK initialization failed: %d\n", result);
    return 1;
}
```

#### 错误处理

```c
void error_callback(sdk_error_code_t error_code, const char* message, void* user_data) {
    printf("SDK Error [%d]: %s\n", error_code, message);
    
    // 根据错误类型进行处理
    switch (error_code) {
        case SDK_ERROR_OUT_OF_MEMORY:
            // 内存不足处理
            break;
        case SDK_ERROR_NETWORK_ERROR:
            // 网络错误处理
            break;
        default:
            break;
    }
}

int main() {
    // 设置错误回调
    sdk_set_error_callback(error_callback, NULL);
    
    // 初始化SDK
    if (sdk_init(NULL) != SDK_INIT_SUCCESS) {
        // 获取详细错误信息
        sdk_error_code_t error = sdk_get_last_error();
        char error_msg[256];
        sdk_get_error_string(error, error_msg, sizeof(error_msg));
        printf("Initialization failed: %s\n", error_msg);
        return 1;
    }
    
    // 正常使用...
    
    sdk_shutdown();
    return 0;
}
```

### 2. 线程池使用

#### 基本任务提交

```c
// 任务函数
void my_task(void* user_data) {
    int* value = (int*)user_data;
    printf("Processing value: %d\n", *value);
    
    // 执行一些工作
    usleep(100000); // 100ms
    
    *value *= 2;
    printf("Result: %d\n", *value);
}

int main() {
    sdk_init(NULL);
    
    int data = 42;
    
    // 提交任务
    sdk_task_id_t task_id = sdk_thread_pool_submit_task(
        my_task,                    // 任务函数
        &data,                      // 用户数据
        SDK_TASK_PRIORITY_NORMAL,   // 优先级
        NULL                        // 完成回调
    );
    
    if (task_id == 0) {
        printf("Failed to submit task\n");
        return 1;
    }
    
    // 等待任务完成
    if (sdk_thread_pool_wait_task(task_id, 5000)) {
        printf("Task completed, result: %d\n", data);
    } else {
        printf("Task timeout\n");
    }
    
    sdk_shutdown();
    return 0;
}
```

#### 任务优先级和回调

```c
void task_callback(sdk_task_id_t task_id, sdk_task_status_t status, void* user_data) {
    const char* task_name = (const char*)user_data;
    
    printf("Task '%s' (ID: %llu) ", task_name, task_id);
    switch (status) {
        case SDK_TASK_STATUS_COMPLETED:
            printf("completed successfully\n");
            break;
        case SDK_TASK_STATUS_FAILED:
            printf("failed\n");
            break;
        case SDK_TASK_STATUS_CANCELLED:
            printf("was cancelled\n");
            break;
        default:
            printf("status unknown\n");
            break;
    }
}

void high_priority_task(void* user_data) {
    printf("High priority task executing\n");
    usleep(50000); // 50ms
}

void low_priority_task(void* user_data) {
    printf("Low priority task executing\n");
    usleep(200000); // 200ms
}

int main() {
    sdk_init(NULL);
    
    // 提交不同优先级的任务
    sdk_task_id_t high_task = sdk_thread_pool_submit_task(
        high_priority_task, NULL, SDK_TASK_PRIORITY_HIGH, 
        task_callback);
    
    sdk_task_id_t low_task = sdk_thread_pool_submit_task(
        low_priority_task, NULL, SDK_TASK_PRIORITY_LOW, 
        task_callback);
    
    // 等待所有任务完成
    sdk_thread_pool_wait_all(10000);
    
    sdk_shutdown();
    return 0;
}
```

#### 任务管理和监控

```c
int main() {
    sdk_init(NULL);
    
    // 获取线程池信息
    printf("Thread pool size: %u\n", sdk_thread_pool_get_size());
    printf("Active threads: %u\n", sdk_thread_pool_get_active_threads());
    printf("Pending tasks: %u\n", sdk_thread_pool_get_pending_tasks());
    
    // 调整线程池大小
    if (sdk_thread_pool_resize(8)) {
        printf("Resized thread pool to 8 threads\n");
    }
    
    // 提交一些任务...
    
    // 获取统计信息
    sdk_thread_pool_stats_t stats;
    if (sdk_thread_pool_get_stats(&stats)) {
        printf("Thread Pool Statistics:\n");
        printf("  Thread count: %u\n", stats.thread_count);
        printf("  Completed tasks: %llu\n", stats.completed_tasks);
        printf("  Failed tasks: %llu\n", stats.failed_tasks);
        printf("  Average duration: %.2f ms\n", stats.average_task_duration_ms);
    }
    
    // 取消所有待处理任务
    uint32_t cancelled = sdk_thread_pool_cancel_all_pending();
    printf("Cancelled %u pending tasks\n", cancelled);
    
    sdk_shutdown();
    return 0;
}
```

### 3. HTTP客户端使用

#### 同步HTTP请求

```c
int main() {
    sdk_init(NULL);
    
    // GET请求
    sdk_http_response_t response = {0};
    bool success = sdk_http_get("https://httpbin.org/get", NULL, &response);
    
    if (success) {
        printf("GET Request Success:\n");
        printf("  Status Code: %d\n", response.status_code);
        printf("  Response Time: %u ms\n", response.response_time_ms);
        printf("  Body Size: %u bytes\n", response.body.size);
        
        if (response.body.data) {
            printf("  Body: %s\n", response.body.data);
        }
        
        // 访问响应头
        for (uint32_t i = 0; i < response.headers.count; i++) {
            printf("  Header: %s = %s\n", 
                   response.headers.headers[i].key,
                   response.headers.headers[i].value);
        }
    } else {
        printf("GET Request Failed: %s\n", response.error_message);
    }
    
    // 清理响应
    sdk_http_response_free(&response);
    
    // POST请求
    const char* json_data = "{\"name\": \"test\", \"value\": 123}";
    sdk_http_response_t post_response = {0};
    
    success = sdk_http_post("https://httpbin.org/post", NULL, 
                           json_data, strlen(json_data), &post_response);
    
    if (success) {
        printf("POST Request Success: %d\n", post_response.status_code);
    }
    
    sdk_http_response_free(&post_response);
    
    sdk_shutdown();
    return 0;
}
```

#### 异步HTTP请求

```c
void http_callback(sdk_http_request_id_t request_id, 
                   const sdk_http_response_t* response, 
                   void* user_data) {
    const char* url = (const char*)user_data;
    
    printf("Async response for %s:\n", url);
    printf("  Request ID: %llu\n", request_id);
    printf("  Status Code: %d\n", response->status_code);
    printf("  Response Time: %u ms\n", response->response_time_ms);
    
    if (response->body.data) {
        printf("  Body Size: %u bytes\n", response->body.size);
    }
    
    if (strlen(response->error_message) > 0) {
        printf("  Error: %s\n", response->error_message);
    }
}

int main() {
    sdk_init(NULL);
    
    // 异步GET请求
    const char* url1 = "https://httpbin.org/delay/1";
    sdk_http_request_id_t request1 = sdk_http_request_async(
        SDK_HTTP_METHOD_GET, url1, NULL, NULL, 0, 5000,
        http_callback, (void*)url1);
    
    // 异步POST请求
    const char* url2 = "https://httpbin.org/post";
    const char* post_data = "{\"async\": true}";
    sdk_http_request_id_t request2 = sdk_http_request_async(
        SDK_HTTP_METHOD_POST, url2, NULL, 
        post_data, strlen(post_data), 5000,
        http_callback, (void*)url2);
    
    if (request1 > 0 && request2 > 0) {
        printf("Submitted async requests: %llu, %llu\n", request1, request2);
        
        // 等待请求完成
        printf("Waiting for responses...\n");
        sleep(3);
    } else {
        printf("Failed to submit async requests\n");
    }
    
    sdk_shutdown();
    return 0;
}
```

### 4. 日志系统使用

#### 基本日志记录

```c
int main() {
    sdk_init(NULL);
    
    // 设置日志级别
    sdk_log_set_level(SDK_LOG_LEVEL_DEBUG);
    
    // 基本日志记录
    sdk_log(SDK_LOG_LEVEL_INFO, "Application started");
    sdk_log(SDK_LOG_LEVEL_WARN, "This is a warning message");
    sdk_log(SDK_LOG_LEVEL_ERROR, "Error occurred: %s", "file not found");
    
    // 使用便利宏（推荐）
    SDK_LOG_INFO("User %s logged in with ID %d", "john", 12345);
    SDK_LOG_DEBUG("Debug information: value = %f", 3.14159);
    SDK_LOG_ERROR("Critical error in function %s at line %d", __func__, __LINE__);
    
    // 条件日志
    bool debug_mode = true;
    if (debug_mode) {
        SDK_LOG_DEBUG("Debug mode is enabled");
    }
    
    sdk_shutdown();
    return 0;
}
```

#### 日志级别控制

```c
int main() {
    sdk_config_t config = {0};
    strcpy(config.log_level, "warn");  // 只记录警告及以上级别
    config.enable_console_log = true;
    strcpy(config.log_file_path, "app.log");
    
    sdk_init(&config);
    
    // 这些日志不会被记录（级别太低）
    SDK_LOG_TRACE("Trace message");
    SDK_LOG_DEBUG("Debug message");
    SDK_LOG_INFO("Info message");
    
    // 这些日志会被记录
    SDK_LOG_WARN("Warning message");
    SDK_LOG_ERROR("Error message");
    SDK_LOG_CRITICAL("Critical message");
    
    // 运行时修改日志级别
    sdk_log_set_level(SDK_LOG_LEVEL_DEBUG);
    SDK_LOG_DEBUG("Now debug messages are visible");
    
    sdk_shutdown();
    return 0;
}
```

## 最佳实践

### 1. 错误处理

```c
// 总是检查返回值
sdk_task_id_t task = sdk_thread_pool_submit_task(my_task, data, priority, callback);
if (task == 0) {
    SDK_LOG_ERROR("Failed to submit task");
    // 处理错误...
}

// 使用错误回调进行全局错误处理
void global_error_handler(sdk_error_code_t error, const char* message, void* user_data) {
    SDK_LOG_ERROR("Global error [%d]: %s", error, message);
    
    // 根据错误类型决定是否需要重试或退出
    if (error == SDK_ERROR_OUT_OF_MEMORY) {
        // 内存不足，可能需要清理资源
        cleanup_resources();
    }
}
```

### 2. 资源管理

```c
int main() {
    // 确保SDK正确初始化
    if (sdk_init(NULL) != SDK_INIT_SUCCESS) {
        return 1;
    }
    
    // 使用RAII模式管理资源
    sdk_http_response_t response = {0};
    
    if (sdk_http_get("https://api.example.com/data", NULL, &response)) {
        // 处理响应...
        process_response(&response);
    }
    
    // 总是清理资源
    sdk_http_response_free(&response);
    
    // 确保SDK正确关闭
    sdk_shutdown();
    return 0;
}
```

### 3. 性能优化

```c
// 合理设置线程池大小
sdk_config_t config = {0};
config.thread_pool_size = std::min(std::thread::hardware_concurrency() * 2, 16u);
config.enable_hyperthreading = true;

// 批量提交任务
void submit_batch_tasks() {
    std::vector<sdk_task_id_t> tasks;
    
    for (int i = 0; i < 100; i++) {
        sdk_task_id_t task = sdk_thread_pool_submit_task(
            process_item, &items[i], SDK_TASK_PRIORITY_NORMAL, NULL);
        if (task > 0) {
            tasks.push_back(task);
        }
    }
    
    // 等待所有任务完成
    sdk_thread_pool_wait_all(30000); // 30秒超时
}

// 复用HTTP连接
void make_multiple_requests() {
    // SDK内部会自动复用连接
    for (int i = 0; i < 10; i++) {
        sdk_http_response_t response = {0};
        char url[256];
        snprintf(url, sizeof(url), "https://api.example.com/item/%d", i);
        
        if (sdk_http_get(url, NULL, &response)) {
            process_response(&response);
        }
        
        sdk_http_response_free(&response);
    }
}
```

### 4. 调试和监控

```c
void monitor_sdk_status() {
    // 监控线程池状态
    sdk_thread_pool_stats_t stats;
    if (sdk_thread_pool_get_stats(&stats)) {
        SDK_LOG_INFO("Thread pool stats: %u threads, %llu completed, %.2f ms avg",
                     stats.thread_count, stats.completed_tasks, 
                     stats.average_task_duration_ms);
        
        // 检查是否有任务堆积
        if (stats.pending_tasks > 100) {
            SDK_LOG_WARN("High task queue: %u pending tasks", stats.pending_tasks);
        }
    }
    
    // 监控内存使用
    uint64_t memory_usage = sdk_get_process_memory_usage();
    SDK_LOG_DEBUG("Current memory usage: %llu MB", memory_usage / 1024 / 1024);
}
```

## 故障排除

### 常见问题

1. **SDK初始化失败**
   - 检查依赖库是否正确安装
   - 确认配置参数有效性
   - 查看错误日志获取详细信息

2. **任务执行失败**
   - 检查任务函数是否正确实现
   - 确认用户数据指针有效性
   - 监控线程池状态

3. **HTTP请求超时**
   - 调整超时设置
   - 检查网络连接
   - 验证目标服务器状态

4. **内存泄漏**
   - 确保调用相应的free函数
   - 检查用户数据的生命周期
   - 使用内存检测工具

### 调试技巧

```c
// 启用详细日志
sdk_log_set_level(SDK_LOG_LEVEL_TRACE);

// 使用错误回调获取详细错误信息
void debug_error_callback(sdk_error_code_t error, const char* message, void* user_data) {
    printf("DEBUG: Error %d - %s\n", error, message);
    
    // 打印调用栈（如果可用）
    print_stack_trace();
}

// 监控资源使用
void debug_monitor() {
    static uint64_t last_memory = 0;
    uint64_t current_memory = sdk_get_process_memory_usage();
    
    if (current_memory > last_memory + 1024*1024) { // 增长超过1MB
        SDK_LOG_WARN("Memory usage increased by %llu bytes", 
                     current_memory - last_memory);
    }
    
    last_memory = current_memory;
}
```
