#include "sdk/sdk_core.h"
#include "sdk/threading/thread_pool.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <random>

using namespace sdk;

// 示例任务函数
void simpleTask(int id) {
    std::cout << "Task " << id << " started on thread " 
              << std::this_thread::get_id() << std::endl;
    
    // 模拟一些工作
    std::this_thread::sleep_for(std::chrono::milliseconds(100 + (id % 5) * 50));
    
    std::cout << "Task " << id << " completed" << std::endl;
}

void longRunningTask(int id) {
    std::cout << "Long task " << id << " started" << std::endl;
    
    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::cout << "Long task " << id << " progress: " << (i + 1) * 10 << "%" << std::endl;
    }
    
    std::cout << "Long task " << id << " completed" << std::endl;
}

int main() {
    std::cout << "=== Thread Pool Example ===" << std::endl;
    
    try {
        // 初始化SDK
        SDKConfig config;
        config.thread_pool_size = 4;
        config.log_level = "info";
        config.enable_console_log = true;
        
        auto result = SDK::getInstance().initialize(config);
        if (result != InitResult::SUCCESS) {
            std::cerr << "Failed to initialize SDK" << std::endl;
            return 1;
        }
        
        auto thread_pool = SDK::getInstance().getThreadPool();
        if (!thread_pool) {
            std::cerr << "Failed to get thread pool" << std::endl;
            return 1;
        }
        
        std::cout << "Thread pool initialized with " << thread_pool->size() << " threads" << std::endl;
        
        // 示例1: 提交简单任务
        std::cout << "\n--- Example 1: Simple Tasks ---" << std::endl;
        
        std::vector<std::future<void>> futures;
        for (int i = 0; i < 8; ++i) {
            auto future = thread_pool->submit("simple_task_" + std::to_string(i), 
                                             TaskPriority::NORMAL,
                                             [i]() { simpleTask(i); });
            futures.push_back(std::move(future));
        }
        
        // 等待所有简单任务完成
        for (auto& future : futures) {
            future.wait();
        }
        
        std::cout << "All simple tasks completed" << std::endl;
        
        // 示例2: 不同优先级的任务
        std::cout << "\n--- Example 2: Priority Tasks ---" << std::endl;
        
        // 提交低优先级任务
        auto low_future = thread_pool->submit("low_priority", TaskPriority::LOW,
                                             []() { 
                                                 std::cout << "Low priority task executing" << std::endl;
                                                 std::this_thread::sleep_for(std::chrono::milliseconds(500));
                                                 std::cout << "Low priority task completed" << std::endl;
                                             });
        
        // 提交高优先级任务
        auto high_future = thread_pool->submit("high_priority", TaskPriority::HIGH,
                                              []() { 
                                                  std::cout << "High priority task executing" << std::endl;
                                                  std::this_thread::sleep_for(std::chrono::milliseconds(200));
                                                  std::cout << "High priority task completed" << std::endl;
                                              });
        
        // 等待优先级任务完成
        high_future.wait();
        low_future.wait();
        
        // 示例3: 任务取消
        std::cout << "\n--- Example 3: Task Cancellation ---" << std::endl;
        
        // 提交长时间运行的任务
        auto long_future = thread_pool->submit("long_task", TaskPriority::NORMAL,
                                              []() { longRunningTask(1); });
        
        // 等待一段时间后取消
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        bool cancelled = thread_pool->cancelTask("long_task");
        std::cout << "Task cancellation " << (cancelled ? "succeeded" : "failed") << std::endl;
        
        // 示例4: 线程池统计
        std::cout << "\n--- Example 4: Thread Pool Statistics ---" << std::endl;
        
        auto stats = thread_pool->getStats();
        std::cout << "Thread Pool Statistics:" << std::endl;
        std::cout << "  Thread count: " << stats.thread_count << std::endl;
        std::cout << "  Active threads: " << stats.active_threads << std::endl;
        std::cout << "  Pending tasks: " << stats.pending_tasks << std::endl;
        std::cout << "  Completed tasks: " << stats.completed_tasks << std::endl;
        std::cout << "  Failed tasks: " << stats.failed_tasks << std::endl;
        std::cout << "  Average task duration: " << stats.average_task_duration_ms << " ms" << std::endl;
        
        // 示例5: 动态调整线程池大小
        std::cout << "\n--- Example 5: Dynamic Resizing ---" << std::endl;
        
        std::cout << "Current thread pool size: " << thread_pool->size() << std::endl;
        
        // 增加线程数
        thread_pool->resize(8);
        std::cout << "Resized thread pool to: " << thread_pool->size() << std::endl;
        
        // 提交更多任务测试新的线程数
        std::vector<std::future<void>> resize_futures;
        for (int i = 0; i < 12; ++i) {
            auto future = thread_pool->submit("resize_task_" + std::to_string(i),
                                             TaskPriority::NORMAL,
                                             [i]() {
                                                 std::cout << "Resize test task " << i << " on thread " 
                                                          << std::this_thread::get_id() << std::endl;
                                                 std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                             });
            resize_futures.push_back(std::move(future));
        }
        
        // 等待所有调整大小测试任务完成
        for (auto& future : resize_futures) {
            future.wait();
        }
        
        // 示例6: 批量任务处理
        std::cout << "\n--- Example 6: Batch Processing ---" << std::endl;
        
        const int batch_size = 20;
        std::vector<std::future<int>> batch_futures;
        
        // 提交批量计算任务
        for (int i = 0; i < batch_size; ++i) {
            auto future = thread_pool->submit("batch_task_" + std::to_string(i),
                                             TaskPriority::NORMAL,
                                             [i]() -> int {
                                                 // 模拟计算工作
                                                 int result = 0;
                                                 for (int j = 0; j < 1000; ++j) {
                                                     result += i * j;
                                                 }
                                                 return result;
                                             });
            batch_futures.push_back(std::move(future));
        }
        
        // 收集批量处理结果
        std::cout << "Batch processing results:" << std::endl;
        for (int i = 0; i < batch_size; ++i) {
            int result = batch_futures[i].get();
            std::cout << "  Task " << i << " result: " << result << std::endl;
        }
        
        // 等待所有任务完成
        thread_pool->waitForAll();
        
        // 最终统计
        auto final_stats = thread_pool->getStats();
        std::cout << "\n--- Final Statistics ---" << std::endl;
        std::cout << "Total completed tasks: " << final_stats.completed_tasks << std::endl;
        std::cout << "Total failed tasks: " << final_stats.failed_tasks << std::endl;
        std::cout << "Average task duration: " << final_stats.average_task_duration_ms << " ms" << std::endl;
        
        // 关闭SDK
        SDK::getInstance().shutdown();
        
        std::cout << "\n=== Thread Pool Example Completed ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
