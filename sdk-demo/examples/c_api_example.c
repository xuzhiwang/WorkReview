#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sdk/sdk_c_api.h>

// 任务函数示例
void simple_task(void* user_data) {
    int* value = (int*)user_data;
    printf("Task executing with value: %d\n", *value);
    
    // 模拟一些工作
    usleep(100000); // 100ms
    
    *value *= 2;
    printf("Task completed, new value: %d\n", *value);
}

void long_running_task(void* user_data) {
    const char* name = (const char*)user_data;
    printf("Long running task '%s' started\n", name);
    
    for (int i = 0; i < 10; i++) {
        printf("Task '%s' progress: %d/10\n", name, i + 1);
        usleep(200000); // 200ms
    }
    
    printf("Long running task '%s' completed\n", name);
}

// 任务完成回调
void task_completion_callback(sdk_task_id_t task_id, sdk_task_status_t status, void* user_data) {
    const char* task_name = (const char*)user_data;
    
    printf("Task callback: %s (ID: %llu) ", task_name, task_id);
    
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

// HTTP响应回调
void http_response_callback(sdk_http_request_id_t request_id, const sdk_http_response_t* response, void* user_data) {
    const char* url = (const char*)user_data;
    
    printf("HTTP Response for %s (ID: %llu):\n", url, request_id);
    printf("  Status Code: %d\n", response->status_code);
    printf("  Response Time: %u ms\n", response->response_time_ms);
    
    if (response->body.data && response->body.size > 0) {
        printf("  Body Size: %u bytes\n", response->body.size);
        printf("  Body Preview: %.100s%s\n", 
               response->body.data, 
               response->body.size > 100 ? "..." : "");
    }
    
    if (strlen(response->error_message) > 0) {
        printf("  Error: %s\n", response->error_message);
    }
}

// 错误回调
void error_callback(sdk_error_code_t error_code, const char* message, void* user_data) {
    printf("SDK Error [%d]: %s\n", error_code, message);
}

int main() {
    printf("=== CrossPlatform SDK C API Example ===\n\n");
    
    // 1. 设置错误回调
    sdk_set_error_callback(error_callback, NULL);
    
    // 2. 初始化SDK
    printf("1. Initializing SDK...\n");
    
    sdk_config_t config = {0};
    config.thread_pool_size = 4;
    config.enable_hyperthreading = true;
    strcpy(config.user_agent, "C-API-Example/1.0");
    config.connection_timeout_ms = 5000;
    config.request_timeout_ms = 30000;
    config.max_concurrent_requests = 10;
    strcpy(config.log_level, "info");
    strcpy(config.log_file_path, "sdk_example.log");
    config.enable_console_log = true;
    config.max_log_file_size = 10 * 1024 * 1024; // 10MB
    config.max_log_files = 5;
    config.enable_metrics = true;
    
    sdk_init_result_t init_result = sdk_init(&config);
    
    switch (init_result) {
        case SDK_INIT_SUCCESS:
            printf("  SDK initialized successfully!\n");
            break;
        case SDK_INIT_ALREADY_INITIALIZED:
            printf("  SDK already initialized\n");
            break;
        default:
            printf("  SDK initialization failed: %d\n", init_result);
            return 1;
    }
    
    // 3. 获取SDK信息
    printf("\n2. SDK Information:\n");
    
    char version_buffer[256];
    uint32_t version_len = sdk_get_version(version_buffer, sizeof(version_buffer));
    printf("  Version: %s\n", version_buffer);
    
    char platform_buffer[512];
    uint32_t platform_len = sdk_get_platform_info(platform_buffer, sizeof(platform_buffer));
    printf("  Platform: %s\n", platform_buffer);
    
    printf("  Initialized: %s\n", sdk_is_initialized() ? "Yes" : "No");
    
    // 4. 测试日志功能
    printf("\n3. Testing Logging:\n");
    
    sdk_log_set_level(SDK_LOG_LEVEL_DEBUG);
    
    SDK_LOG_INFO("This is an info message from C API");
    SDK_LOG_WARN("This is a warning message");
    SDK_LOG_ERROR("This is an error message");
    SDK_LOG_DEBUG("This is a debug message");
    
    printf("  Log messages sent\n");
    
    // 5. 测试线程池
    printf("\n4. Testing Thread Pool:\n");
    
    // 获取线程池信息
    printf("  Thread pool size: %u\n", sdk_thread_pool_get_size());
    printf("  Active threads: %u\n", sdk_thread_pool_get_active_threads());
    printf("  Pending tasks: %u\n", sdk_thread_pool_get_pending_tasks());
    
    // 提交简单任务
    int task_data1 = 10;
    int task_data2 = 20;
    int task_data3 = 30;
    
    sdk_task_id_t task1 = sdk_thread_pool_submit_task(
        simple_task, &task_data1, SDK_TASK_PRIORITY_NORMAL, NULL);
    
    sdk_task_id_t task2 = sdk_thread_pool_submit_task(
        simple_task, &task_data2, SDK_TASK_PRIORITY_HIGH, NULL);
    
    sdk_task_id_t task3 = sdk_thread_pool_submit_task_with_id(
        "custom_task", simple_task, &task_data3, SDK_TASK_PRIORITY_LOW, 
        task_completion_callback);
    
    printf("  Submitted tasks: %llu, %llu, %llu\n", task1, task2, task3);
    
    // 等待任务完成
    printf("  Waiting for tasks to complete...\n");
    sdk_thread_pool_wait_task(task1, 5000);
    sdk_thread_pool_wait_task(task2, 5000);
    sdk_thread_pool_wait_task(task3, 5000);
    
    printf("  Task results: %d, %d, %d\n", task_data1, task_data2, task_data3);
    
    // 提交长时间运行的任务并取消
    printf("  Testing task cancellation...\n");
    
    const char* long_task_name = "cancellable_task";
    sdk_task_id_t long_task = sdk_thread_pool_submit_task(
        long_running_task, (void*)long_task_name, SDK_TASK_PRIORITY_NORMAL, 
        task_completion_callback);
    
    printf("  Submitted long running task: %llu\n", long_task);
    
    // 等待一会儿然后取消
    usleep(500000); // 500ms
    
    bool cancelled = sdk_thread_pool_cancel_task(long_task);
    printf("  Task cancellation %s\n", cancelled ? "succeeded" : "failed");
    
    // 获取线程池统计信息
    sdk_thread_pool_stats_t stats;
    if (sdk_thread_pool_get_stats(&stats)) {
        printf("  Thread Pool Stats:\n");
        printf("    Thread count: %u\n", stats.thread_count);
        printf("    Active threads: %u\n", stats.active_threads);
        printf("    Pending tasks: %u\n", stats.pending_tasks);
        printf("    Completed tasks: %llu\n", stats.completed_tasks);
        printf("    Failed tasks: %llu\n", stats.failed_tasks);
        printf("    Average task duration: %.2f ms\n", stats.average_task_duration_ms);
    }
    
    // 6. 测试HTTP客户端
    printf("\n5. Testing HTTP Client:\n");
    
    // 同步GET请求
    printf("  Performing synchronous GET request...\n");
    
    sdk_http_response_t response = {0};
    bool success = sdk_http_get("https://httpbin.org/get", NULL, &response);
    
    if (success) {
        printf("  GET request successful:\n");
        printf("    Status Code: %d\n", response.status_code);
        printf("    Response Time: %u ms\n", response.response_time_ms);
        printf("    Body Size: %u bytes\n", response.body.size);
        
        if (response.body.data) {
            printf("    Body Preview: %.200s%s\n", 
                   response.body.data, 
                   response.body.size > 200 ? "..." : "");
        }
    } else {
        printf("  GET request failed: %s\n", response.error_message);
    }
    
    // 清理响应
    sdk_http_response_free(&response);
    
    // 异步POST请求
    printf("  Performing asynchronous POST request...\n");
    
    const char* post_data = "{\"key\": \"value\", \"test\": true}";
    const char* post_url = "https://httpbin.org/post";
    
    sdk_http_request_id_t async_request = sdk_http_request_async(
        SDK_HTTP_METHOD_POST, post_url, NULL, 
        post_data, strlen(post_data), 10000,
        http_response_callback, (void*)post_url);
    
    if (async_request > 0) {
        printf("  Async POST request submitted: %llu\n", async_request);
        printf("  Waiting for response...\n");
        
        // 等待异步请求完成
        sleep(5);
    } else {
        printf("  Failed to submit async POST request\n");
    }
    
    // 7. 测试线程池调整
    printf("\n6. Testing Thread Pool Resize:\n");
    
    printf("  Current thread pool size: %u\n", sdk_thread_pool_get_size());
    
    if (sdk_thread_pool_resize(8)) {
        printf("  Resized thread pool to 8 threads\n");
        printf("  New thread pool size: %u\n", sdk_thread_pool_get_size());
    } else {
        printf("  Failed to resize thread pool\n");
    }
    
    // 8. 等待所有任务完成
    printf("\n7. Waiting for all tasks to complete...\n");
    
    if (sdk_thread_pool_wait_all(10000)) {
        printf("  All tasks completed\n");
    } else {
        printf("  Timeout waiting for tasks\n");
    }
    
    // 9. 最终统计
    printf("\n8. Final Statistics:\n");
    
    if (sdk_thread_pool_get_stats(&stats)) {
        printf("  Final Thread Pool Stats:\n");
        printf("    Completed tasks: %llu\n", stats.completed_tasks);
        printf("    Failed tasks: %llu\n", stats.failed_tasks);
        printf("    Average task duration: %.2f ms\n", stats.average_task_duration_ms);
    }
    
    // 10. 关闭SDK
    printf("\n9. Shutting down SDK...\n");
    
    sdk_shutdown();
    
    printf("  SDK shutdown completed\n");
    
    printf("\n=== Example completed successfully ===\n");
    
    return 0;
}
