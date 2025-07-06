# API参考文档

## 概述

CrossPlatform SDK提供了C语言接口，确保最大的兼容性和易用性。所有接口都是线程安全的，支持跨平台使用。

## 快速开始

### 基本使用流程

```c
#include <sdk/sdk_c_api.h>

int main() {
    // 1. 初始化SDK
    sdk_config_t config = {0};
    config.thread_pool_size = 4;
    strcpy(config.log_level, "info");
    config.enable_console_log = true;

    if (sdk_init(&config) != SDK_INIT_SUCCESS) {
        printf("SDK initialization failed\n");
        return 1;
    }

    // 2. 使用SDK功能
    SDK_LOG_INFO("SDK initialized successfully");

    // 提交任务
    sdk_task_id_t task = sdk_thread_pool_submit_task(
        my_task_function, user_data, SDK_TASK_PRIORITY_NORMAL, NULL);

    // HTTP请求
    sdk_http_response_t response = {0};
    sdk_http_get("https://httpbin.org/get", NULL, &response);
    sdk_http_response_free(&response);

    // 3. 关闭SDK
    sdk_shutdown();
    return 0;
}
```

### 编译和链接

```bash
# Linux/macOS
gcc -o my_app my_app.c -lCrossPlatformSDK -lcurl -pthread

# Windows (MSVC)
cl my_app.c CrossPlatformSDK.lib libcurl.lib

# CMake
target_link_libraries(my_app PRIVATE CrossPlatformSDK)
```

## 核心API

### SDK初始化和管理

#### sdk_init
```c
typedef enum {
    SDK_INIT_SUCCESS = 0,
    SDK_INIT_ALREADY_INITIALIZED = 1,
    SDK_INIT_INVALID_CONFIG = 2,
    SDK_INIT_PLATFORM_ERROR = 3,
    SDK_INIT_DEPENDENCY_ERROR = 4
} sdk_init_result_t;

typedef struct {
    // 线程池配置
    uint32_t thread_pool_size;
    bool enable_hyperthreading;
    
    // HTTP客户端配置
    char user_agent[256];
    uint32_t connection_timeout_ms;
    uint32_t request_timeout_ms;
    uint32_t max_concurrent_requests;
    
    // 日志配置
    char log_level[16];          // "trace", "debug", "info", "warn", "error", "critical"
    char log_file_path[512];
    bool enable_console_log;
    uint64_t max_log_file_size;
    uint32_t max_log_files;
    
    // 其他配置
    bool enable_metrics;
    char metrics_endpoint[512];
} sdk_config_t;

/**
 * 初始化SDK
 * @param config 配置参数，可以为NULL使用默认配置
 * @return 初始化结果
 */
SDK_API sdk_init_result_t sdk_init(const sdk_config_t* config);

/**
 * 关闭SDK，释放所有资源
 */
SDK_API void sdk_shutdown(void);

/**
 * 检查SDK是否已初始化
 * @return true表示已初始化
 */
SDK_API bool sdk_is_initialized(void);

/**
 * 获取SDK版本信息
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 实际写入的字符数
 */
SDK_API uint32_t sdk_get_version(char* buffer, uint32_t buffer_size);
```

## 线程池API

### 任务管理

#### 任务状态和优先级
```c
typedef enum {
    SDK_TASK_PRIORITY_LOW = 0,
    SDK_TASK_PRIORITY_NORMAL = 1,
    SDK_TASK_PRIORITY_HIGH = 2,
    SDK_TASK_PRIORITY_CRITICAL = 3
} sdk_task_priority_t;

typedef enum {
    SDK_TASK_STATUS_PENDING = 0,
    SDK_TASK_STATUS_RUNNING = 1,
    SDK_TASK_STATUS_COMPLETED = 2,
    SDK_TASK_STATUS_FAILED = 3,
    SDK_TASK_STATUS_CANCELLED = 4
} sdk_task_status_t;

typedef struct {
    char id[64];
    sdk_task_priority_t priority;
    sdk_task_status_t status;
    uint64_t submit_time_ms;
    uint64_t start_time_ms;
    uint64_t end_time_ms;
    char error_message[256];
} sdk_task_info_t;

typedef uint64_t sdk_task_id_t;
typedef void (*sdk_task_func_t)(void* user_data);
typedef void (*sdk_task_callback_t)(sdk_task_id_t task_id, sdk_task_status_t status, void* user_data);
```

