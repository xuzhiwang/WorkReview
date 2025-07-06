#include "sdk/sdk_core.h"
#include "sdk/threading/thread_pool.h"
#include "sdk/network/http_client.h"
#include "sdk/logging/logger.h"
#include "sdk/platform/platform_utils.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <random>

using namespace sdk;

// 模拟数据处理任务
class DataProcessor {
public:
    DataProcessor(std::shared_ptr<Logger> logger, std::shared_ptr<HttpClient> http_client)
        : logger_(logger), http_client_(http_client) {}
    
    void processData(int batch_id) {
        logger_->info("Starting data processing for batch {}", batch_id);
        
        // 模拟数据获取
        auto response = http_client_->get("https://httpbin.org/json");
        if (response.isSuccess()) {
            logger_->debug("Data fetched successfully for batch {}, size: {} bytes", 
                          batch_id, response.getBody().length());
        } else {
            logger_->error("Failed to fetch data for batch {}: {}", batch_id, response.getError());
            return;
        }
        
        // 模拟数据处理
        std::this_thread::sleep_for(std::chrono::milliseconds(100 + (batch_id % 5) * 50));
        
        // 模拟结果上传
        std::string result_data = R"({"batch_id": )" + std::to_string(batch_id) + 
                                 R"(, "status": "completed", "timestamp": )" + 
                                 std::to_string(std::time(nullptr)) + "}";
        
        HttpRequest upload_request("https://httpbin.org/post");
        upload_request.setMethod(HttpMethod::POST)
                     .setHeader("Content-Type", "application/json")
                     .setBody(result_data);
        
        auto upload_response = http_client_->request(upload_request);
        if (upload_response.isSuccess()) {
            logger_->info("Batch {} processing completed and uploaded successfully", batch_id);
        } else {
            logger_->error("Failed to upload results for batch {}: {}", batch_id, upload_response.getError());
        }
    }

private:
    std::shared_ptr<Logger> logger_;
    std::shared_ptr<HttpClient> http_client_;
};

// 模拟监控任务
class SystemMonitor {
public:
    SystemMonitor(std::shared_ptr<Logger> logger) : logger_(logger) {}
    
