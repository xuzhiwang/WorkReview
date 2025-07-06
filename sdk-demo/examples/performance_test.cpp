#include "sdk/sdk_core.h"
#include "sdk/threading/thread_pool.h"
#include "sdk/network/http_client.h"
#include "sdk/logging/logger.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <random>
#include <atomic>
#include <vector>
#include <algorithm>

using namespace sdk;

class PerformanceBenchmark {
public:
    PerformanceBenchmark(std::shared_ptr<Logger> logger) : logger_(logger) {}
    
    // 线程池性能测试
    void benchmarkThreadPool(std::shared_ptr<ThreadPool> thread_pool) {
        logger_->info("=== Thread Pool Performance Benchmark ===");
        
        // 测试1: 轻量级任务吞吐量
        {
            const int task_count = 10000;
            std::atomic<int> completed_tasks{0};
            
            auto start = std::chrono::high_resolution_clock::now();
            
            std::vector<std::future<void>> futures;
            for (int i = 0; i < task_count; ++i) {
                auto future = thread_pool->submit("light_task_" + std::to_string(i),
                                                 TaskPriority::NORMAL,
                                                 [&completed_tasks]() {
                                                     // 轻量级计算
                                                     volatile int result = 0;
                                                     for (int j = 0; j < 100; ++j) {
                                                         result += j;
                                                     }
                                                     completed_tasks.fetch_add(1);
                                                 });
                futures.push_back(std::move(future));
            }
            
            // 等待所有任务完成
            for (auto& future : futures) {
                future.wait();
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            double throughput = static_cast<double>(task_count) / duration.count() * 1000;
            logger_->info("Light tasks: {} tasks in {} ms, throughput: {:.2f} tasks/sec",
                         task_count, duration.count(), throughput);
        }
        
        // 测试2: 中等负载任务
        {
            const int task_count = 1000;
            std::atomic<int> completed_tasks{0};
            
            auto start = std::chrono::high_resolution_clock::now();
            
            std::vector<std::future<void>> futures;
            for (int i = 0; i < task_count; ++i) {
                auto future = thread_pool->submit("medium_task_" + std::to_string(i),
                                                 TaskPriority::NORMAL,
                                                 [&completed_tasks, i]() {
                                                     // 中等计算负载
                                                     std::vector<int> data(1000);
                                                     std::iota(data.begin(), data.end(), i);
                                                     std::sort(data.begin(), data.end());
                                                     completed_tasks.fetch_add(1);
                                                 });
                futures.push_back(std::move(future));
            }
            
            // 等待所有任务完成
            for (auto& future : futures) {
                future.wait();
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            double throughput = static_cast<double>(task_count) / duration.count() * 1000;
            logger_->info("Medium tasks: {} tasks in {} ms, throughput: {:.2f} tasks/sec",
                         task_count, duration.count(), throughput);
        }
        
        // 测试3: 优先级调度性能
        {
            const int high_priority_tasks = 100;
            const int normal_priority_tasks = 500;
            const int low_priority_tasks = 200;
            
            std::atomic<int> high_completed{0};
            std::atomic<int> normal_completed{0};
            std::atomic<int> low_completed{0};
            
            auto start = std::chrono::high_resolution_clock::now();
            
            std::vector<std::future<void>> futures;
            
            // 提交低优先级任务
            for (int i = 0; i < low_priority_tasks; ++i) {
                auto future = thread_pool->submit("low_task_" + std::to_string(i),
                                                 TaskPriority::LOW,
                                                 [&low_completed]() {
                                                     std::this_thread::sleep_for(std::chrono::milliseconds(10));
                                                     low_completed.fetch_add(1);
                                                 });
                futures.push_back(std::move(future));
            }
            
            // 提交普通优先级任务
            for (int i = 0; i < normal_priority_tasks; ++i) {
                auto future = thread_pool->submit("normal_task_" + std::to_string(i),
                                                 TaskPriority::NORMAL,
                                                 [&normal_completed]() {
                                                     std::this_thread::sleep_for(std::chrono::milliseconds(5));
                                                     normal_completed.fetch_add(1);
                                                 });
                futures.push_back(std::move(future));
            }
            
            // 提交高优先级任务
            for (int i = 0; i < high_priority_tasks; ++i) {
                auto future = thread_pool->submit("high_task_" + std::to_string(i),
                                                 TaskPriority::HIGH,
                                                 [&high_completed]() {
                                                     std::this_thread::sleep_for(std::chrono::milliseconds(2));
                                                     high_completed.fetch_add(1);
                                                 });
                futures.push_back(std::move(future));
            }
            
            // 等待所有任务完成
            for (auto& future : futures) {
                future.wait();
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            logger_->info("Priority scheduling test completed in {} ms", duration.count());
            logger_->info("  High priority: {} tasks", high_completed.load());
            logger_->info("  Normal priority: {} tasks", normal_completed.load());
            logger_->info("  Low priority: {} tasks", low_completed.load());
        }
    }
    
    // HTTP客户端性能测试
    void benchmarkHttpClient(std::shared_ptr<HttpClient> http_client) {
        logger_->info("=== HTTP Client Performance Benchmark ===");
        
        // 测试1: 同步请求性能
        {
            const int request_count = 10;
            auto start = std::chrono::high_resolution_clock::now();
            
            std::vector<std::chrono::milliseconds> response_times;
            
            for (int i = 0; i < request_count; ++i) {
                auto request_start = std::chrono::high_resolution_clock::now();
                auto response = http_client->get("https://httpbin.org/json");
                auto request_end = std::chrono::high_resolution_clock::now();
                
                auto request_time = std::chrono::duration_cast<std::chrono::milliseconds>(request_end - request_start);
                response_times.push_back(request_time);
                
                if (response.isSuccess()) {
                    logger_->debug("Sync request {} completed in {} ms", i, request_time.count());
                } else {
                    logger_->error("Sync request {} failed: {}", i, response.getError());
                }
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            // 计算统计信息
            auto min_time = *std::min_element(response_times.begin(), response_times.end());
            auto max_time = *std::max_element(response_times.begin(), response_times.end());
            auto avg_time = std::accumulate(response_times.begin(), response_times.end(), 
                                          std::chrono::milliseconds(0)) / response_times.size();
            
            logger_->info("Sync requests: {} requests in {} ms", request_count, total_duration.count());
            logger_->info("  Min response time: {} ms", min_time.count());
            logger_->info("  Max response time: {} ms", max_time.count());
            logger_->info("  Avg response time: {} ms", avg_time.count());
        }
        
        // 测试2: 异步请求性能
        {
            const int request_count = 20;
            auto start = std::chrono::high_resolution_clock::now();
            
            std::vector<std::future<HttpResponse>> futures;
            
            // 提交所有异步请求
            for (int i = 0; i < request_count; ++i) {
                auto future = http_client->getAsync("https://httpbin.org/delay/1");
                futures.push_back(std::move(future));
            }
            
            // 等待所有请求完成
            std::vector<std::chrono::milliseconds> response_times;
            for (int i = 0; i < request_count; ++i) {
                auto response = futures[i].get();
                if (response.isSuccess()) {
                    response_times.push_back(response.getResponseTime());
                    logger_->debug("Async request {} completed in {} ms", i, response.getResponseTime().count());
                } else {
                    logger_->error("Async request {} failed: {}", i, response.getError());
                }
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            if (!response_times.empty()) {
                auto min_time = *std::min_element(response_times.begin(), response_times.end());
                auto max_time = *std::max_element(response_times.begin(), response_times.end());
                auto avg_time = std::accumulate(response_times.begin(), response_times.end(), 
                                              std::chrono::milliseconds(0)) / response_times.size();
                
                logger_->info("Async requests: {} requests in {} ms (wall time)", request_count, total_duration.count());
                logger_->info("  Min response time: {} ms", min_time.count());
                logger_->info("  Max response time: {} ms", max_time.count());
                logger_->info("  Avg response time: {} ms", avg_time.count());
                logger_->info("  Concurrency benefit: {:.2f}x faster", 
                             static_cast<double>(avg_time.count() * request_count) / total_duration.count());
            }
        }
    }
    
    // 日志性能测试
    void benchmarkLogging(std::shared_ptr<Logger> logger) {
        logger_->info("=== Logging Performance Benchmark ===");
        
        // 测试1: 单线程日志性能
        {
            const int message_count = 50000;
            auto start = std::chrono::high_resolution_clock::now();
            
            for (int i = 0; i < message_count; ++i) {
                logger->debug("Performance test message {} with some additional data: {}", i, i * 2);
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            double throughput = static_cast<double>(message_count) / duration.count() * 1000;
            logger_->info("Single-threaded logging: {} messages in {} ms, throughput: {:.2f} msg/sec",
                         message_count, duration.count(), throughput);
        }
        
        // 测试2: 多线程日志性能
        {
            const int thread_count = 4;
            const int messages_per_thread = 10000;
            const int total_messages = thread_count * messages_per_thread;
            
            std::atomic<int> completed_threads{0};
            auto start = std::chrono::high_resolution_clock::now();
            
            std::vector<std::thread> threads;
            for (int t = 0; t < thread_count; ++t) {
                threads.emplace_back([logger, t, messages_per_thread, &completed_threads]() {
                    for (int i = 0; i < messages_per_thread; ++i) {
                        logger->debug("Thread {} message {} with data: {}", t, i, t * 1000 + i);
                    }
                    completed_threads.fetch_add(1);
                });
            }
            
            // 等待所有线程完成
            for (auto& thread : threads) {
                thread.join();
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            double throughput = static_cast<double>(total_messages) / duration.count() * 1000;
            logger_->info("Multi-threaded logging: {} messages ({} threads) in {} ms, throughput: {:.2f} msg/sec",
                         total_messages, thread_count, duration.count(), throughput);
        }
    }

private:
    std::shared_ptr<Logger> logger_;
};

int main() {
    std::cout << "=== Performance Test Suite ===" << std::endl;
    
    try {
        // 初始化SDK
        SDKConfig config;
        config.thread_pool_size = 8;
        config.enable_hyperthreading = true;
        config.user_agent = "Performance-Test/1.0";
        config.connection_timeout_ms = 10000;
        config.request_timeout_ms = 30000;
        config.max_concurrent_requests = 50;
        config.log_level = "info";
        config.log_file_path = "performance_test.log";
        config.enable_console_log = true;
        config.enable_metrics = true;
        
        auto result = SDK::getInstance().initialize(config);
        if (result != InitResult::SUCCESS) {
            std::cerr << "Failed to initialize SDK" << std::endl;
            return 1;
        }
        
        auto thread_pool = SDK::getInstance().getThreadPool();
        auto http_client = SDK::getInstance().getHttpClient();
        auto logger = SDK::getInstance().getLogger();
        
        if (!thread_pool || !http_client || !logger) {
            std::cerr << "Failed to get SDK components" << std::endl;
            return 1;
        }
        
        logger->info("=== Performance Test Suite Started ===");
        logger->info("System: {} cores, {} MB memory", 
                    std::thread::hardware_concurrency(),
                    platform::PlatformUtils::getTotalMemory() / (1024 * 1024));
        
        // 创建性能测试器
        PerformanceBenchmark benchmark(logger);
        
        // 运行线程池性能测试
        std::cout << "\nRunning Thread Pool Benchmarks..." << std::endl;
        benchmark.benchmarkThreadPool(thread_pool);
        
        // 运行HTTP客户端性能测试
        std::cout << "\nRunning HTTP Client Benchmarks..." << std::endl;
        benchmark.benchmarkHttpClient(http_client);
        
        // 运行日志性能测试
        std::cout << "\nRunning Logging Benchmarks..." << std::endl;
        benchmark.benchmarkLogging(logger);
        
        // 综合性能测试
        std::cout << "\nRunning Integrated Performance Test..." << std::endl;
        logger->info("=== Integrated Performance Test ===");
        
        const int integrated_task_count = 100;
        std::atomic<int> completed_tasks{0};
        
        auto integrated_start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::future<void>> integrated_futures;
        for (int i = 0; i < integrated_task_count; ++i) {
            auto future = thread_pool->submit("integrated_task_" + std::to_string(i),
                                             TaskPriority::NORMAL,
                                             [logger, http_client, i, &completed_tasks]() {
                                                 // 组合操作：日志 + HTTP请求 + 计算
                                                 logger->debug("Integrated task {} started", i);
                                                 
                                                 // HTTP请求
                                                 auto response = http_client->get("https://httpbin.org/json");
                                                 
                                                 // 计算
                                                 volatile int result = 0;
                                                 for (int j = 0; j < 10000; ++j) {
                                                     result += i * j;
                                                 }
                                                 
                                                 logger->debug("Integrated task {} completed, HTTP status: {}", 
                                                              i, response.getStatusCode());
                                                 completed_tasks.fetch_add(1);
                                             });
            integrated_futures.push_back(std::move(future));
        }
        
        // 等待所有集成测试任务完成
        for (auto& future : integrated_futures) {
            future.wait();
        }
        
        auto integrated_end = std::chrono::high_resolution_clock::now();
        auto integrated_duration = std::chrono::duration_cast<std::chrono::milliseconds>(integrated_end - integrated_start);
        
        logger->info("Integrated test: {} tasks in {} ms, throughput: {:.2f} tasks/sec",
                    integrated_task_count, integrated_duration.count(),
                    static_cast<double>(integrated_task_count) / integrated_duration.count() * 1000);
        
        // 最终统计
        auto final_thread_stats = thread_pool->getStats();
        auto final_http_stats = http_client->getStats();
        
        logger->info("=== Final Performance Statistics ===");
        logger->info("Thread Pool:");
        logger->info("  Total completed tasks: {}", final_thread_stats.completed_tasks);
        logger->info("  Average task duration: {:.2f} ms", final_thread_stats.average_task_duration_ms);
        logger->info("HTTP Client:");
        logger->info("  Total requests: {}", final_http_stats.total_requests);
        logger->info("  Success rate: {:.2f}%", 
                    static_cast<double>(final_http_stats.successful_requests) / final_http_stats.total_requests * 100);
        logger->info("  Average response time: {} ms", final_http_stats.average_time.count());
        
        logger->info("=== Performance Test Suite Completed ===");
        
        // 关闭SDK
        SDK::getInstance().shutdown();
        
        std::cout << "\n=== Performance Test Suite Completed Successfully ===" << std::endl;
        std::cout << "Check 'performance_test.log' for detailed results" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