#### 任务提交和管理
```c
/**
 * 提交任务到线程池
 * @param func 任务函数
 * @param user_data 用户数据
 * @param priority 任务优先级
 * @param callback 完成回调（可选）
 * @return 任务ID，0表示失败
 */
SDK_API sdk_task_id_t sdk_thread_pool_submit_task(
    sdk_task_func_t func,
    void* user_data,
    sdk_task_priority_t priority,
    sdk_task_callback_t callback
);

/**
 * 提交带自定义ID的任务
 * @param task_id 自定义任务ID字符串
 * @param func 任务函数
 * @param user_data 用户数据
 * @param priority 任务优先级
 * @param callback 完成回调（可选）
 * @return 数字任务ID，0表示失败
 */
SDK_API sdk_task_id_t sdk_thread_pool_submit_task_with_id(
    const char* task_id,
    sdk_task_func_t func,
    void* user_data,
    sdk_task_priority_t priority,
    sdk_task_callback_t callback
);

/**
 * 获取任务状态
 * @param task_id 任务ID
 * @return 任务状态
 */
SDK_API sdk_task_status_t sdk_thread_pool_get_task_status(sdk_task_id_t task_id);

/**
 * 获取任务详细信息
 * @param task_id 任务ID
 * @param info 输出任务信息
 * @return true表示成功
 */
SDK_API bool sdk_thread_pool_get_task_info(sdk_task_id_t task_id, sdk_task_info_t* info);

/**
 * 取消任务
 * @param task_id 任务ID
 * @return true表示成功取消
 */
SDK_API bool sdk_thread_pool_cancel_task(sdk_task_id_t task_id);

/**
 * 等待任务完成
 * @param task_id 任务ID
 * @param timeout_ms 超时时间（毫秒），0表示无限等待
 * @return true表示任务完成
 */
SDK_API bool sdk_thread_pool_wait_task(sdk_task_id_t task_id, uint32_t timeout_ms);
```

#### 线程池管理
```c
typedef struct {
    uint32_t thread_count;
    uint32_t active_threads;
    uint32_t pending_tasks;
    uint64_t completed_tasks;
    uint64_t failed_tasks;
    double average_task_duration_ms;
    uint64_t start_time_ms;
} sdk_thread_pool_stats_t;

/**
 * 调整线程池大小
 * @param new_size 新的线程数量
 * @return true表示成功
 */
SDK_API bool sdk_thread_pool_resize(uint32_t new_size);

/**
 * 获取线程池大小
 * @return 当前线程数量
 */
SDK_API uint32_t sdk_thread_pool_get_size(void);

/**
 * 获取活跃线程数
 * @return 活跃线程数量
 */
SDK_API uint32_t sdk_thread_pool_get_active_threads(void);

/**
 * 获取待处理任务数
 * @return 待处理任务数量
 */
SDK_API uint32_t sdk_thread_pool_get_pending_tasks(void);

/**
 * 获取线程池统计信息
 * @param stats 输出统计信息
 * @return true表示成功
 */
SDK_API bool sdk_thread_pool_get_stats(sdk_thread_pool_stats_t* stats);

/**
 * 等待所有任务完成
 * @param timeout_ms 超时时间（毫秒），0表示无限等待
 * @return true表示所有任务完成
 */
SDK_API bool sdk_thread_pool_wait_all(uint32_t timeout_ms);

/**
 * 取消所有待处理任务
 * @return 取消的任务数量
 */
SDK_API uint32_t sdk_thread_pool_cancel_all_pending(void);
```

## HTTP客户端API

### HTTP请求和响应
```c
typedef enum {
    SDK_HTTP_METHOD_GET = 0,
    SDK_HTTP_METHOD_POST = 1,
    SDK_HTTP_METHOD_PUT = 2,
    SDK_HTTP_METHOD_DELETE = 3,
    SDK_HTTP_METHOD_PATCH = 4,
    SDK_HTTP_METHOD_HEAD = 5,
    SDK_HTTP_METHOD_OPTIONS = 6
} sdk_http_method_t;

typedef struct {
    char* data;
    uint32_t size;
    uint32_t capacity;
} sdk_http_buffer_t;

typedef struct {
    char key[256];
    char value[1024];
} sdk_http_header_t;

typedef struct {
    sdk_http_header_t* headers;
    uint32_t count;
    uint32_t capacity;
} sdk_http_headers_t;

typedef struct {
    int status_code;
    sdk_http_headers_t headers;
    sdk_http_buffer_t body;
    uint32_t response_time_ms;
    char error_message[512];
} sdk_http_response_t;

typedef uint64_t sdk_http_request_id_t;
typedef void (*sdk_http_progress_callback_t)(uint64_t downloaded, uint64_t total, void* user_data);
typedef void (*sdk_http_response_callback_t)(sdk_http_request_id_t request_id, const sdk_http_response_t* response, void* user_data);
```