    void monitor() {
        logger_->info("System monitoring started");
        
        for (int i = 0; i < 10; ++i) {
            // 获取系统信息
            auto system_info = platform::PlatformUtils::getSystemInfo();
            auto memory_usage = platform::PlatformUtils::getProcessMemoryUsage();
            auto available_memory = platform::PlatformUtils::getAvailableMemory();
            
            double memory_usage_mb = static_cast<double>(memory_usage) / (1024 * 1024);
            double available_memory_mb = static_cast<double>(available_memory) / (1024 * 1024);
            
            logger_->info("System Monitor - CPU Cores: {}, Memory Usage: {:.2f} MB, Available: {:.2f} MB",
                         system_info.cpu_core_count, memory_usage_mb, available_memory_mb);
            
            // 检查内存使用情况
            if (memory_usage_mb > 100) {
                logger_->warn("High memory usage detected: {:.2f} MB", memory_usage_mb);
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        
        logger_->info("System monitoring completed");
    }

private:
    std::shared_ptr<Logger> logger_;
};

int main() {
    std::cout << "=== Comprehensive SDK Example ===" << std::endl;
    
    try {
        // 初始化SDK
        std::cout << "Initializing SDK..." << std::endl;
        
        SDKConfig config;
        config.thread_pool_size = 6;
        config.enable_hyperthreading = true;
        config.user_agent = "Comprehensive-Example/1.0";
        config.connection_timeout_ms = 5000;
        config.request_timeout_ms = 30000;
        config.max_concurrent_requests = 10;
        config.log_level = "info";
        config.log_file_path = "comprehensive_example.log";
        config.enable_console_log = true;
        config.max_log_file_size = 50 * 1024 * 1024; // 50MB
        config.max_log_files = 3;
        config.enable_metrics = true;
        
        auto result = SDK::getInstance().initialize(config);
        if (result != InitResult::SUCCESS) {
            std::cerr << "Failed to initialize SDK" << std::endl;
            return 1;
        }
        
        // 获取SDK组件
        auto thread_pool = SDK::getInstance().getThreadPool();
        auto http_client = SDK::getInstance().getHttpClient();
        auto logger = SDK::getInstance().getLogger();
        
        if (!thread_pool || !http_client || !logger) {
            std::cerr << "Failed to get SDK components" << std::endl;
            return 1;
        }
        
        logger->info("=== Comprehensive SDK Example Started ===");
        logger->info("SDK Version: {}", SDK::getInstance().getVersion());
        logger->info("Platform: {}", SDK::getInstance().getPlatformInfo());
        
        // 显示系统信息
        auto system_info = platform::PlatformUtils::getSystemInfo();
        logger->info("System Info - OS: {}, Architecture: {}, CPU Cores: {}, Total Memory: {} MB",
                    system_info.os_name, 
                    static_cast<int>(system_info.architecture),
                    system_info.cpu_core_count,
                    system_info.total_memory_bytes / (1024 * 1024));
        
        // 创建数据处理器
        DataProcessor processor(logger, http_client);
        
        // 创建系统监控器
        SystemMonitor monitor(logger);
        
        // 场景1: 并发数据处理
        std::cout << "\n--- Scenario 1: Concurrent Data Processing ---" << std::endl;
        logger->info("Starting concurrent data processing scenario");
        
        std::vector<std::future<void>> processing_futures;
        
        // 提交数据处理任务
        for (int i = 0; i < 8; ++i) {
            auto future = thread_pool->submit("data_process_" + std::to_string(i),
                                             TaskPriority::NORMAL,
                                             [&processor, i]() {
                                                 processor.processData(i);
                                             });
            processing_futures.push_back(std::move(future));
        }
        
        // 启动系统监控
        auto monitor_future = thread_pool->submit("system_monitor", TaskPriority::LOW,
                                                 [&monitor]() {
                                                     monitor.monitor();
                                                 });
        
        // 等待数据处理完成
        for (auto& future : processing_futures) {
            future.wait();
        }
        
        logger->info("Data processing scenario completed");
        
        // 场景2: 高优先级紧急任务
        std::cout << "\n--- Scenario 2: High Priority Emergency Task ---" << std::endl;
        logger->info("Simulating emergency task scenario");
        
        // 提交一些普通任务
        std::vector<std::future<void>> normal_futures;
        for (int i = 0; i < 5; ++i) {
            auto future = thread_pool->submit("normal_task_" + std::to_string(i),
                                             TaskPriority::NORMAL,
                                             [logger, i]() {
                                                 logger->debug("Normal task {} executing", i);
                                                 std::this_thread::sleep_for(std::chrono::milliseconds(500));
                                                 logger->debug("Normal task {} completed", i);
                                             });
            normal_futures.push_back(std::move(future));
        }
        
        // 等待一会儿，然后提交紧急任务
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        auto emergency_future = thread_pool->submit("emergency_task", TaskPriority::CRITICAL,
                                                   [logger]() {
                                                       logger->critical("EMERGENCY: Critical system task executing");
                                                       std::this_thread::sleep_for(std::chrono::milliseconds(200));
                                                       logger->critical("EMERGENCY: Critical system task completed");
                                                   });
        
        // 等待紧急任务完成
        emergency_future.wait();
        
        // 等待普通任务完成
        for (auto& future : normal_futures) {
            future.wait();
        }
        
        logger->info("Emergency task scenario completed");
        
        // 场景3: 异步HTTP请求批处理
        std::cout << "\n--- Scenario 3: Async HTTP Batch Processing ---" << std::endl;
        logger->info("Starting async HTTP batch processing");
        
        std::vector<std::future<HttpResponse>> http_futures;
        
        // 提交多个异步HTTP请求
        std::vector<std::string> urls = {
            "https://httpbin.org/delay/1",
            "https://httpbin.org/json",
            "https://httpbin.org/uuid",
            "https://httpbin.org/user-agent",
            "https://httpbin.org/headers"
        };
        
        for (size_t i = 0; i < urls.size(); ++i) {
            auto future = http_client->getAsync(urls[i]);
            http_futures.push_back(std::move(future));
            logger->debug("Submitted async HTTP request {} to {}", i, urls[i]);
        }
        
        // 收集结果
        for (size_t i = 0; i < http_futures.size(); ++i) {
            auto response = http_futures[i].get();
            if (response.isSuccess()) {
                logger->info("HTTP request {} completed: status={}, time={}ms, size={}bytes",
                           i, response.getStatusCode(), response.getResponseTime().count(),
                           response.getBody().length());
            } else {
                logger->error("HTTP request {} failed: {}", i, response.getError());
            }
        }
        
        logger->info("Async HTTP batch processing completed");
        
        // 场景4: 错误处理和恢复
        std::cout << "\n--- Scenario 4: Error Handling and Recovery ---" << std::endl;
        logger->info("Testing error handling and recovery");
        
        // 提交一个会失败的任务
        auto error_future = thread_pool->submit("error_task", TaskPriority::NORMAL,
                                               [logger]() {
                                                   logger->warn("Task about to throw exception");
                                                   throw std::runtime_error("Simulated task failure");
                                               });
        
        // 提交恢复任务
        auto recovery_future = thread_pool->submit("recovery_task", TaskPriority::HIGH,
                                                  [logger]() {
                                                      logger->info("Recovery task executing");
                                                      std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                                      logger->info("System recovered successfully");
                                                  });
        
        // 等待任务完成
        try {
            error_future.wait();
        } catch (const std::exception& e) {
            logger->error("Caught expected exception: {}", e.what());
        }
        
        recovery_future.wait();
        
        // 等待监控任务完成
        monitor_future.wait();
        
        // 最终统计
        std::cout << "\n--- Final Statistics ---" << std::endl;
        
        auto thread_stats = thread_pool->getStats();
        logger->info("Thread Pool Final Stats:");
        logger->info("  Completed tasks: {}", thread_stats.completed_tasks);
        logger->info("  Failed tasks: {}", thread_stats.failed_tasks);
        logger->info("  Average task duration: {:.2f} ms", thread_stats.average_task_duration_ms);
        
        auto http_stats = http_client->getStats();
        logger->info("HTTP Client Final Stats:");
        logger->info("  Total requests: {}", http_stats.total_requests);
        logger->info("  Successful requests: {}", http_stats.successful_requests);
        logger->info("  Failed requests: {}", http_stats.failed_requests);
        logger->info("  Average response time: {} ms", http_stats.average_time.count());
        
        // 性能测试
        std::cout << "\n--- Performance Test ---" << std::endl;
        logger->info("Running performance test");
        
        const int perf_task_count = 100;
        auto perf_start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::future<void>> perf_futures;
        for (int i = 0; i < perf_task_count; ++i) {
            auto future = thread_pool->submit("perf_task_" + std::to_string(i),
                                             TaskPriority::NORMAL,
                                             [i]() {
                                                 // 模拟轻量级计算
                                                 volatile int result = 0;
                                                 for (int j = 0; j < 1000; ++j) {
                                                     result += i * j;
                                                 }
                                             });
            perf_futures.push_back(std::move(future));
        }
        
        // 等待所有性能测试任务完成
        for (auto& future : perf_futures) {
            future.wait();
        }
        
        auto perf_end = std::chrono::high_resolution_clock::now();
        auto perf_duration = std::chrono::duration_cast<std::chrono::milliseconds>(perf_end - perf_start);
        
        logger->info("Performance test completed: {} tasks in {} ms", perf_task_count, perf_duration.count());
        logger->info("Throughput: {:.2f} tasks per second", 
                    static_cast<double>(perf_task_count) / perf_duration.count() * 1000);
        
        // 等待所有任务完成
        thread_pool->waitForAll();
        
        logger->info("=== Comprehensive SDK Example Completed ===");
        
        // 关闭SDK
        SDK::getInstance().shutdown();
        
        std::cout << "\n=== Comprehensive Example Completed Successfully ===" << std::endl;
        std::cout << "Check 'comprehensive_example.log' for detailed logs" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
