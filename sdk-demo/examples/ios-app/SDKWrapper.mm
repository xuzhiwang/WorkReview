#import "SDKWrapper.h"
#include "sdk/sdk_c_api.h"
#include <string>

@implementation SDKWrapper

- (BOOL)initializeSDK {
    sdk_config_t config = {0};
    config.thread_pool_size = 4;
    config.enable_hyperthreading = true;
    strcpy(config.user_agent, "iOS-SDK-Test/1.0");
    config.connection_timeout_ms = 5000;
    config.request_timeout_ms = 30000;
    strcpy(config.log_level, "info");
    config.enable_console_log = true;
    
    sdk_init_result_t result = sdk_init(&config);
    return result == SDK_INIT_SUCCESS;
}

- (void)shutdownSDK {
    sdk_shutdown();
}

- (NSString *)getSDKVersion {
    char buffer[256];
    uint32_t len = sdk_get_version(buffer, sizeof(buffer));
    if (len > 0) {
        return [NSString stringWithUTF8String:buffer];
    }
    return @"Unknown";
}

- (NSString *)getPlatformInfo {
    char buffer[512];
    uint32_t len = sdk_get_platform_info(buffer, sizeof(buffer));
    if (len > 0) {
        return [NSString stringWithUTF8String:buffer];
    }
    return @"Unknown";
}

// 任务函数
void test_task(void* user_data) {
    int* value = (int*)user_data;
    usleep(100000); // 100ms
    *value *= 2;
}

// 任务回调
void task_callback(sdk_task_id_t task_id, sdk_task_status_t status, void* user_data) {
    SDKTestCallback callback = (__bridge SDKTestCallback)user_data;
    
    NSString *statusStr = @"Unknown";
    switch (status) {
        case SDK_TASK_STATUS_COMPLETED:
            statusStr = @"Completed";
            break;
        case SDK_TASK_STATUS_FAILED:
            statusStr = @"Failed";
            break;
        case SDK_TASK_STATUS_CANCELLED:
            statusStr = @"Cancelled";
            break;
        default:
            break;
    }
    
    NSString *result = [NSString stringWithFormat:@"Task %llu: %@", task_id, statusStr];
    
    dispatch_async(dispatch_get_main_queue(), ^{
        callback(result);
    });
}

- (void)testThreadPool:(SDKTestCallback)callback {
    // 获取线程池信息
    uint32_t pool_size = sdk_thread_pool_get_size();
    uint32_t active_threads = sdk_thread_pool_get_active_threads();
    uint32_t pending_tasks = sdk_thread_pool_get_pending_tasks();
    
    NSString *info = [NSString stringWithFormat:@"Thread Pool: %u threads, %u active, %u pending", 
                      pool_size, active_threads, pending_tasks];
    callback(info);
    
    // 提交测试任务
    static int test_data = 10;
    
    sdk_task_id_t task_id = sdk_thread_pool_submit_task(
        test_task, 
        &test_data, 
        SDK_TASK_PRIORITY_NORMAL, 
        task_callback
    );
    
    if (task_id > 0) {
        NSString *result = [NSString stringWithFormat:@"Submitted task %llu with data %d", task_id, test_data];
        callback(result);
        
        // 等待任务完成
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            if (sdk_thread_pool_wait_task(task_id, 5000)) {
                NSString *final_result = [NSString stringWithFormat:@"Task completed, result: %d", test_data];
                dispatch_async(dispatch_get_main_queue(), ^{
                    callback(final_result);
                });
            } else {
                dispatch_async(dispatch_get_main_queue(), ^{
                    callback(@"Task timeout");
                });
            }
        });
    } else {
        callback(@"Failed to submit task");
    }
}

// HTTP响应回调
void http_callback(sdk_http_request_id_t request_id, const sdk_http_response_t* response, void* user_data) {
    SDKTestCallback callback = (__bridge SDKTestCallback)user_data;
    
    NSString *result;
    if (response->status_code > 0) {
        result = [NSString stringWithFormat:@"HTTP Response: %d (%u ms)", 
                  response->status_code, response->response_time_ms];
        
        if (response->body.size > 0 && response->body.data) {
            NSString *body = [[NSString alloc] initWithBytes:response->body.data 
                                                      length:MIN(response->body.size, 100) 
                                                    encoding:NSUTF8StringEncoding];
            if (body) {
                NSString *preview = [NSString stringWithFormat:@"Body preview: %@%@", 
                                     body, response->body.size > 100 ? @"..." : @""];
                result = [result stringByAppendingFormat:@"\n%@", preview];
            }
        }
    } else {
        result = [NSString stringWithFormat:@"HTTP Error: %s", response->error_message];
    }
    
    dispatch_async(dispatch_get_main_queue(), ^{
        callback(result);
    });
}

- (void)testHttpClient:(SDKTestCallback)callback {
    callback(@"Starting HTTP test...");
    
    // 同步GET请求
    sdk_http_response_t response = {0};
    bool success = sdk_http_get("https://httpbin.org/get", NULL, &response);
    
    if (success) {
        NSString *result = [NSString stringWithFormat:@"Sync GET: %d (%u ms)", 
                            response.status_code, response.response_time_ms];
        callback(result);
    } else {
        NSString *error = [NSString stringWithFormat:@"Sync GET failed: %s", response.error_message];
        callback(error);
    }
    
    sdk_http_response_free(&response);
    
    // 异步POST请求
    const char* json_data = "{\"platform\": \"iOS\", \"test\": true}";
    
    // 保持callback的引用
    SDKTestCallback retainedCallback = [callback copy];
    
    sdk_http_request_id_t request_id = sdk_http_request_async(
        SDK_HTTP_METHOD_POST,
        "https://httpbin.org/post",
        NULL,
        json_data,
        strlen(json_data),
        10000,
        http_callback,
        (__bridge void*)retainedCallback
    );
    
    if (request_id > 0) {
        NSString *result = [NSString stringWithFormat:@"Async POST submitted: %llu", request_id];
        callback(result);
    } else {
        callback(@"Failed to submit async POST");
    }
}

- (void)testLogging:(SDKTestCallback)callback {
    // 设置日志级别
    sdk_log_set_level(SDK_LOG_LEVEL_DEBUG);
    
    // 测试不同级别的日志
    SDK_LOG_INFO("iOS App: Testing logging system");
    SDK_LOG_DEBUG("iOS App: Debug message from iOS");
    SDK_LOG_WARN("iOS App: Warning message");
    SDK_LOG_ERROR("iOS App: Error message for testing");
    
    callback(@"Log messages sent");
    
    // 获取当前日志级别
    sdk_log_level_t level = sdk_log_get_level();
    NSString *levelStr = @"Unknown";
    
    switch (level) {
        case SDK_LOG_LEVEL_TRACE:
            levelStr = @"TRACE";
            break;
        case SDK_LOG_LEVEL_DEBUG:
            levelStr = @"DEBUG";
            break;
        case SDK_LOG_LEVEL_INFO:
            levelStr = @"INFO";
            break;
        case SDK_LOG_LEVEL_WARN:
            levelStr = @"WARN";
            break;
        case SDK_LOG_LEVEL_ERROR:
            levelStr = @"ERROR";
            break;
        case SDK_LOG_LEVEL_CRITICAL:
            levelStr = @"CRITICAL";
            break;
        default:
            break;
    }
    
    NSString *result = [NSString stringWithFormat:@"Current log level: %@", levelStr];
    callback(result);
}

@end