#### 同步HTTP请求
```c
/**
 * 执行GET请求
 * @param url 请求URL
 * @param headers 请求头（可选）
 * @param response 输出响应
 * @return true表示成功
 */
SDK_API bool sdk_http_get(const char* url, const sdk_http_headers_t* headers, sdk_http_response_t* response);

/**
 * 执行POST请求
 * @param url 请求URL
 * @param headers 请求头（可选）
 * @param body 请求体数据
 * @param body_size 请求体大小
 * @param response 输出响应
 * @return true表示成功
 */
SDK_API bool sdk_http_post(const char* url, const sdk_http_headers_t* headers, 
                          const void* body, uint32_t body_size, sdk_http_response_t* response);

/**
 * 执行通用HTTP请求
 * @param method HTTP方法
 * @param url 请求URL
 * @param headers 请求头（可选）
 * @param body 请求体数据（可选）
 * @param body_size 请求体大小
 * @param timeout_ms 超时时间
 * @param response 输出响应
 * @return true表示成功
 */
SDK_API bool sdk_http_request(sdk_http_method_t method, const char* url,
                             const sdk_http_headers_t* headers, const void* body, uint32_t body_size,
                             uint32_t timeout_ms, sdk_http_response_t* response);
```

#### 异步HTTP请求
```c
/**
 * 异步执行HTTP请求
 * @param method HTTP方法
 * @param url 请求URL
 * @param headers 请求头（可选）
 * @param body 请求体数据（可选）
 * @param body_size 请求体大小
 * @param timeout_ms 超时时间
 * @param callback 完成回调
 * @param user_data 用户数据
 * @return 请求ID，0表示失败
 */
SDK_API sdk_http_request_id_t sdk_http_request_async(
    sdk_http_method_t method, const char* url,
    const sdk_http_headers_t* headers, const void* body, uint32_t body_size,
    uint32_t timeout_ms, sdk_http_response_callback_t callback, void* user_data
);

/**
 * 取消异步请求
 * @param request_id 请求ID
 * @return true表示成功取消
 */
SDK_API bool sdk_http_cancel_request(sdk_http_request_id_t request_id);

/**
 * 等待异步请求完成
 * @param request_id 请求ID
 * @param timeout_ms 超时时间
 * @param response 输出响应
 * @return true表示成功
 */
SDK_API bool sdk_http_wait_request(sdk_http_request_id_t request_id, uint32_t timeout_ms, sdk_http_response_t* response);
```

#### 文件上传下载
```c
/**
 * 下载文件
 * @param url 文件URL
 * @param file_path 本地文件路径
 * @param progress_callback 进度回调（可选）
 * @param user_data 用户数据
 * @return 请求ID，0表示失败
 */
SDK_API sdk_http_request_id_t sdk_http_download_file(
    const char* url, const char* file_path,
    sdk_http_progress_callback_t progress_callback, void* user_data
);

/**
 * 上传文件
 * @param url 上传URL
 * @param file_path 本地文件路径
 * @param field_name 表单字段名
 * @param headers 额外请求头（可选）
 * @param progress_callback 进度回调（可选）
 * @param user_data 用户数据
 * @return 请求ID，0表示失败
 */
SDK_API sdk_http_request_id_t sdk_http_upload_file(
    const char* url, const char* file_path, const char* field_name,
    const sdk_http_headers_t* headers,
    sdk_http_progress_callback_t progress_callback, void* user_data
);
```

## 日志API

### 日志级别和配置
```c
typedef enum {
    SDK_LOG_LEVEL_TRACE = 0,
    SDK_LOG_LEVEL_DEBUG = 1,
    SDK_LOG_LEVEL_INFO = 2,
    SDK_LOG_LEVEL_WARN = 3,
    SDK_LOG_LEVEL_ERROR = 4,
    SDK_LOG_LEVEL_CRITICAL = 5,
    SDK_LOG_LEVEL_OFF = 6
} sdk_log_level_t;

/**
 * 设置全局日志级别
 * @param level 日志级别
 */
SDK_API void sdk_log_set_level(sdk_log_level_t level);

/**
 * 获取当前日志级别
 * @return 当前日志级别
 */
SDK_API sdk_log_level_t sdk_log_get_level(void);

/**
 * 检查指定级别是否启用
 * @param level 日志级别
 * @return true表示启用
 */
SDK_API bool sdk_log_is_enabled(sdk_log_level_t level);
```

