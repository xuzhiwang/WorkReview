#include "sdk/sdk_core.h"
#include "sdk/logging/logger.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace sdk;

int main() {
    std::cout << "=== Logging Example ===" << std::endl;
    
    try {
        // 初始化SDK
        SDKConfig config;
        config.log_level = "trace";
        config.log_file_path = "logging_example.log";
        config.enable_console_log = true;
        config.max_log_file_size = 10 * 1024 * 1024; // 10MB
        config.max_log_files = 5;
        
        auto result = SDK::getInstance().initialize(config);
        if (result != InitResult::SUCCESS) {
            std::cerr << "Failed to initialize SDK" << std::endl;
            return 1;
        }
        
        auto logger = SDK::getInstance().getLogger();
        if (!logger) {
            std::cerr << "Failed to get logger" << std::endl;
            return 1;
        }
        
        // 示例1: 基本日志级别
        std::cout << "\n--- Example 1: Basic Log Levels ---" << std::endl;
        
        logger->trace("This is a trace message - very detailed debugging info");
        logger->debug("This is a debug message - debugging information");
        logger->info("This is an info message - general information");
        logger->warn("This is a warning message - something might be wrong");
        logger->error("This is an error message - something went wrong");
        logger->critical("This is a critical message - system is in critical state");
        
        // 示例2: 格式化日志
        std::cout << "\n--- Example 2: Formatted Logging ---" << std::endl;
        
        int user_id = 12345;
        std::string username = "john_doe";
        double balance = 1234.56;
        
        logger->info("User login: ID={}, Username={}, Balance=${:.2f}", user_id, username, balance);
        logger->warn("Low balance warning for user {}: ${:.2f}", username, balance);
        logger->error("Transaction failed for user {} (ID: {}): Insufficient funds", username, user_id);
        
        // 示例3: 上下文日志
        std::cout << "\n--- Example 3: Contextual Logging ---" << std::endl;
        
        std::unordered_map<std::string, std::string> context = {
            {"session_id", "sess_abc123"},
            {"request_id", "req_xyz789"},
            {"client_ip", "192.168.1.100"}
        };
        
        logger->logWithContext(LogLevel::INFO, "User authentication successful", context);
        logger->logWithContext(LogLevel::WARN, "Rate limit approaching", context);
        logger->logWithContext(LogLevel::ERROR, "Database connection failed", context);
        
        // 示例4: 多线程日志
        std::cout << "\n--- Example 4: Multi-threaded Logging ---" << std::endl;
        
        std::vector<std::thread> threads;
        
        for (int i = 0; i < 5; ++i) {
            threads.emplace_back([logger, i]() {
                for (int j = 0; j < 10; ++j) {
                    logger->info("Thread {} - Message {}", i, j);
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });
        }
        
        // 等待所有线程完成
        for (auto& thread : threads) {
            thread.join();
        }
        
        logger->info("Multi-threaded logging completed");
        
        // 示例5: 动态日志级别
        std::cout << "\n--- Example 5: Dynamic Log Level ---" << std::endl;
        
        logger->info("Current log level: {}", static_cast<int>(logger->getLevel()));
        
        // 设置为WARNING级别
        logger->setLevel(LogLevel::WARN);
        logger->info("This info message should not appear");
        logger->warn("This warning message should appear");
        logger->error("This error message should appear");
        
        // 恢复到TRACE级别
        logger->setLevel(LogLevel::TRACE);
        logger->info("Log level restored - this info message should appear");
        
        // 示例6: 自定义日志器
        std::cout << "\n--- Example 6: Custom Logger ---" << std::endl;
        
        auto custom_logger = LogManager::getInstance().getLogger("CustomModule");
        custom_logger->setLevel(LogLevel::DEBUG);
        
        custom_logger->debug("Custom logger debug message");
        custom_logger->info("Custom logger info message");
        custom_logger->warn("Custom logger warning message");
        
        // 示例7: 性能测试
        std::cout << "\n--- Example 7: Performance Test ---" << std::endl;
        
        const int message_count = 10000;
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < message_count; ++i) {
            logger->debug("Performance test message {}", i);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        logger->info("Performance test completed: {} messages in {} ms", 
                    message_count, duration.count());
        logger->info("Average: {:.2f} messages per second", 
                    static_cast<double>(message_count) / duration.count() * 1000);
        
        // 示例8: 错误场景日志
        std::cout << "\n--- Example 8: Error Scenarios ---" << std::endl;
        
        try {
            // 模拟一些错误场景
            throw std::runtime_error("Simulated network error");
        } catch (const std::exception& e) {
            logger->error("Caught exception: {}", e.what());
            logger->error("Stack trace would be here in a real application");
        }
        
        // 模拟系统错误
        logger->critical("System memory usage: 95% - Critical level reached");
        logger->critical("Disk space: 99% full - Immediate action required");
        
        // 示例9: 结构化日志
        std::cout << "\n--- Example 9: Structured Logging ---" << std::endl;
        
        // 模拟API请求日志
        logger->info("API Request: method=GET, path=/api/users, status=200, duration=45ms, user_id=12345");
        logger->info("API Request: method=POST, path=/api/orders, status=201, duration=123ms, user_id=12345");
        logger->warn("API Request: method=GET, path=/api/products, status=429, duration=12ms, user_id=12345, reason=rate_limited");
        
        // 模拟数据库操作日志
        logger->debug("DB Query: table=users, operation=SELECT, duration=15ms, rows=1");
        logger->debug("DB Query: table=orders, operation=INSERT, duration=8ms, rows=1");
        logger->error("DB Query: table=products, operation=UPDATE, duration=5000ms, rows=0, error=timeout");
        
        // 示例10: 日志刷新和清理
        std::cout << "\n--- Example 10: Log Flushing ---" << std::endl;
        
        logger->info("Before flush - this message should be written immediately");
        logger->flush();
        
        logger->info("After flush - logging system is working correctly");
        
        // 关闭SDK前的最后日志
        logger->info("Logging example completed successfully");
        logger->info("Total examples executed: 10");
        
        // 关闭SDK
        SDK::getInstance().shutdown();
        
        std::cout << "\n=== Logging Example Completed ===" << std::endl;
        std::cout << "Check 'logging_example.log' for file output" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