#### 基础日志函数
```c
/**
 * 写入日志
 * @param level 日志级别
 * @param format 格式字符串
 * @param ... 格式参数
 */
SDK_API void sdk_log(sdk_log_level_t level, const char* format, ...);

/**
 * 写入带上下文的日志
 * @param level 日志级别
 * @param file 源文件名
 * @param line 行号
 * @param func 函数名
 * @param format 格式字符串
 * @param ... 格式参数
 */
SDK_API void sdk_log_with_context(sdk_log_level_t level, const char* file, int line, 
                                 const char* func, const char* format, ...);

// 便利宏
#define SDK_LOG_TRACE(fmt, ...) sdk_log_with_context(SDK_LOG_LEVEL_TRACE, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define SDK_LOG_DEBUG(fmt, ...) sdk_log_with_context(SDK_LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define SDK_LOG_INFO(fmt, ...)  sdk_log_with_context(SDK_LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define SDK_LOG_WARN(fmt, ...)  sdk_log_with_context(SDK_LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define SDK_LOG_ERROR(fmt, ...) sdk_log_with_context(SDK_LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define SDK_LOG_CRITICAL(fmt, ...) sdk_log_with_context(SDK_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
```

## 平台工具API

### 系统信息
```c
typedef enum {
    SDK_PLATFORM_WINDOWS = 0,
    SDK_PLATFORM_MACOS = 1,
    SDK_PLATFORM_IOS = 2,
    SDK_PLATFORM_ANDROID = 3,
    SDK_PLATFORM_LINUX = 4,
    SDK_PLATFORM_UNKNOWN = 5
} sdk_platform_type_t;

typedef enum {
    SDK_ARCH_X86 = 0,
    SDK_ARCH_X64 = 1,
    SDK_ARCH_ARM = 2,
    SDK_ARCH_ARM64 = 3,
    SDK_ARCH_UNKNOWN = 4
} sdk_arch_type_t;

typedef struct {
    sdk_platform_type_t platform;
    sdk_arch_type_t architecture;
    char os_name[128];
    char os_version[64];
    char device_model[128];
    uint64_t total_memory_bytes;
    uint32_t cpu_core_count;
    char cpu_brand[256];
} sdk_system_info_t;

/**
 * 获取系统信息
 * @param info 输出系统信息
 * @return true表示成功
 */
SDK_API bool sdk_get_system_info(sdk_system_info_t* info);

/**
 * 获取当前时间戳（毫秒）
 * @return 时间戳
 */
SDK_API uint64_t sdk_get_current_time_ms(void);

/**
 * 获取可用内存大小
 * @return 可用内存字节数
 */
SDK_API uint64_t sdk_get_available_memory(void);

/**
 * 获取当前进程内存使用量
 * @return 内存使用字节数
 */
SDK_API uint64_t sdk_get_process_memory_usage(void);
```

## 错误处理

### 错误码定义
```c
typedef enum {
    SDK_ERROR_SUCCESS = 0,
    SDK_ERROR_INVALID_PARAMETER = 1,
    SDK_ERROR_NOT_INITIALIZED = 2,
    SDK_ERROR_ALREADY_INITIALIZED = 3,
    SDK_ERROR_OUT_OF_MEMORY = 4,
    SDK_ERROR_TIMEOUT = 5,
    SDK_ERROR_NETWORK_ERROR = 6,
    SDK_ERROR_FILE_ERROR = 7,
    SDK_ERROR_PERMISSION_DENIED = 8,
    SDK_ERROR_NOT_SUPPORTED = 9,
    SDK_ERROR_INTERNAL_ERROR = 10
} sdk_error_code_t;

/**
 * 获取最后一个错误码
 * @return 错误码
 */
SDK_API sdk_error_code_t sdk_get_last_error(void);

/**
 * 获取错误描述
 * @param error_code 错误码
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 实际写入的字符数
 */
SDK_API uint32_t sdk_get_error_string(sdk_error_code_t error_code, char* buffer, uint32_t buffer_size);

/**
 * 设置错误回调函数
 * @param callback 错误回调
 * @param user_data 用户数据
 */
typedef void (*sdk_error_callback_t)(sdk_error_code_t error_code, const char* message, void* user_data);
SDK_API void sdk_set_error_callback(sdk_error_callback_t callback, void* user_data);
```

## 内存管理

### 内存分配函数
```c
/**
 * 分配内存
 * @param size 大小
 * @return 内存指针，失败返回NULL
 */
SDK_API void* sdk_malloc(size_t size);

/**
 * 重新分配内存
 * @param ptr 原内存指针
 * @param size 新大小
 * @return 新内存指针，失败返回NULL
 */
SDK_API void* sdk_realloc(void* ptr, size_t size);

/**
 * 释放内存
 * @param ptr 内存指针
 */
SDK_API void sdk_free(void* ptr);

/**
 * 释放HTTP响应
 * @param response 响应对象
 */
SDK_API void sdk_http_response_free(sdk_http_response_t* response);

/**
 * 释放HTTP头部
 * @param headers 头部对象
 */
SDK_API void sdk_http_headers_free(sdk_http_headers_t* headers);
```
